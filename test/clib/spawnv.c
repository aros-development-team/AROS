/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Test program for the libc's spawnv() function.
    Lang: English
*/

#include <process.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int ret;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <path to command> <command's arguments>\n", argv[0]);
        return 20;
    }

    if ((ret = spawnv(P_WAIT, argv[1], &argv[1])) == -1)
    {
        perror(argv[1]);
	return 20;
    }

    return ret;
}
