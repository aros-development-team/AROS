/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: special main function for code which has to use special *nix features.
          This function gets called from a function with a similar name statically
	  linked with the program. This is so to make the program independent from the
	  libc's version.

    Lang: english
*/

#include <sys/syscall.h>

int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[])
{
    return (*main)(argc, argv);
}


