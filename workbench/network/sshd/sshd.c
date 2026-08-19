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
 *   - Shell bridge via PIPE: handler
 *   - CTRL-C signal to shutdown
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/socket.h>
#include <exec/types.h>
#include <dos/dostags.h>
#include <dos/dos.h>

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

struct Library *SocketBase;
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

/* --- Public key authentication callback --- */
static int auth_pubkey_cb(ssh_session session, const char *user,
                          struct ssh_key_struct *pubkey,
                          char signature_state, void *userdata)
{
    (void)session; (void)userdata;

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
    return found ? SSH_AUTH_SUCCESS : SSH_AUTH_DENIED;
}

/* --- Password authentication callback --- */
static int auth_password_cb(ssh_session session, const char *user,
                            const char *password, void *userdata)
{
    (void)session; (void)userdata;

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
        return SSH_AUTH_SUCCESS;

    return SSH_AUTH_DENIED;
}

/* --- Shell session context --- */
struct ShellBridge {
    ssh_channel channel;
    BPTR        shell_in;   /* our write end -> shell reads from this pipe */
    BPTR        shell_out;  /* our read end  <- shell writes to this pipe */
    int         cols;
    int         rows;
};

/* --- Unique pipe name generator --- */
static ULONG g_pipe_seq = 0;

static void make_pipe_name(char *buf, size_t len, const char *tag)
{
    snprintf(buf, len, "PIPE:sshd_%s_%p_%lx/32768/rw", tag, (void *)FindTask(NULL), (unsigned long)++g_pipe_seq);
}

/* --- Launch shell with pipe I/O --- */
static BOOL launch_shell(struct ShellBridge *sb)
{
    char in_name[64], out_name[64];

    make_pipe_name(in_name, sizeof(in_name), "in");
    make_pipe_name(out_name, sizeof(out_name), "out");

    /* Open write end of stdin pipe (we write, shell reads) */
    sb->shell_in = Open(in_name, MODE_NEWFILE);
    if (!sb->shell_in)
        return FALSE;

    /* Open read end of stdout pipe (shell writes, we read) */
    sb->shell_out = Open(out_name, MODE_NEWFILE);
    if (!sb->shell_out)
    {
        Close(sb->shell_in);
        sb->shell_in = BNULL;
        return FALSE;
    }

    /* Open the other ends for the shell process */
    BPTR shell_stdin = Open(in_name, MODE_OLDFILE);
    BPTR shell_stdout = Open(out_name, MODE_OLDFILE);

    if (!shell_stdin || !shell_stdout)
    {
        if (shell_stdin) Close(shell_stdin);
        if (shell_stdout) Close(shell_stdout);
        Close(sb->shell_in);
        Close(sb->shell_out);
        sb->shell_in = BNULL;
        sb->shell_out = BNULL;
        return FALSE;
    }

    /* Launch C:Shell asynchronously */
    LONG rc = SystemTags("C:Shell",
        SYS_Input,      (IPTR)shell_stdin,
        SYS_Output,     (IPTR)shell_stdout,
        SYS_Asynch,     TRUE,
        SYS_UserShell,  TRUE,
        NP_CloseInput,  TRUE,
        NP_CloseOutput, TRUE,
        TAG_DONE);

    if (rc == -1)
    {
        Close(shell_stdin);
        Close(shell_stdout);
        Close(sb->shell_in);
        Close(sb->shell_out);
        sb->shell_in = BNULL;
        sb->shell_out = BNULL;
        return FALSE;
    }

    return TRUE;
}

/* --- Process window-change requests from channel messages --- */
static void check_window_change(ssh_session session, struct ShellBridge *sb)
{
    ssh_message msg;

    while ((msg = ssh_message_get(session)) != NULL)
    {
        if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL &&
            ssh_message_subtype(msg) == SSH_CHANNEL_REQUEST_WINDOW_CHANGE)
        {
            sb->cols = ssh_message_channel_request_pty_width(msg);
            sb->rows = ssh_message_channel_request_pty_height(msg);
            ssh_message_channel_request_reply_success(msg);
        }
        else
        {
            ssh_message_reply_default(msg);
        }
        ssh_message_free(msg);
    }
}

/* --- Bidirectional shell bridge with PTY emulation --- */
static void handle_shell(ssh_session session, ssh_channel channel, int cols, int rows)
{
    struct ShellBridge sb = {0};
    sb.channel = channel;
    sb.cols = cols;
    sb.rows = rows;

    if (!launch_shell(&sb))
    {
        ssh_channel_write(channel, "Error: cannot start shell\r\n", 27);
        return;
    }

    char buf[BUF_SIZE];

    /* Main poll loop: bridge SSH channel <-> shell pipes */
    while (ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel))
    {
        /* Shell stdout -> SSH channel (ANSI passthrough, no filtering) */
        if (WaitForChar(sb.shell_out, 20000)) /* 20ms poll */
        {
            LONG n = Read(sb.shell_out, buf, sizeof(buf));
            if (n > 0)
                ssh_channel_write(channel, buf, n);
            else if (n < 0)
                break; /* Shell closed its output */
        }

        /* SSH channel -> shell stdin */
        int nbytes = ssh_channel_read_nonblocking(channel, buf, sizeof(buf), 0);
        if (nbytes > 0)
        {
            Write(sb.shell_in, buf, nbytes);
        }
        else if (nbytes == SSH_ERROR)
            break;

        /* Handle out-of-band channel requests (window-change) */
        check_window_change(session, &sb);

        /* Check for server CTRL-C shutdown */
        if (SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
            break;
    }

    /* Graceful shutdown: close our pipe ends, which signals EOF to shell */
    Close(sb.shell_in);
    Close(sb.shell_out);
}

/* --- Per-connection session handler --- */
static void handle_session(ssh_session session)
{
    struct ssh_server_callbacks_struct cb = {0};
    cb.auth_pubkey_function = auth_pubkey_cb;
    cb.auth_password_function = auth_password_cb;
    cb.userdata = NULL;

    ssh_callbacks_init(&cb);
    ssh_set_server_callbacks(session, &cb);
    ssh_set_auth_methods(session, SSH_AUTH_METHOD_PUBLICKEY | SSH_AUTH_METHOD_PASSWORD);

    if (ssh_handle_key_exchange(session) != SSH_OK)
    {
        Printf("sshd: key exchange failed: %s\n", (IPTR)ssh_get_error(session));
        goto done;
    }

    /* Authentication loop */
    ssh_event event = ssh_event_new();
    ssh_event_add_session(event, session);

    int n_auth = 0;
    while (!ssh_is_authenticated(session) && n_auth < 20)
    {
        if (ssh_event_dopoll(event, 1000) == SSH_ERROR)
            break;
        n_auth++;
    }

    ssh_event_free(event);

    if (!ssh_is_authenticated(session))
    {
        Printf("sshd: authentication failed\n");
        goto done;
    }

    /* Wait for channel open request */
    ssh_channel channel = NULL;
    ssh_message msg;
    int timeout = 30;

    while (timeout-- > 0)
    {
        msg = ssh_message_get(session);
        if (!msg) { Delay(25); continue; }

        if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL_OPEN &&
            ssh_message_subtype(msg) == SSH_CHANNEL_SESSION)
        {
            channel = ssh_message_channel_request_open_reply_accept(msg);
            ssh_message_free(msg);
            break;
        }
        ssh_message_reply_default(msg);
        ssh_message_free(msg);
    }

    if (!channel) goto done;

    /* Wait for shell/exec/pty request, track terminal dimensions */
    BOOL got_shell = FALSE;
    const char *exec_cmd = NULL;
    int pty_cols = 80, pty_rows = 24;
    timeout = 30;

    while (timeout-- > 0 && !got_shell)
    {
        msg = ssh_message_get(session);
        if (!msg) { Delay(25); continue; }

        if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL)
        {
            int subtype = ssh_message_subtype(msg);
            switch (subtype)
            {
                case SSH_CHANNEL_REQUEST_PTY:
                    pty_cols = ssh_message_channel_request_pty_width(msg);
                    pty_rows = ssh_message_channel_request_pty_height(msg);
                    ssh_message_channel_request_reply_success(msg);
                    break;
                case SSH_CHANNEL_REQUEST_SHELL:
                    ssh_message_channel_request_reply_success(msg);
                    got_shell = TRUE;
                    break;
                case SSH_CHANNEL_REQUEST_EXEC:
                    exec_cmd = ssh_message_channel_request_command(msg);
                    if (exec_cmd) exec_cmd = strdup(exec_cmd);
                    ssh_message_channel_request_reply_success(msg);
                    got_shell = TRUE;
                    break;
                default:
                    ssh_message_reply_default(msg);
                    break;
            }
        }
        else
        {
            ssh_message_reply_default(msg);
        }
        ssh_message_free(msg);
    }

    if (!got_shell) goto cleanup_channel;

    if (exec_cmd)
    {
        /* Single command execution */
        char out_name[64];
        make_pipe_name(out_name, sizeof(out_name), "exec");

        BPTR out_w = Open(out_name, MODE_NEWFILE);
        BPTR out_r = Open(out_name, MODE_OLDFILE);

        if (out_w && out_r)
        {
            BPTR nil_in = Open("NIL:", MODE_OLDFILE);
            LONG rc = SystemTags(exec_cmd,
                SYS_Input,      (IPTR)nil_in,
                SYS_Output,     (IPTR)out_w,
                NP_CloseInput,  TRUE,
                NP_CloseOutput, TRUE,
                TAG_DONE);

            char buf[BUF_SIZE];
            LONG n;
            while ((n = Read(out_r, buf, sizeof(buf))) > 0)
                ssh_channel_write(channel, buf, n);

            Close(out_r);
            ssh_channel_request_send_exit_status(channel, rc);
        }
        else
        {
            if (out_w) Close(out_w);
            if (out_r) Close(out_r);
            ssh_channel_write(channel, "Error: cannot execute command\r\n", 30);
        }
        free((void *)exec_cmd);
    }
    else
    {
        /* Interactive shell with PTY bridge */
        handle_shell(session, channel, pty_cols, pty_rows);
    }

cleanup_channel:
    if (channel)
    {
        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
    }

done:
    ssh_disconnect(session);
    ssh_free(session);
}

/* --- Child process entry point for concurrent sessions --- */
struct SessionMsg {
    struct Message msg;
    ssh_session    session;
};

static void session_proc_entry(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct MsgPort *port = &me->pr_MsgPort;

    WaitPort(port);
    struct SessionMsg *sm = (struct SessionMsg *)GetMsg(port);
    ssh_session session = sm->session;
    ReplyMsg(&sm->msg);

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!SocketBase)
    {
        ssh_disconnect(session);
        ssh_free(session);
        return;
    }

    handle_session(session);

    CloseLibrary(SocketBase);
}

static void spawn_session(ssh_session session)
{
    struct MsgPort *reply_port = CreateMsgPort();
    if (!reply_port)
    {
        handle_session(session);
        return;
    }

    struct Process *proc = CreateNewProcTags(
        NP_Entry,   (IPTR)session_proc_entry,
        NP_Name,    (IPTR)"sshd_session",
        NP_Priority, 0,
        NP_StackSize, 65536,
        TAG_DONE);

    if (!proc)
    {
        DeleteMsgPort(reply_port);
        handle_session(session);
        return;
    }

    /* Send session pointer to child via message */
    struct SessionMsg sm = {0};
    sm.msg.mn_ReplyPort = reply_port;
    sm.msg.mn_Length = sizeof(sm);
    sm.session = session;

    PutMsg(&((struct Process *)proc)->pr_MsgPort, &sm.msg);
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

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!SocketBase)
    {
        Printf("sshd: Cannot open bsdsocket.library v4\n");
        return RETURN_FAIL;
    }

    /* Ensure host key exists */
    if (!ensure_host_key())
    {
        CloseLibrary(SocketBase);
        return RETURN_FAIL;
    }

    ssh_init();

    ssh_bind sshbind = ssh_bind_new();
    if (!sshbind)
    {
        Printf("sshd: Cannot create SSH bind\n");
        ssh_finalize();
        CloseLibrary(SocketBase);
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
        CloseLibrary(SocketBase);
        return RETURN_FAIL;
    }

    Printf("sshd: " SSHD_BANNER " listening on port %ld\n", (long)port);
    Printf("sshd: Press CTRL-C to shutdown\n");

    /* Accept loop */
    while (!g_shutdown)
    {
        /* Check for CTRL-C */
        if (SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
        {
            Printf("sshd: Shutdown requested\n");
            break;
        }

        /* Use WaitSelect with 1-second timeout to allow periodic signal checks */
        {
            int bind_fd = ssh_bind_get_fd(sshbind);
            fd_set rfds;
            struct timeval tv;

            FD_ZERO(&rfds);
            FD_SET(bind_fd, &rfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            int sel = WaitSelect(bind_fd + 1, &rfds, NULL, NULL, &tv, NULL);
            if (sel <= 0)
                continue; /* timeout or error — loop back to check CTRL-C */
        }

        ssh_session session = ssh_new();
        if (!session) continue;

        if (ssh_bind_accept(sshbind, session) == SSH_OK)
        {
            spawn_session(session);
        }
        else
        {
            ssh_free(session);
        }
    }

    ssh_bind_free(sshbind);
    ssh_finalize();
    CloseLibrary(SocketBase);

    Printf("sshd: Stopped\n");
    return RETURN_OK;
}
