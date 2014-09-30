/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    LONG ret;
    LONG arg;

    if (argc < 3)
    {
    	printf("Usage: %s <device> [ON|OFF]\n", argv[0]);
    	return 0;
    }

    arg = !stricmp(argv[2], "ON");
    ret = Inhibit(argv[1], arg);

    printf("Inhibit(%d) returned %d\n", (int)arg, (int)ret);

    return 0;
}
