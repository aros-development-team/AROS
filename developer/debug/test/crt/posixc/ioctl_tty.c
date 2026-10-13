/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* DOS geometry is intentionally unsupported until a public tty ABI exists. */
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int main(void)
{
    struct winsize w = {1, 2, 3, 4};
    int fd = -1, failed = 0, error = 0;
#define CHECK(x) do { if (!(x)) { failed = __LINE__; error = errno; goto done; } } while (0)
    CHECK(ioctl(-1, TIOCGWINSZ, &w) == -1 && errno == EBADF);
    CHECK(ioctl(STDIN_FILENO, TIOCGWINSZ, &w) == -1 && errno == ENOTTY);
    CHECK(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 && errno == ENOTTY);
    CHECK(ioctl(STDIN_FILENO, TIOCGWINSZ, NULL) == -1 && errno == ENOTTY);
    fd = open("RAM:ioctl-tty-test.tmp", O_CREAT | O_EXCL | O_RDWR, 0600);
    CHECK(fd >= 0);
    CHECK(ioctl(fd, TIOCGWINSZ, &w) == -1 && errno == ENOTTY);
    CHECK(ioctl(fd, 123, &w) == -1 && errno == ENOTTY);
    CHECK(w.ws_row == 1 && w.ws_col == 2 && w.ws_xpixel == 3 && w.ws_ypixel == 4);
done:
    if (fd >= 0)
    {
        if (close(fd)) { failed = __LINE__; error = errno; }
        if (unlink("RAM:ioctl-tty-test.tmp")) { failed = __LINE__; error = errno; }
    }
    if (failed) printf("IOCTL TTY FAIL line=%d errno=%d\n", failed, error);
    else puts("IOCTL TTY PASS: descriptor errors, unsupported geometry, unchanged output");
    return failed ? 20 : 0;
}
