/*
    Copyright (C) 2009, The AROS Development Team. All rights reserved.

    Desc: Test program for the libc's system() function.
*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int ret;

    if (argc < 2)
    {
        fprintf(stderr,
                "Usage: %s <command string>\n"
                "  only first command is used, quote command if it contains spaces\n",
                argv[0]
        );
        return 20;
    }

    ret = system(argv[1]);
    if (ret == -1)
    {
        perror(argv[1]);
        return 20;
    }

    return ret;
}
