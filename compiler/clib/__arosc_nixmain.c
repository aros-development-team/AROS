/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: special main function for code which has to use special *nix features.
          This function gets called from a function with a similar name statically
	  linked with the program. This is so to make the program not depend on a
	  particular libc version.

    Lang: english
*/

#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <aros/startup.h>
#include "__upath.h"

int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[])
{
    volatile char *old_argv0 = NULL;

    /* Trigger *nix path handling on.  */
    __doupath = 1;

    /* argv[0] usually contains the name of the program, possibly with the full
       path to it. Here we translate that path, which is an AmigaDOS-style path, into
       an unix-style one.  */
    if (argv && argv[0])
    {
        char *new_argv0;

	new_argv0 = strdup(__path_a2u(argv[0]));
	if (new_argv0 == NULL)
	    return EXIT_FAILURE;

        old_argv0 = argv[0];
	argv[0] = new_argv0;
    }

    /* Call the real main.  */
    if (setjmp(__aros_startup_jmp_buf) == 0)
    {
        __aros_startup_error = (*main)(argc, argv);
    }

    /* Restore the old argv[0].  */
    if (old_argv0)
    {
        free(argv[0]);
	argv[0] = old_argv0;
    }

    return __aros_startup_error;
}


