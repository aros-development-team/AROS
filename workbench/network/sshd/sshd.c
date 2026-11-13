/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 * Author: Fabian Schmieder (@metaneutrons)
 *
 * AROS SSH Server (sshd) using libssh 0.12.0 + mbedTLS 3.6.6
 *
 * Features:
 *   - Host key generation on first run (RSA 2048)
 *   - Public-key auth (ENVARC:SSH/authorized_keys, OpenSSH format)
 *   - Password auth (SHA-256 hash in ENVARC:SSH/password)
 *   - Concurrent sessions via CreateNewProc
 *   - Interactive shell: the session task is the new CLI's console
 *   - Commands (ssh host cmd): output through a T: file, exit status kept
 *   - CTRL-C signal to shutdown
 */

/* bsdsocket bases are per task: the main task and every session task open
 * their own, and all socket calls (ours and libssh's) resolve the base
 * through __libssh_SocketBase() below */
struct Library;
struct Library *__libssh_SocketBase(void);
#define SocketBase (__libssh_SocketBase())

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/socket.h>
#include <proto/netservice.h>
#include <libraries/bsdsocket.h>
#include <exec/types.h>
#include <dos/dostags.h>
#include <dos/cliinit.h>     /* CLI_NEWCLI */
#include <dos/dos.h>
#include <libraries/netservice.h>
#include <utility/tagitem.h>

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/sha256.h>
#include <mbedtls/error.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char __attribute__((used)) verstag[] = "\0$VER: sshd 0.1 (19.8.2026)\r\n";

#define SSHD_PORT           22
#define SSHD_HOST_KEY       "ENVARC:SSH/host_key"
#define SSHD_AUTH_KEYS      "ENVARC:SSH/authorized_keys"
#define SSHD_PASSWORD_FILE  "ENVARC:SSH/password"
#define SSHD_CONFIG_DIR     "ENVARC:SSH"
#define SSHD_BANNER         "AROS SSH Server 0.1"
#define RSA_KEY_BITS        2048
#define MAX_LINE            1024
#define BUF_SIZE            4096

/* The main task's base; a session task keeps its own in tc_UserData
 * (CreateNewProc's NP_UserData slot, unused by anything else here). */
static struct Task *DaemonTask;
static struct Library *DaemonSocketBase;

struct Library *__libssh_SocketBase(void)
{
    struct Task *me = FindTask(NULL);

    if (me == DaemonTask)
        return DaemonSocketBase;
    return (struct Library *)me->tc_UserData;
}
struct Library *NetServicesBase;        /* optional: set when run as a managed service */
static volatile BOOL g_shutdown = FALSE;

/* --- Utility: SHA-256 hex digest --- */
static void sha256_hex(const char *input, char *output)
{
    unsigned char hash[32];
    mbedtls_sha256((const unsigned char *)input, strnlen(input, 1024), hash, 0);

    int i;
    for (i = 0; i < 32; i++)
        sprintf(output + (i * 2), "%02x", hash[i]);
    output[64] = '\0';
}

/* --- Host key generation using mbedTLS --- */
static BOOL generate_host_key(void)
{
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char pers[] = "aros_sshd_keygen";
    unsigned char buf[16384];
    int ret;
    BPTR fh;

    Printf("sshd: Generating RSA %ld-bit host key...\n", (long)RSA_KEY_BITS);

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                 (const unsigned char *)pers, sizeof(pers) - 1);
    if (ret != 0) goto fail;

    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret != 0) goto fail;

    ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), mbedtls_ctr_drbg_random,
                               &ctr_drbg, RSA_KEY_BITS, 65537);
    if (ret != 0) goto fail;

    /* Write PEM to buffer */
    memset(buf, 0, sizeof(buf));
    ret = mbedtls_pk_write_key_pem(&pk, buf, sizeof(buf));
    if (ret != 0) goto fail;

    /* Ensure config directory exists */
    BPTR lock = CreateDir(SSHD_CONFIG_DIR);
    if (lock) UnLock(lock);

    /* Save to file */
    fh = Open(SSHD_HOST_KEY, MODE_NEWFILE);
    if (!fh) goto fail;

    Write(fh, buf, strnlen((char *)buf, sizeof(buf)));
    Close(fh);

    Printf("sshd: Host key saved to %s\n", (IPTR)SSHD_HOST_KEY);

    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return TRUE;

fail:
    Printf("sshd: Failed to generate host key (mbedtls error %ld)\n", (long)ret);
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return FALSE;
}

/* --- Ensure host key exists --- */
static BOOL ensure_host_key(void)
{
    BPTR lock = Lock(SSHD_HOST_KEY, SHARED_LOCK);
    if (lock)
    {
        UnLock(lock);
        return TRUE;
    }
    return generate_host_key();
}

/* Per-session state handed to the libssh callbacks as userdata; libssh has
 * no server-side "is authenticated" query.
 *
 * Once a session has server callbacks (ssh_set_server_callbacks()), libssh
 * answers every message that no callback takes with ssh_message_reply_default()
 * and never queues it for ssh_message_get(): a channel open would be refused
 * ("administratively prohibited"). So the channel and its requests are taken
 * by callbacks too, and recorded here. */
struct SessionCtx {
    int authenticated;
    ssh_channel channel;                /* the session channel, one per connection */
    struct ssh_channel_callbacks_struct chan_cb;
    int got_shell;                      /* a shell or exec request was accepted */
    char *exec_cmd;                     /* exec: the command, strdup()ed */
    int pty;                            /* a pty was asked for: a terminal is at the other end */
    int cols, rows;                     /* the terminal's size, from pty-req and window-change */
};

/* --- Public key authentication callback --- */
static int auth_pubkey_cb(ssh_session session, const char *user,
                          struct ssh_key_struct *pubkey,
                          char signature_state, void *userdata)
{
    struct SessionCtx *ctx = userdata;

    (void)session;

    if (strcmp(user, "root") != 0)
        return SSH_AUTH_DENIED;

    /* Probe phase: accept any key type for further verification */
    if (signature_state == SSH_PUBLICKEY_STATE_NONE)
        return SSH_AUTH_SUCCESS;

    if (signature_state != SSH_PUBLICKEY_STATE_VALID)
        return SSH_AUTH_DENIED;

    /* Verify key against authorized_keys */
    BPTR fh = Open(SSHD_AUTH_KEYS, MODE_OLDFILE);
    if (!fh)
        return SSH_AUTH_DENIED;

    char line[MAX_LINE];
    BOOL found = FALSE;

    while (FGets(fh, line, sizeof(line)))
    {
        /* Strip newline */
        int len = strnlen(line, sizeof(line));
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\0')
            continue;

        /* OpenSSH format: "type base64 comment" - extract base64 part */
        char *space1 = strchr(line, ' ');
        if (!space1) continue;

        char *b64_start = space1 + 1;
        char *space2 = strchr(b64_start, ' ');
        if (space2) *space2 = '\0';

        /* Determine key type from prefix */
        enum ssh_keytypes_e ktype = SSH_KEYTYPE_UNKNOWN;
        if (strncmp(line, "ssh-rsa", 7) == 0)
            ktype = SSH_KEYTYPE_RSA;
        else if (strncmp(line, "ssh-ed25519", 11) == 0)
            ktype = SSH_KEYTYPE_ED25519;
        else if (strncmp(line, "ecdsa-sha2", 10) == 0)
            ktype = SSH_KEYTYPE_ECDSA_P256;

        ssh_key ref_key = NULL;
        if (ssh_pki_import_pubkey_base64(b64_start, ktype, &ref_key) == SSH_OK)
        {
            if (ssh_key_cmp(pubkey, ref_key, SSH_KEY_CMP_PUBLIC) == 0)
                found = TRUE;
            ssh_key_free(ref_key);
        }

        if (found) break;
    }

    Close(fh);
    if (found)
        ctx->authenticated = 1;
    return found ? SSH_AUTH_SUCCESS : SSH_AUTH_DENIED;
}

/* --- Password authentication callback --- */
static int auth_password_cb(ssh_session session, const char *user,
                            const char *password, void *userdata)
{
    struct SessionCtx *ctx = userdata;

    (void)session;

    if (strcmp(user, "root") != 0)
        return SSH_AUTH_DENIED;

    /* Password auth only if password file exists */
    BPTR fh = Open(SSHD_PASSWORD_FILE, MODE_OLDFILE);
    if (!fh)
        return SSH_AUTH_DENIED;

    char stored_hash[65] = {0};
    Read(fh, stored_hash, 64);
    Close(fh);
    stored_hash[64] = '\0';

    /* Strip any trailing whitespace */
    int len = strnlen(stored_hash, sizeof(stored_hash));
    while (len > 0 && (stored_hash[len-1] == '\n' || stored_hash[len-1] == '\r' || stored_hash[len-1] == ' '))
        stored_hash[--len] = '\0';

    /* Hash the provided password and compare */
    char computed_hash[65];
    sha256_hex(password, computed_hash);

    if (strcmp(computed_hash, stored_hash) == 0)
    {
        ctx->authenticated = 1;
        return SSH_AUTH_SUCCESS;
    }

    return SSH_AUTH_DENIED;
}

/* --- Channel callbacks: pty-req, shell, exec, window-change --- */
static int chan_pty_cb(ssh_session session, ssh_channel channel, const char *term,
                       int width, int height, int pxwidth, int pxheight, void *userdata)
{
    struct SessionCtx *ctx = userdata;

    (void)session; (void)channel; (void)term; (void)pxwidth; (void)pxheight;
    ctx->pty = 1;
    ctx->cols = width;
    ctx->rows = height;
    return 0;
}

static int chan_shell_cb(ssh_session session, ssh_channel channel, void *userdata)
{
    struct SessionCtx *ctx = userdata;

    (void)session; (void)channel;
    if (ctx->got_shell)
        return -1;
    ctx->got_shell = 1;
    return 0;
}

static int chan_exec_cb(ssh_session session, ssh_channel channel, const char *command,
                        void *userdata)
{
    struct SessionCtx *ctx = userdata;

    (void)session; (void)channel;
    if (ctx->got_shell || command == NULL || (ctx->exec_cmd = strdup(command)) == NULL)
        return -1;
    ctx->got_shell = 1;
    return 0;
}

static int chan_winch_cb(ssh_session session, ssh_channel channel,
                         int width, int height, int pxwidth, int pxheight, void *userdata)
{
    struct SessionCtx *ctx = userdata;

    (void)session; (void)channel; (void)pxwidth; (void)pxheight;
    ctx->cols = width;
    ctx->rows = height;
    return 0;
}

/* A session channel, once the client is authenticated: one per connection. */
static ssh_channel chan_open_cb(ssh_session session, void *userdata)
{
    struct SessionCtx *ctx = userdata;

    if (!ctx->authenticated || ctx->channel != NULL)
        return NULL;
    if ((ctx->channel = ssh_channel_new(session)) == NULL)
        return NULL;
    memset(&ctx->chan_cb, 0, sizeof(ctx->chan_cb));
    ctx->chan_cb.userdata = ctx;
    ctx->chan_cb.channel_pty_request_function = chan_pty_cb;
    ctx->chan_cb.channel_shell_request_function = chan_shell_cb;
    ctx->chan_cb.channel_exec_request_function = chan_exec_cb;
    ctx->chan_cb.channel_pty_window_change_function = chan_winch_cb;
    ssh_callbacks_init(&ctx->chan_cb);
    ssh_set_channel_callbacks(ctx->channel, &ctx->chan_cb);
    return ctx->channel;
}

/* --- The interactive shell's console ---
 * The shell's input and output are file handles whose handler is this
 * session task: their packets come to con->port, and are answered from the
 * SSH channel. A pipe would not do: a read from PIPE: returns only when it
 * has filled its whole buffer (or at the end), and the shell reads its
 * commands through a buffer of 208 bytes at least, so it would see no line
 * until the client closed its input. Here a read is answered with what has
 * been typed, the handles are interactive (the shell prompts), and ^C
 * breaks the shell's command. */
static ULONG g_pipe_seq = 0;            /* names the exec output files */

#define CON_INMAX   2048
#define CON_READQ   8

struct ShellCon {
    struct MsgPort   *port;             /* the handler port of both handles */
    ssh_channel       channel;
    struct SessionCtx *ctx;
    char              in[CON_INMAX];    /* typed, not yet read by the shell */
    int               ilen;
    struct DosPacket *readq[CON_READQ]; /* reads waiting for input */
    int               nread;
    int               opens;            /* handles of ours still open */
    BOOL              eof;              /* no more input */
    struct Task      *breaktask;        /* gets ^C: the console's last reader */
    /* the line discipline, for a client that asked for a pty */
    char              line[512];
    int               len;
    int               esc;              /* 1 after ESC, 2 inside ESC [ ... */
    BOOL              last_cr;
};

/* Answer the waiting reads from what has been typed, or with the end. */
static void con_serve_reads(struct ShellCon *con)
{
    while (con->nread > 0 && (con->ilen > 0 || con->eof))
    {
        struct DosPacket *dp = con->readq[0];
        LONG n = con->ilen < dp->dp_Arg3 ? con->ilen : dp->dp_Arg3;

        memcpy((APTR)dp->dp_Arg2, con->in, n);
        memmove(con->in, con->in + n, con->ilen - n);
        con->ilen -= n;
        memmove(con->readq, con->readq + 1, --con->nread * sizeof(con->readq[0]));
        ReplyPkt(dp, n, 0);
    }
}

static void con_queue_input(struct ShellCon *con, const char *s, int n)
{
    if (n > CON_INMAX - con->ilen)
        n = CON_INMAX - con->ilen;
    memcpy(con->in + con->ilen, s, n);
    con->ilen += n;
}

/* What the client types. With a pty there is nothing at the other end to
 * echo or edit a line, so it is done here, and a line goes to the shell
 * when Return ends it. */
static void con_input(struct ShellCon *con, const char *in, int n)
{
    int i;

    if (!con->ctx->pty)
    {
        con_queue_input(con, in, n);
        con_serve_reads(con);
        return;
    }
    for (i = 0; i < n; i++)
    {
        unsigned char c = in[i];

        if (con->esc == 1) { con->esc = (c == '[' || c == 'O') ? 2 : 0; continue; }
        if (con->esc == 2) { if (c >= 0x40 && c <= 0x7e) con->esc = 0; continue; }
        if (c == 0x1b) { con->esc = 1; continue; }  /* cursor keys and the like: dropped */

        if (c == '\n' && con->last_cr) { con->last_cr = FALSE; continue; }
        con->last_cr = (c == '\r');
        if (c == '\r' || c == '\n')
        {
            ssh_channel_write(con->channel, "\r\n", 2);
            con->line[con->len++] = '\n';
            con_queue_input(con, con->line, con->len);
            con->len = 0;
        }
        else if (c == 0x7f || c == 0x08)
        {
            if (con->len > 0) { con->len--; ssh_channel_write(con->channel, "\b \b", 3); }
        }
        else if (c == 0x03)                         /* ^C: break the shell's command */
        {
            ssh_channel_write(con->channel, "^C\r\n", 4);
            con->len = 0;
            if (con->breaktask)
                Signal(con->breaktask, SIGBREAKF_CTRL_C);
        }
        else if (c == 0x04)                         /* ^D on an empty line: the end */
        {
            if (con->len == 0)
                con->eof = TRUE;
        }
        else if (c >= 0x20 && con->len < (int)sizeof(con->line) - 1)
        {
            con->line[con->len++] = c;
            ssh_channel_write(con->channel, (char *)&c, 1);
        }
    }
    con_serve_reads(con);
}

/* The shell's output: LF as CRLF for a terminal. */
static void con_output(struct ShellCon *con, const char *s, LONG n)
{
    LONG i, start = 0;

    if (!con->ctx->pty)
    {
        ssh_channel_write(con->channel, s, n);
        return;
    }
    for (i = 0; i < n; i++)
        if (s[i] == '\n')
        {
            if (i > start)
                ssh_channel_write(con->channel, s + start, i - start);
            ssh_channel_write(con->channel, "\r\n", 2);
            start = i + 1;
        }
    if (n > start)
        ssh_channel_write(con->channel, s + start, n - start);
}

/* The packets the shell and its commands send to their console. */
static void con_packets(struct ShellCon *con)
{
    struct Message *m;

    while ((m = GetMsg(con->port)) != NULL)
    {
        struct DosPacket *dp = (struct DosPacket *)m->mn_Node.ln_Name;

        switch (dp->dp_Type)
        {
        case ACTION_READ:
            /* ^C goes to whoever reads the console: the shell, and so
             * the command it runs (as con-handler does) */
            if (dp->dp_Port && dp->dp_Port->mp_SigTask)
                con->breaktask = dp->dp_Port->mp_SigTask;
            if (con->nread < CON_READQ)
            {
                con->readq[con->nread++] = dp;
                con_serve_reads(con);
            }
            else
                ReplyPkt(dp, -1, ERROR_OBJECT_IN_USE);
            break;
        case ACTION_WRITE:
            con_output(con, (const char *)dp->dp_Arg2, dp->dp_Arg3);
            ReplyPkt(dp, dp->dp_Arg3, 0);
            break;
        case ACTION_WAIT_CHAR:
            ReplyPkt(dp, con->ilen > 0 ? DOSTRUE : DOSFALSE, 0);
            break;
        case ACTION_FINDINPUT:                      /* Open("*") and the like */
        case ACTION_FINDOUTPUT:
        case ACTION_FINDUPDATE:
        {
            struct FileHandle *fh = BADDR(dp->dp_Arg1);

            fh->fh_Arg1 = (SIPTR)con;
            fh->fh_Interactive = DOSTRUE;
            con->opens++;
            ReplyPkt(dp, DOSTRUE, 0);
            break;
        }
        case ACTION_END:
            con->opens--;
            ReplyPkt(dp, DOSTRUE, 0);
            break;
        case ACTION_SCREEN_MODE:                    /* raw or cooked: always cooked here */
            ReplyPkt(dp, DOSTRUE, 0);
            break;
        case ACTION_CHANGE_SIGNAL:
            /* Callers pass a port or a task, and later restore the old
             * value from Res2: acknowledged, and ^C keeps going to the
             * console's reader. */
            ReplyPkt(dp, DOSTRUE, 0);
            break;
        default:
            ReplyPkt(dp, DOSFALSE, ERROR_ACTION_NOT_KNOWN);
            break;
        }
    }
}

/* One of the console's handles. */
static BPTR con_handle(struct ShellCon *con)
{
    struct FileHandle *fh = AllocDosObject(DOS_FILEHANDLE, NULL);

    if (fh == NULL)
        return BNULL;
    fh->fh_Type = con->port;
    fh->fh_Arg1 = (SIPTR)con;
    fh->fh_Interactive = DOSTRUE;
    con->opens++;
    return MKBADDR(fh);
}

/* The shell is started by a process of its own: SystemTags() waits for
 * the new CLI to take its start-up packet, and the CLI may first talk to
 * its console - this session task, which must be free to answer. */
struct ShellStart {
    struct Message msg;
    BPTR           in, out;
    LONG           rc;
};

static void shell_starter_entry(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct ShellStart *ss;

    WaitPort(&me->pr_MsgPort);
    ss = (struct ShellStart *)GetMsg(&me->pr_MsgPort);
    /* A new interactive CLI (CLI_NEWCLI) that takes its commands from its
     * input until that ends - a SYSTEM one would run one line and stop.
     * The handles are its, and it closes them when it ends. */
    ss->rc = SystemTags("",
        SYS_Input,      (IPTR)ss->in,
        SYS_Output,     (IPTR)ss->out,
        SYS_CliType,    CLI_NEWCLI,
        SYS_UserShell,  TRUE,
        TAG_DONE);
    ReplyMsg(&ss->msg);
}

/* --- Interactive shell over the channel ---
 * Window changes arrive through chan_winch_cb() while the channel is read,
 * and land in ctx. */
static void handle_shell(ssh_session session, ssh_channel channel, struct SessionCtx *ctx)
{
    struct ShellCon *con;
    BPTR in, out;
    char buf[BUF_SIZE];
    int fd = ssh_get_fd(session);
    BOOL in_eof_seen = FALSE, started = FALSE;
    struct MsgPort *starter_reply = NULL;
    struct Process *starter = NULL;
    struct ShellStart start = {0};

    if ((con = AllocVec(sizeof(*con), MEMF_ANY | MEMF_CLEAR)) == NULL ||
        (con->port = CreateMsgPort()) == NULL)
    {
        ssh_channel_write(channel, "Error: cannot start shell\r\n", 27);
        FreeVec(con);
        return;
    }
    con->channel = channel;
    con->ctx = ctx;
    in = con_handle(con);
    out = con_handle(con);
    starter_reply = CreateMsgPort();
    if (in && out && starter_reply)
        starter = CreateNewProcTags(
            NP_Entry,     (IPTR)shell_starter_entry,
            NP_Name,      (IPTR)"sshd_shell_start",
            NP_StackSize, 16384,
            TAG_DONE);
    if (starter == NULL)
    {
        ssh_channel_write(channel, "Error: cannot start shell\r\n", 27);
        if (in) FreeDosObject(DOS_FILEHANDLE, BADDR(in));
        if (out) FreeDosObject(DOS_FILEHANDLE, BADDR(out));
        if (starter_reply) DeleteMsgPort(starter_reply);
        DeleteMsgPort(con->port);
        FreeVec(con);
        return;
    }
    start.msg.mn_ReplyPort = starter_reply;
    start.msg.mn_Length = sizeof(start);
    start.in = in;
    start.out = out;
    PutMsg(&starter->pr_MsgPort, &start.msg);

    /* Serve the console while the shell starts */
    while (!started)
    {
        con_packets(con);
        if (GetMsg(starter_reply))
            started = TRUE;
        else
            Wait((1UL << con->port->mp_SigBit) | (1UL << starter_reply->mp_SigBit));
    }
    DeleteMsgPort(starter_reply);
    if (start.rc == -1)
    {
        /* no shell: the handles were not taken */
        ssh_channel_write(channel, "Error: cannot start shell\r\n", 27);
        FreeDosObject(DOS_FILEHANDLE, BADDR(in));
        FreeDosObject(DOS_FILEHANDLE, BADDR(out));
        DeleteMsgPort(con->port);
        FreeVec(con);
        return;
    }

    /* Until the shell has closed its console */
    while (con->opens > 0)
    {
        fd_set rfds;
        struct timeval tv = { 0, 200000 };
        ULONG sigs = (1UL << con->port->mp_SigBit) | SIGBREAKF_CTRL_C;

        con_packets(con);
        if (con->opens <= 0)
            break;

        if (ssh_channel_is_open(channel))
        {
            int nbytes = ssh_channel_read_nonblocking(channel, buf, sizeof(buf), 0);

            if (nbytes > 0)
                con_input(con, buf, nbytes);
            if (nbytes == SSH_ERROR || !ssh_channel_is_open(channel) ||
                (ssh_channel_is_eof(channel) && !in_eof_seen))
            {
                /* No more input: the shell sees the end of its own, and
                 * what it writes until it ends still goes out. */
                in_eof_seen = TRUE;
                con->eof = TRUE;
                con_serve_reads(con);
            }
        }
        else if (!con->eof)
        {
            con->eof = TRUE;
            con_serve_reads(con);
        }

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        if (WaitSelect(fd + 1, &rfds, NULL, NULL, &tv, &sigs) < 0)
            break;
        if (sigs & SIGBREAKF_CTRL_C)
        {
            /* the daemon is stopping: end the shell's input */
            con->eof = TRUE;
            con_serve_reads(con);
            if (con->breaktask)
                Signal(con->breaktask, SIGBREAKF_CTRL_C);
        }
    }

    if (con->opens > 0)
    {
        /* The shell still has its console: the port must stay for it. */
        Printf("sshd: the shell did not end; its console is left open\n");
        return;
    }
    ssh_channel_request_send_exit_status(channel, 0);
    DeleteMsgPort(con->port);
    FreeVec(con);
}

/* --- Per-connection session handler --- */
static void handle_session(ssh_session session)
{
    struct ssh_server_callbacks_struct cb = {0};
    struct SessionCtx ctx = {0};

    ssh_channel channel;
    const char *exec_cmd;

    ctx.cols = 80;
    ctx.rows = 24;
    cb.auth_pubkey_function = auth_pubkey_cb;
    cb.auth_password_function = auth_password_cb;
    cb.channel_open_request_session_function = chan_open_cb;
    cb.userdata = &ctx;

    ssh_callbacks_init(&cb);
    ssh_set_server_callbacks(session, &cb);
    ssh_set_auth_methods(session, SSH_AUTH_METHOD_PUBLICKEY | SSH_AUTH_METHOD_PASSWORD);

    if (ssh_handle_key_exchange(session) != SSH_OK)
    {
        Printf("sshd: key exchange failed: %s\n", (IPTR)ssh_get_error(session));
        goto done;
    }

    /* Authentication, the session channel and its pty/shell/exec requests
     * all arrive through the callbacks: run the session until a shell or a
     * command is asked for, for a minute at most. */
    ssh_event event = ssh_event_new();
    ssh_event_add_session(event, session);

    int polls = 0;
    while (!ctx.got_shell && polls++ < 60)
    {
        if (ssh_event_dopoll(event, 1000) == SSH_ERROR)
            break;
    }

    ssh_event_free(event);

    if (!ctx.authenticated)
    {
        Printf("sshd: authentication failed\n");
        goto cleanup_channel;
    }

    channel = ctx.channel;
    exec_cmd = ctx.exec_cmd;
    if (!channel || !ctx.got_shell) goto cleanup_channel;

    if (exec_cmd)
    {
        /* Single command execution. The command runs to its end with its
         * output in a file, which is then sent: a synchronous SystemTags()
         * does not close the handles it is given, so a pipe read after it
         * would never see EOF - and a command that wrote more than the pipe
         * holds would wait for a reader that is not reading yet. */
        char out_name[64];
        snprintf(out_name, sizeof(out_name), "T:sshd_exec_%p_%lx",
                 (void *)FindTask(NULL), (unsigned long)++g_pipe_seq);

        BPTR out_w = Open(out_name, MODE_NEWFILE);
        BPTR nil_in = Open("NIL:", MODE_OLDFILE);

        if (out_w && nil_in)
        {
            LONG rc = SystemTags(exec_cmd,
                SYS_Input,      (IPTR)nil_in,
                SYS_Output,     (IPTR)out_w,
                TAG_DONE);
            BPTR out_r;

            Close(out_w);
            Close(nil_in);
            if ((out_r = Open(out_name, MODE_OLDFILE)))
            {
                char buf[BUF_SIZE];
                LONG n;

                while ((n = Read(out_r, buf, sizeof(buf))) > 0)
                    ssh_channel_write(channel, buf, n);
                Close(out_r);
            }
            DeleteFile(out_name);
            ssh_channel_request_send_exit_status(channel, rc);
        }
        else
        {
            if (out_w) { Close(out_w); DeleteFile(out_name); }
            if (nil_in) Close(nil_in);
            ssh_channel_write(channel, "Error: cannot execute command\r\n", 30);
        }
    }
    else
    {
        /* Interactive shell, the session task its console */
        handle_shell(session, channel, &ctx);
    }

cleanup_channel:
    if (ctx.channel)
    {
        ssh_channel_send_eof(ctx.channel);
        ssh_channel_close(ctx.channel);
        ssh_channel_free(ctx.channel);
    }
    free(ctx.exec_cmd);

done:
    ssh_disconnect(session);
    ssh_free(session);
}

/* --- Session tasks ---
 * Every session runs in its own process with its own bsdsocket base. The
 * main task accepts the connection and parks the socket in the stack with
 * ReleaseSocket(); the session task adopts it with ObtainSocket() on its
 * own base and wraps it with ssh_bind_accept_fd(), which only reads the
 * bind object (listening, host keys already imported). */
struct SessionMsg {
    struct Message msg;
    ssh_bind       sshbind;
    LONG           socket_id;   /* ReleaseSocket() unique id */
};

/* Serve an accepted socket owned by the calling task's base. */
static void serve_socket(ssh_bind sshbind, LONG fd)
{
    ssh_session session = ssh_new();

    if (session == NULL)
    {
        CloseSocket(fd);
        return;
    }

    if (ssh_bind_accept_fd(sshbind, session, fd) != SSH_OK)
    {
        Printf("sshd: accept failed: %s\n", (IPTR)ssh_get_error(sshbind));
        CloseSocket(fd);
        ssh_free(session);
        return;
    }

    handle_session(session);
}

static void session_proc_entry(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct MsgPort *port = &me->pr_MsgPort;
    struct SessionMsg *sm;
    ssh_bind sshbind;
    LONG socket_id, fd;

    WaitPort(port);
    sm = (struct SessionMsg *)GetMsg(port);
    sshbind = sm->sshbind;
    socket_id = sm->socket_id;
    ReplyMsg(&sm->msg);

    /* our own base; __libssh_SocketBase() finds it here from now on */
    me->pr_Task.tc_UserData = OpenLibrary("bsdsocket.library", 4);
    if (me->pr_Task.tc_UserData == NULL)
    {
        Printf("sshd: session cannot open bsdsocket.library\n");
        return;
    }

    fd = ObtainSocket(socket_id, 0, 0, 0);
    if (fd >= 0)
        serve_socket(sshbind, fd);
    else
        Printf("sshd: ObtainSocket failed (%ld)\n", (long)Errno());

    CloseLibrary((struct Library *)me->pr_Task.tc_UserData);
    me->pr_Task.tc_UserData = NULL;
}

static void spawn_session(ssh_bind sshbind, LONG fd)
{
    struct MsgPort *reply_port;
    struct Process *proc = NULL;
    struct SessionMsg sm = {0};
    LONG socket_id;

    socket_id = ReleaseSocket(fd, UNIQUE_ID);
    if (socket_id < 0)
    {
        CloseSocket(fd);
        return;
    }

    reply_port = CreateMsgPort();
    if (reply_port != NULL)
        proc = CreateNewProcTags(
            NP_Entry,   (IPTR)session_proc_entry,
            NP_Name,    (IPTR)"sshd_session",
            NP_Priority, 0,
            NP_StackSize, 65536,
            TAG_DONE);

    if (proc == NULL)
    {
        /* no session task: take the socket back and serve it here */
        if (reply_port != NULL)
            DeleteMsgPort(reply_port);
        fd = ObtainSocket(socket_id, 0, 0, 0);
        if (fd >= 0)
            serve_socket(sshbind, fd);
        return;
    }

    sm.msg.mn_ReplyPort = reply_port;
    sm.msg.mn_Length = sizeof(sm);
    sm.sshbind = sshbind;
    sm.socket_id = socket_id;

    PutMsg(&proc->pr_MsgPort, &sm.msg);
    WaitPort(reply_port);
    GetMsg(reply_port);
    DeleteMsgPort(reply_port);
}

/* --- Main --- */
int main(int argc, char **argv)
{
    int port = SSHD_PORT;
    int i;

    /* Parse arguments */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
            port = atoi(argv[++i]);
        else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0)
        {
            Printf("AROS SSH Server 0.1 (libssh 0.12.0, mbedTLS 3.6.7)\n");
            return RETURN_OK;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "?") == 0)
        {
            Printf("AROS SSH Server 0.1\n");
            Printf("Usage: sshd [-p port] [-V]\n");
            return RETURN_OK;
        }
    }

    DaemonTask = FindTask(NULL);
    DaemonSocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!DaemonSocketBase)
    {
        Printf("sshd: Cannot open bsdsocket.library v4\n");
        return RETURN_FAIL;
    }

    /* Ensure host key exists */
    if (!ensure_host_key())
    {
        CloseLibrary(DaemonSocketBase);
        return RETURN_FAIL;
    }

    ssh_init();

    ssh_bind sshbind = ssh_bind_new();
    if (!sshbind)
    {
        Printf("sshd: Cannot create SSH bind\n");
        ssh_finalize();
        CloseLibrary(DaemonSocketBase);
        return RETURN_FAIL;
    }

    char port_str[8];
    sprintf(port_str, "%d", port);

    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, port_str);
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, SSHD_HOST_KEY);
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BANNER, SSHD_BANNER);

    if (ssh_bind_listen(sshbind) < 0)
    {
        Printf("sshd: Listen failed: %s\n", (IPTR)ssh_get_error(sshbind));
        ssh_bind_free(sshbind);
        ssh_finalize();
        CloseLibrary(DaemonSocketBase);
        return RETURN_FAIL;
    }

    Printf("sshd: " SSHD_BANNER " listening on port %ld\n", (long)port);

    /*
     * Register with netservices.library when it is present, so the stack's
     * service manager can run us from db/services.d/sshd and drive us across a
     * configuration reload and a clean shutdown.  Started from a plain CLI
     * (no stack service manager) this simply doesn't register and CTRL-C still
     * works.  We ask for the reconfigure-begin/end and stop signals; on begin
     * we only pause accepting (we must NOT call into the stack while it holds
     * its reload fence), on end we resume, on stop we exit.
     */
    BYTE  sStop  = AllocSignal(-1);
    BYTE  sBegin = AllocSignal(-1);
    BYTE  sEnd   = AllocSignal(-1);
    ULONG mStop  = (sStop  >= 0) ? (1UL << sStop)  : 0;
    ULONG mBegin = (sBegin >= 0) ? (1UL << sBegin) : 0;
    ULONG mEnd   = (sEnd   >= 0) ? (1UL << sEnd)   : 0;
    APTR  nshandle = NULL;
    BOOL  paused = FALSE;

    NetServicesBase = OpenLibrary(NETSERVICESNAME, 0);
    if (NetServicesBase)
    {
        struct TagItem nstags[] = {
            { NETSERVICE_Name,             (IPTR)"sshd" },
            { NETSERVICE_Order,            (IPTR)NSPRI_RCDAEMONS },
            { NETSERVICE_ReconfigBeginSig, (IPTR)sBegin },
            { NETSERVICE_ReconfigEndSig,   (IPTR)sEnd },
            { NETSERVICE_StopSig,          (IPTR)sStop },
            { TAG_DONE, 0 }
        };
        nshandle = RegisterNetService(nstags);
    }
    if (nshandle)
        Printf("sshd: registered as a network service\n");
    else
        Printf("sshd: Press CTRL-C to shutdown\n");

    /* Accept loop.  Existing sessions run in their own processes, unaffected. */
    while (!g_shutdown)
    {
        ULONG sigs;

        if (paused)
        {
            /* Quiesced during a stack reconfigure: wait only on signals. */
            sigs = Wait(mStop | mBegin | mEnd | SIGBREAKF_CTRL_C);
        }
        else
        {
            int bind_fd = ssh_bind_get_fd(sshbind);
            fd_set rfds;
            struct timeval tv;
            ULONG sigmask = mStop | mBegin | mEnd | SIGBREAKF_CTRL_C;
            int sel;

            FD_ZERO(&rfds);
            FD_SET(bind_fd, &rfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            /* WaitSelect also returns when one of the requested signals fires;
             * sigmask comes back holding the signals that were received. */
            sel = WaitSelect(bind_fd + 1, &rfds, NULL, NULL, &tv, &sigmask);
            sigs = sigmask;

            if (sel > 0 && FD_ISSET(bind_fd, &rfds))
            {
                LONG fd = accept(bind_fd, NULL, NULL);

                if (fd >= 0)
                    spawn_session(sshbind, fd);
            }
        }

        if (sigs & (mStop | SIGBREAKF_CTRL_C))
        {
            Printf("sshd: Shutdown requested\n");
            break;
        }
        if (sigs & mBegin)
            paused = TRUE;      /* reconfigure beginning - stop touching the stack */
        if (sigs & mEnd)
            paused = FALSE;     /* reconfigure done - resume accepting */
    }

    if (nshandle)
        UnregisterNetService(nshandle);
    if (NetServicesBase)
        CloseLibrary(NetServicesBase);
    if (sStop  >= 0) FreeSignal(sStop);
    if (sBegin >= 0) FreeSignal(sBegin);
    if (sEnd   >= 0) FreeSignal(sEnd);

    ssh_bind_free(sshbind);
    ssh_finalize();
    CloseLibrary(DaemonSocketBase);

    Printf("sshd: Stopped\n");
    return RETURN_OK;
}
