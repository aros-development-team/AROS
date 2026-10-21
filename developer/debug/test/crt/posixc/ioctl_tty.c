/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Pass --require for a console that implements the geometry protocol. */
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    const struct winsize sentinel = {1, 2, 3, 4};
    struct winsize w = sentinel;
    int fd = -1, dir = -1, failed = 0, error = 0, i, rc;
    int require = argc > 1 && strcmp(argv[1], "--require") == 0;
#define CHECK(x) do { if (!(x)) { failed = __LINE__; error = errno; goto done; } } while (0)
    CHECK(ioctl(-1, TIOCGWINSZ, &w) == -1 && errno == EBADF);
    CHECK(memcmp(&w, &sentinel, sizeof(w)) == 0);
    for (i = STDIN_FILENO; i <= STDOUT_FILENO; i++)
    {
        w = sentinel;
        rc = ioctl(i, TIOCGWINSZ, &w);
        if (rc == 0)
        {
            CHECK(w.ws_col && w.ws_row && w.ws_xpixel && w.ws_ypixel);
            CHECK(w.ws_xpixel >= w.ws_col && w.ws_ypixel >= w.ws_row);
            printf("GEOMETRY fd=%d columns=%u rows=%u pixels=%ux%u\n", i,
                w.ws_col, w.ws_row, w.ws_xpixel, w.ws_ypixel);
            CHECK(ioctl(i, TIOCGWINSZ, NULL) == -1 && errno == EFAULT);
        }
        else
        {
            CHECK(!require && rc == -1 && errno == ENOTTY);
            CHECK(memcmp(&w, &sentinel, sizeof(w)) == 0);
        }
    }
    w = sentinel;
    fd = open("RAM:ioctl-tty-test.tmp", O_CREAT | O_EXCL | O_RDWR, 0600);
    CHECK(fd >= 0);
    CHECK(ioctl(fd, TIOCGWINSZ, &w) == -1 && errno == ENOTTY);
    CHECK(ioctl(fd, 123) == -1 && errno == ENOTTY);
    CHECK(memcmp(&w, &sentinel, sizeof(w)) == 0);
    dir = open("RAM:", O_RDONLY);
    CHECK(dir >= 0);
    CHECK(ioctl(dir, TIOCGWINSZ, &w) == -1 && errno == ENOTTY);
    CHECK(memcmp(&w, &sentinel, sizeof(w)) == 0);
done:
    if (dir >= 0 && close(dir)) { failed = __LINE__; error = errno; }
    if (fd >= 0)
    {
        if (close(fd)) { failed = __LINE__; error = errno; }
        if (unlink("RAM:ioctl-tty-test.tmp")) { failed = __LINE__; error = errno; }
    }
    if (failed) printf("IOCTL TTY FAIL line=%d errno=%d\n", failed, error);
    else puts("IOCTL TTY PASS: geometry, descriptor errors, unchanged output on failure");
    return failed ? 20 : 0;
}
