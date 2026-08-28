/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 * Author: Fabian Schmieder (@metaneutrons)
 *
 * AROS SSH Client using libssh 0.12.0 + mbedTLS 3.6.6
 *
 * Usage: ssh [-i keyfile] [-p port] [-get file] [-put file] [user@]host [command]
 *
 * Features:
 *   - Public-key auth (default: ENVARC:SSH/id_rsa)
 *   - Password fallback with prompt
 *   - Known hosts verification (ENVARC:SSH/known_hosts)
 *   - Interactive shell mode (WaitForChar + ssh_channel_read_nonblocking)
 *   - Single command execution
 *   - SCP file transfer (-get / -put)
 *   - Exit code forwarding
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/socket.h>
#include <exec/types.h>
#include <dos/dos.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char __attribute__((used)) verstag[] = "\0$VER: ssh 0.1 (19.8.2026)\r\n";

#define DEFAULT_KEY         "ENVARC:SSH/id_rsa"
#define KNOWN_HOSTS_FILE    "ENVARC:SSH/known_hosts"
#define DEFAULT_PORT        22
#define DEFAULT_USER        "root"
#define BUF_SIZE            4096
#define MAX_LINE            1024
#define SSHD_CONFIG_DIR     "ENVARC:SSH"

struct Library *SocketBase;

/* --- Known hosts verification --- */
static int verify_known_host(ssh_session session)
{
    ssh_key srv_pubkey = NULL;
    unsigned char *hash = NULL;
    size_t hlen = 0;
    char *hexa = NULL;
    int rc;

    rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0) return -1;

    rc = ssh_get_publickey_hash(srv_pubkey, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0) return -1;

    hexa = ssh_get_hexa(hash, hlen);

    /* Try to find in known_hosts */
    BPTR fh = Open(KNOWN_HOSTS_FILE, MODE_OLDFILE);
    if (fh)
    {
        char line[MAX_LINE];
        char *host = NULL;
        ssh_options_get(session, SSH_OPTIONS_HOST, &host);
        BOOL found = FALSE;

        while (FGets(fh, line, sizeof(line)))
        {
            /* Format: "hostname fingerprint" */
            char *sp = strchr(line, ' ');
            if (!sp) continue;
            *sp = '\0';

            if (strcmp(line, host) == 0)
            {
                /* Strip newline from stored fingerprint */
                char *fp = sp + 1;
                int len = strnlen(fp, sizeof(line));
                while (len > 0 && (fp[len-1] == '\n' || fp[len-1] == '\r'))
                    fp[--len] = '\0';

                if (strcmp(fp, hexa) == 0)
                {
                    found = TRUE;
                    break;
                }
                else
                {
                    Printf("WARNING: Host key for '%s' has CHANGED!\n", (IPTR)host);
                    Printf("  Stored:  %s\n", (IPTR)fp);
                    Printf("  Current: %s\n", (IPTR)hexa);
                    Printf("Connection refused. Remove old entry from %s to continue.\n",
                           (IPTR)KNOWN_HOSTS_FILE);
                    Close(fh);
                    ssh_string_free_char(hexa);
                    ssh_clean_pubkey_hash(&hash);
                    return -1;
                }
            }
        }
        Close(fh);

        if (found)
        {
            ssh_string_free_char(host);
            ssh_string_free_char(hexa);
            ssh_clean_pubkey_hash(&hash);
            return 0;
        }

        ssh_string_free_char(host);
    }

    /* Host not found - prompt user */
    char *prompt_host = NULL;
    ssh_options_get(session, SSH_OPTIONS_HOST, &prompt_host);

    Printf("The authenticity of host '%s' cannot be established.\n",
           (IPTR)(prompt_host ? prompt_host : "unknown"));
    Printf("SHA256 fingerprint: %s\n", (IPTR)hexa);
    Printf("Accept and save? (yes/no): ");
    Flush(Output());

    char answer[16] = {0};
    FGets(Input(), answer, sizeof(answer));

    if (answer[0] != 'y' && answer[0] != 'Y')
    {
        ssh_string_free_char(hexa);
        ssh_clean_pubkey_hash(&hash);
        if (prompt_host) ssh_string_free_char(prompt_host);
        return -1;
    }

    /* Save to known_hosts */
    BPTR lock = CreateDir(SSHD_CONFIG_DIR);
    if (lock) UnLock(lock);

    fh = Open(KNOWN_HOSTS_FILE, MODE_READWRITE);
    if (!fh)
        fh = Open(KNOWN_HOSTS_FILE, MODE_NEWFILE);

    if (fh)
    {
        Seek(fh, 0, OFFSET_END);
        FPrintf(fh, "%s %s\n", (IPTR)(prompt_host ? prompt_host : "unknown"), (IPTR)hexa);
        Close(fh);
    }

    if (prompt_host) ssh_string_free_char(prompt_host);
    ssh_string_free_char(hexa);
    ssh_clean_pubkey_hash(&hash);
    return 0;
}

/* --- Authentication --- */
static int authenticate(ssh_session session, const char *keyfile, const char *user)
{
    int rc;

    /* Try public key authentication first */
    ssh_key privkey = NULL;
    rc = ssh_pki_import_privkey_file(keyfile, NULL, NULL, NULL, &privkey);
    if (rc == SSH_OK)
    {
        rc = ssh_userauth_publickey(session, user, privkey);
        ssh_key_free(privkey);
        if (rc == SSH_AUTH_SUCCESS)
            return 0;
    }

    /* Fallback: password prompt */
    Printf("Password: ");
    Flush(Output());

    /* Read password (no echo on AROS without raw mode) */
    char password[128] = {0};

    /* Try to disable echo via SetMode */
    BPTR input = Input();
    SetMode(input, 1); /* Raw mode - disables echo */

    int pos = 0;
    char ch;
    while (pos < (int)sizeof(password) - 1)
    {
        if (WaitForChar(input, 5000000)) /* 5s timeout */
        {
            if (Read(input, &ch, 1) == 1)
            {
                if (ch == '\r' || ch == '\n') break;
                if (ch == 0x08 || ch == 0x7F) /* Backspace/DEL */
                {
                    if (pos > 0) pos--;
                    continue;
                }
                password[pos++] = ch;
            }
        }
    }
    password[pos] = '\0';

    SetMode(input, 0); /* Back to cooked mode */
    Printf("\n");

    rc = ssh_userauth_password(session, user, password);
    memset(password, 0, sizeof(password));

    return (rc == SSH_AUTH_SUCCESS) ? 0 : -1;
}

/* --- SCP download --- */
static int scp_get(ssh_session session, const char *remote_path)
{
    ssh_scp scp = ssh_scp_new(session, SSH_SCP_READ, remote_path);
    if (!scp)
    {
        Printf("ssh: SCP init failed: %s\n", (IPTR)ssh_get_error(session));
        return RETURN_FAIL;
    }

    if (ssh_scp_init(scp) != SSH_OK)
    {
        Printf("ssh: SCP open failed: %s\n", (IPTR)ssh_get_error(session));
        ssh_scp_free(scp);
        return RETURN_FAIL;
    }

    int rc = ssh_scp_pull_request(scp);
    if (rc != SSH_SCP_REQUEST_NEWFILE)
    {
        Printf("ssh: SCP no file received\n");
        ssh_scp_free(scp);
        return RETURN_FAIL;
    }

    size_t file_size = ssh_scp_request_get_size(scp);
    const char *filename = ssh_scp_request_get_filename(scp);

    Printf("Receiving: %s (%lu bytes)\n", (IPTR)filename, (unsigned long)file_size);

    ssh_scp_accept_request(scp);

    /* Extract just the filename from path */
    const char *base = strrchr(remote_path, '/');
    if (base) base++; else base = remote_path;

    BPTR fh = Open(base, MODE_NEWFILE);
    if (!fh)
    {
        Printf("ssh: Cannot create local file '%s'\n", (IPTR)base);
        ssh_scp_free(scp);
        return RETURN_FAIL;
    }

    char buf[BUF_SIZE];
    size_t total = 0;

    while (total < file_size)
    {
        size_t to_read = file_size - total;
        if (to_read > sizeof(buf)) to_read = sizeof(buf);

        int nbytes = ssh_scp_read(scp, buf, to_read);
        if (nbytes == SSH_ERROR) break;

        Write(fh, buf, nbytes);
        total += nbytes;
    }

    Close(fh);
    ssh_scp_free(scp);

    Printf("Received %lu bytes\n", (unsigned long)total);
    return (total == file_size) ? RETURN_OK : RETURN_FAIL;
}

/* --- SCP upload --- */
static int scp_put(ssh_session session, const char *local_path)
{
    BPTR fh = Open(local_path, MODE_OLDFILE);
    if (!fh)
    {
        Printf("ssh: Cannot open local file '%s'\n", (IPTR)local_path);
        return RETURN_FAIL;
    }

    /* Get file size */
    Seek(fh, 0, OFFSET_END);
    LONG file_size = Seek(fh, 0, OFFSET_BEGINNING);

    /* Extract filename */
    const char *base = strrchr(local_path, '/');
    if (!base) base = strrchr(local_path, ':');
    if (base) base++; else base = local_path;

    ssh_scp scp = ssh_scp_new(session, SSH_SCP_WRITE, ".");
    if (!scp)
    {
        Printf("ssh: SCP init failed: %s\n", (IPTR)ssh_get_error(session));
        Close(fh);
        return RETURN_FAIL;
    }

    if (ssh_scp_init(scp) != SSH_OK)
    {
        Printf("ssh: SCP open failed: %s\n", (IPTR)ssh_get_error(session));
        ssh_scp_free(scp);
        Close(fh);
        return RETURN_FAIL;
    }

    if (ssh_scp_push_file(scp, base, file_size, 0644) != SSH_OK)
    {
        Printf("ssh: SCP push failed: %s\n", (IPTR)ssh_get_error(session));
        ssh_scp_free(scp);
        Close(fh);
        return RETURN_FAIL;
    }

    Printf("Sending: %s (%ld bytes)\n", (IPTR)base, (long)file_size);

    char buf[BUF_SIZE];
    LONG n;
    LONG total = 0;

    while ((n = Read(fh, buf, sizeof(buf))) > 0)
    {
        if (ssh_scp_write(scp, buf, n) != SSH_OK)
        {
            Printf("ssh: SCP write error: %s\n", (IPTR)ssh_get_error(session));
            break;
        }
        total += n;
    }

    Close(fh);
    ssh_scp_free(scp);

    Printf("Sent %ld bytes\n", (long)total);
    return (total == file_size) ? RETURN_OK : RETURN_FAIL;
}

/* --- Interactive shell --- */
static int interactive_shell(ssh_session session, ssh_channel channel)
{
    char buf[BUF_SIZE];
    int exit_code = 0;
    BPTR input = Input();

    /* Put console in raw mode for interactive character-by-character I/O */
    SetMode(input, 1);

    while (ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel))
    {
        /* Read from remote (non-blocking) */
        int nbytes = ssh_channel_read_nonblocking(channel, buf, sizeof(buf), 0);
        if (nbytes > 0)
            Write(Output(), buf, nbytes);
        else if (nbytes == SSH_ERROR)
            break;

        /* Also check stderr */
        int err_bytes = ssh_channel_read_nonblocking(channel, buf, sizeof(buf), 1);
        if (err_bytes > 0)
            Write(Output(), buf, err_bytes);

        /* Read from local input (non-blocking, 50ms timeout) */
        if (WaitForChar(input, 50000))
        {
            LONG n = Read(input, buf, sizeof(buf));
            if (n > 0)
                ssh_channel_write(channel, buf, n);
            else if (n < 0)
                break;
        }

        /* Check for CTRL-C */
        if (SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
        {
            ssh_channel_write(channel, "\x03", 1); /* Send CTRL-C to remote */
            break;
        }
    }

    SetMode(input, 0); /* Restore standard line-buffered mode */

    exit_code = ssh_channel_get_exit_status(channel);
    return exit_code;
}

/* --- Main --- */
int main(int argc, char **argv)
{
    const char *host = NULL;
    const char *user = DEFAULT_USER;
    const char *keyfile = DEFAULT_KEY;
    const char *command = NULL;
    const char *scp_get_file = NULL;
    const char *scp_put_file = NULL;
    int port = DEFAULT_PORT;
    int i;
    int exit_code = 0;

    /* Parse arguments */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
            keyfile = argv[++i];
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
            port = atoi(argv[++i]);
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
            user = argv[++i];
        else if (strcmp(argv[i], "-get") == 0 && i + 1 < argc)
            scp_get_file = argv[++i];
        else if (strcmp(argv[i], "-put") == 0 && i + 1 < argc)
            scp_put_file = argv[++i];
        else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0)
        {
            Printf("AROS SSH Client 0.1 (libssh 0.12.0, mbedTLS 3.6.7)\n");
            return RETURN_OK;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "?") == 0)
        {
            Printf("AROS SSH Client 0.1\n");
            Printf("Usage: ssh [-i keyfile] [-p port] [-l user] [user@]host [command]\n");
            Printf("       ssh [-get remotefile] host\n");
            Printf("       ssh [-put localfile] host\n");
            Printf("       ssh -V\n");
            return RETURN_OK;
        }
        else if (!host)
        {
            /* Handle user@host format */
            char *at = strchr(argv[i], '@');
            if (at)
            {
                *at = '\0';
                user = argv[i];
                host = at + 1;
            }
            else
            {
                host = argv[i];
            }
        }
        else if (!command)
        {
            /* Remaining args form the command */
            command = argv[i];
        }
    }

    if (!host)
    {
        Printf("AROS SSH Client\n");
        Printf("Usage: ssh [-i keyfile] [-p port] [-l user] [user@]host [command]\n");
        Printf("       ssh [-get remotefile] host\n");
        Printf("       ssh [-put localfile] host\n");
        return RETURN_WARN;
    }

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!SocketBase)
    {
        Printf("ssh: Cannot open bsdsocket.library v4\n");
        return RETURN_FAIL;
    }

    ssh_init();
    ssh_session session = ssh_new();
    if (!session)
    {
        Printf("ssh: Cannot create session\n");
        ssh_finalize();
        CloseLibrary(SocketBase);
        return RETURN_FAIL;
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, host);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, user);

    /* Connect */
    if (ssh_connect(session) != SSH_OK)
    {
        Printf("ssh: Connection to %s:%ld failed: %s\n",
               (IPTR)host, (long)port, (IPTR)ssh_get_error(session));
        ssh_free(session);
        ssh_finalize();
        CloseLibrary(SocketBase);
        return RETURN_FAIL;
    }

    /* Verify host key */
    if (verify_known_host(session) != 0)
    {
        ssh_disconnect(session);
        ssh_free(session);
        ssh_finalize();
        CloseLibrary(SocketBase);
        return RETURN_FAIL;
    }

    /* Authenticate */
    if (authenticate(session, keyfile, user) != 0)
    {
        Printf("ssh: Authentication failed\n");
        ssh_disconnect(session);
        ssh_free(session);
        ssh_finalize();
        CloseLibrary(SocketBase);
        return RETURN_FAIL;
    }

    /* Handle SCP operations */
    if (scp_get_file)
    {
        exit_code = scp_get(session, scp_get_file);
        goto disconnect;
    }

    if (scp_put_file)
    {
        exit_code = scp_put(session, scp_put_file);
        goto disconnect;
    }

    /* Open channel */
    ssh_channel channel = ssh_channel_new(session);
    if (!channel || ssh_channel_open_session(channel) != SSH_OK)
    {
        Printf("ssh: Cannot open channel: %s\n", (IPTR)ssh_get_error(session));
        exit_code = RETURN_FAIL;
        goto disconnect;
    }

    if (command)
    {
        /* Single command execution */
        if (ssh_channel_request_exec(channel, command) != SSH_OK)
        {
            Printf("ssh: Exec failed: %s\n", (IPTR)ssh_get_error(session));
            exit_code = RETURN_FAIL;
        }
        else
        {
            char buf[BUF_SIZE];
            int nbytes;

            /* Read stdout */
            while ((nbytes = ssh_channel_read(channel, buf, sizeof(buf), 0)) > 0)
                Write(Output(), buf, nbytes);

            /* Read stderr */
            while ((nbytes = ssh_channel_read(channel, buf, sizeof(buf), 1)) > 0)
                Write(Output(), buf, nbytes);

            exit_code = ssh_channel_get_exit_status(channel);
        }
    }
    else
    {
        /* Interactive shell */
        ssh_channel_request_pty(channel);
        if (ssh_channel_request_shell(channel) != SSH_OK)
        {
            Printf("ssh: Shell request failed: %s\n", (IPTR)ssh_get_error(session));
            exit_code = RETURN_FAIL;
        }
        else
        {
            exit_code = interactive_shell(session, channel);
        }
    }

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);

disconnect:
    ssh_disconnect(session);
    ssh_free(session);
    ssh_finalize();
    CloseLibrary(SocketBase);

    return exit_code;
}
