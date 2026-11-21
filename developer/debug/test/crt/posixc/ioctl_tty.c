/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ioctl(TIOCGWINSZ): descriptor errors, a file is no terminal, and the
    size of a console window in cooked and raw mode, asked through another
    descriptor of the same console, without changing its mode or input.
*/

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>

#include <proto/dos.h>

static int failures;

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static void console(const char *path, int width, int height, int raw)
{
    struct winsize w = { 0, 0, 0, 0 };
    struct termios t;
    long fh = 0;
    int fd = open(path, O_RDWR), other;

    if (fd < 0)
    {
        printf("FAIL: open %s errno=%d\n", path, errno);
        failures++;
        return;
    }
    if (raw)
    {
        tcgetattr(fd, &t);
        t.c_lflag &= ~ICANON;
        tcsetattr(fd, TCSANOW, &t);
    }
    /* the size is asked through a second descriptor, as a program that set
       stdin raw and asks stdout does */
    other = dup(fd);
    check(other >= 0 && ioctl(other, TIOCGWINSZ, &w) == 0, raw ? "raw console: TIOCGWINSZ" : "cooked console: TIOCGWINSZ");
    check(w.ws_row > 0 && w.ws_col > 0, "rows and columns are known");
    check(w.ws_xpixel > 0 && w.ws_xpixel <= width && w.ws_ypixel > 0 && w.ws_ypixel <= height,
          "the pixel size fits the window");
    tcgetattr(fd, &t);
    check(!(t.c_lflag & ICANON) == !!raw, "the mode is as it was");
    __get_default_file(fd, &fh);
    check(fh && !WaitForChar((BPTR)fh, 200000), "nothing became readable");
    printf("  %s: %d rows, %d columns, %dx%d pixels\n", path, w.ws_row, w.ws_col, w.ws_xpixel, w.ws_ypixel);
    if (other >= 0)
        close(other);
    close(fd);
}

int main(void)
{
    struct winsize w = { 1, 2, 3, 4 };
    int fd;

    check(ioctl(-1, TIOCGWINSZ, &w) == -1 && errno == EBADF, "a bad descriptor gives EBADF");
    fd = open("RAM:ioctl-tty-test.tmp", O_CREAT | O_EXCL | O_RDWR, 0600);
    check(fd >= 0, "a file opened");
    if (fd >= 0)
    {
        check(ioctl(fd, TIOCGWINSZ, &w) == -1 && errno == ENOTTY, "a file gives ENOTTY");
        check(ioctl(fd, 123, &w) == -1 && errno == ENOTTY, "an unknown request gives ENOTTY");
        check(ioctl(fd, TIOCGWINSZ, NULL) == -1 && errno == EFAULT, "no structure gives EFAULT");
        check(w.ws_row == 1 && w.ws_col == 2 && w.ws_xpixel == 3 && w.ws_ypixel == 4, "a failed query leaves the structure alone");
        close(fd);
        unlink("RAM:ioctl-tty-test.tmp");
    }
    console("CON:0/0/640/200/ioctl_tty/CLOSE", 640, 200, 0);
    console("CON:0/0/320/120/ioctl_tty raw/CLOSE", 320, 120, 1);

    printf("IOCTL TTY %s\n", failures ? "FAIL" : "PASS");
    return failures ? 20 : 0;
}
