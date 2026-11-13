#ifndef AROS_PTY_H
#define AROS_PTY_H

/*
    A pseudo console for Rust on AROS.

    A DOS packet handler process acts as the console of a shell: the shell
    and everything it runs read and write it like a CON: window, and the
    terminal emulator holds the other side. Output reaches the terminal as
    UTF-8 with ANSI escapes; terminal input is ISO-8859-1 to the programs.
    A program that speaks xterm itself sets SetMode(fh, 2): then nothing is
    translated either way until it sets another mode. A raw mode program
    that writes a DEC private sequence (CSI ?) is taken for an xterm one
    too: it gets xterm keys and no newline translation or breaks, until the
    console is cooked again.

    The Rust half is ../src/lib.rs.
*/

#include <stdint.h>

struct aros_pty;

/* Starts the handler and runs `command` in the user shell, or the shell
   itself when `command` is NULL or empty. Returns NULL on failure. */
struct aros_pty *aros_pty_spawn(const char *command, int32_t cols, int32_t rows);

/* Blocks until output is available. Returns the byte count, or 0 once the
   shell has closed every handle and all output was read. */
int32_t aros_pty_read(struct aros_pty *pty, uint8_t *buf, int32_t len);

/* Queues UTF-8 terminal input for the console. Never blocks. */
void aros_pty_write(struct aros_pty *pty, const uint8_t *buf, int32_t len);

void aros_pty_resize(struct aros_pty *pty, int32_t cols, int32_t rows);

/* Sends EOF and a break to the shell, then waits until every handle on
   the console is closed and the handler is gone. */
void aros_pty_close(struct aros_pty *pty);

/* Frees the console. Only after close, and once no thread is in read. */
void aros_pty_free(struct aros_pty *pty);

#endif /* AROS_PTY_H */
