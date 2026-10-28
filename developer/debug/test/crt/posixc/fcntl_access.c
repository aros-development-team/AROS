/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    const int modes[] = { O_RDONLY, O_WRONLY, O_RDWR };
    unsigned int i;
    int failed = 0;

    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i)
    {
        int fd = open("NIL:", modes[i]);
        int copy;
        int flags;

        if (fd < 0)
        {
            perror("open NIL:");
            return 20;
        }
        copy = dup(fd);
        flags = fcntl(fd, F_GETFL);
        if (copy < 0 || flags < 0 || (flags & O_ACCMODE) != modes[i] ||
            (fcntl(copy, F_GETFL) & O_ACCMODE) != modes[i])
        {
            fprintf(stderr, "F_GETFL: mode=%d flags=%d dup=%d\n",
                modes[i], flags, copy);
            failed = 1;
        }
        if (copy >= 0 && close(copy) != 0)
            failed = 1;
        if (close(fd) != 0)
            failed = 1;
    }
    return failed ? 20 : 0;
}
