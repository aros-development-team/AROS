# AROS SSH Suite

Secure Shell server, client, and key generation for AROS, built on libssh 0.12.0 + mbedTLS 3.6.6.

## Components

| Program | Description |
|---------|-------------|
| `sshd` | SSH server — accepts incoming connections on port 22 |
| `ssh` | SSH client — connect to remote hosts |
| `ssh-keygen` | Generate RSA key pairs for authentication |

## Quick Start

```
1> ssh-keygen
Generating RSA 2048-bit key pair...
Private key: ENVARC:SSH/id_rsa
Public key:  ENVARC:SSH/id_rsa.pub

1> sshd
sshd: listening on port 22

1> ssh root@192.168.1.100
Connected to 192.168.1.100.
```

## Configuration

All config lives in `ENVARC:SSH/`:

| File | Purpose | Created by |
|------|---------|-----------|
| `host_key` | Server RSA private key (PEM) | `sshd` on first run |
| `authorized_keys` | Allowed public keys (OpenSSH format, one per line) | User |
| `password` | SHA-256 hex hash for password auth (optional) | User |
| `id_rsa` | Client private key (PEM) | `ssh-keygen` |
| `id_rsa.pub` | Client public key (OpenSSH format) | `ssh-keygen` |
| `known_hosts` | Trusted server fingerprints (SHA256) | `ssh` on first connect |

## Authentication

### Server (sshd)

1. **Public key** (preferred): Client presents a key listed in `ENVARC:SSH/authorized_keys`
2. **Password** (fallback): Only enabled if `ENVARC:SSH/password` exists. File contains a 64-char lowercase SHA-256 hex hash.

Username is always `root` — AROS is a single-user system.

To create a password hash:
```
1> echo -n "mypassword" | sha256sum > ENVARC:SSH/password
```

### Client (ssh)

1. Tries private key from `-i` flag or `ENVARC:SSH/id_rsa`
2. Falls back to password prompt (no echo)

## Usage

### ssh

```
ssh [-i keyfile] [-p port] [user@]host [command]
ssh -get remotefile              # Download file via SCP
ssh -put localfile               # Upload file via SCP
```

Examples:
```
1> ssh 192.168.1.1                    ; Interactive shell
1> ssh admin@server ls -la            ; Run command
1> ssh -i ENVARC:SSH/other_key host   ; Specific key
1> ssh -get server:/path/file         ; Download
1> ssh -put myfile server:/path/      ; Upload
```

### sshd

```
sshd                                  ; Start on port 22 (foreground)
run >NIL: sshd                        ; Start as background daemon
```

Shutdown: send CTRL-C or `Break` the sshd process.

### ssh-keygen

```
ssh-keygen                            ; RSA 2048, default paths
ssh-keygen -t rsa -f SYS:mykey       ; Custom output path
```

## Architecture

```
+-------------+     +--------------+     +--------------+
|   ssh (C:)  |     |  sshd (C:)   |     | ssh-keygen   |
+------+------+     +------+-------+     +------+-------+
       |                   |                     |
       v                   v                     v
+------------------------------------------------------+
|                   libssh 0.12.0                      |
|        (session, channel, auth, scp, ed25519)        |
+----------------------------+-------------------------+
                             |
                             v
+------------------------------------------------------+
|                   mbedTLS 3.6.6                      |
|          (TLS, RSA, ECDH, SHA-256, AES)              |
+----------------------------+-------------------------+
                             |
                             v
+------------------------------------------------------+
|                bsdsocket.library                     |
|                (TCP/IP via AROSTCP)                   |
+------------------------------------------------------+

sshd shell session:

+----------+    PIPE:ssh_in_N     +----------+
| SSH      | ------------------->  | C:Shell  |
| Channel  |    PIPE:ssh_out_N    |          |
|          | <-------------------  |          |
+----------+                      +----------+
```

## Security Notes

- Host key: RSA 2048 or Ed25519, auto-generated on first `sshd` run
- Password stored as SHA-256 hash, never plaintext
- Known hosts verified by SHA-256 fingerprint
- No root/privilege escalation (AROS has no privilege levels)
- All keys stored in `ENVARC:` (persistent, survives reboot)

## Building

```
make workbench-network-sshd workbench-network-ssh workbench-network-ssh-keygen
```

Requires: `workbench-libs-libssh` and `workbench-libs-mbedtls` built first.

## Limitations

- No SSH agent forwarding
- No X11 forwarding
- No port forwarding / tunneling
- No SFTP subsystem (use SCP for file transfer)
