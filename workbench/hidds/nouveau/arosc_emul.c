/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

/* Workaround for standard streams being bound DOS files of first opener of library */

#include <unistd.h>

struct hackFILE
{
    int fd;
    int flags;
};

typedef struct hackFILE FILE;

static int __putc(int c, void *fh);

FILE *__stdio_getstderr(void)
{
    static struct hackFILE tmp;
    tmp.fd = -STDERR_FILENO;
    return &tmp;
}
