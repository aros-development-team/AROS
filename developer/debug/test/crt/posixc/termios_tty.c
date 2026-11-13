/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Run with interactive DOS stdin. Tests ICANON compatibility, not full termios. */
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct termios t, before, saved;
    const int actions[] = {TCSANOW, TCSADRAIN, TCSAFLUSH};
    int fd = -1, copy = -1, changed = 0, failed = 0, error = 0;
    unsigned i;
#define CHECK(x) do { if (!(x)) { failed = __LINE__; error = errno; goto done; } } while (0)
    memset(&t, 0x55, sizeof t); before = t;
    CHECK(tcgetattr(-1, &t) == -1 && errno == EBADF);
    CHECK(tcsetattr(-1, TCSANOW, &t) == -1 && errno == EBADF);
    fd = open("RAM:termios-tty-test.tmp", O_CREAT | O_EXCL | O_RDWR, 0600);
    CHECK(fd >= 0);
    CHECK(tcgetattr(fd, &t) == -1 && errno == ENOTTY && !memcmp(&t, &before, sizeof t));
    CHECK(tcsetattr(fd, TCSANOW, &t) == -1 && errno == ENOTTY);
    CHECK(tcgetattr(STDIN_FILENO, NULL) == -1 && errno == EFAULT);
    CHECK(tcsetattr(STDIN_FILENO, TCSANOW, NULL) == -1 && errno == EFAULT);
    memset(&saved, 0, sizeof saved);
    CHECK(tcgetattr(STDIN_FILENO, &saved) == 0);
    CHECK(tcsetattr(STDIN_FILENO, 99, &saved) == -1 && errno == EINVAL);
    copy = dup(STDIN_FILENO); CHECK(copy >= 0);
    for (i = 0; i < sizeof actions / sizeof actions[0]; ++i)
    {
        t = saved; t.c_lflag &= ~ICANON;
        CHECK(tcsetattr(copy, actions[i], &t) == 0); changed = 1;
        t.c_lflag = ICANON;
        CHECK(tcgetattr(STDIN_FILENO, &t) == 0 && !(t.c_lflag & ICANON));
        CHECK(tcsetattr(STDIN_FILENO, actions[i], &saved) == 0); changed = 0;
        CHECK(tcgetattr(copy, &t) == 0 && (t.c_lflag & ICANON) == (saved.c_lflag & ICANON));
    }
done:
    if (changed && tcsetattr(STDIN_FILENO, TCSANOW, &saved)) { failed = __LINE__; error = errno; }
    if (copy >= 0 && close(copy)) { failed = __LINE__; error = errno; }
    if (fd >= 0)
    {
        if (close(fd)) { failed = __LINE__; error = errno; }
        if (unlink("RAM:termios-tty-test.tmp")) { failed = __LINE__; error = errno; }
    }
    if (failed) printf("TERMIOS TTY FAIL line=%d errno=%d\n", failed, error);
    else puts("TERMIOS TTY PASS: errors, all legacy actions, dup ICANON roundtrips");
    return failed ? 20 : 0;
}
