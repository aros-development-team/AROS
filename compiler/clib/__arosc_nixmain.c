/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: special main function for code which has to use special *nix features.
          This function gets called from a function with a similar name statically
	  linked with the program. This is so to make the program not depend on a
	  particular libc version.

    Lang: english
*/

int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[])
{
    /* Trigger *nix path handling on.  */
    __doupath = 1;

    /* Call the real main.  */
    return (*main)(argc, argv);
}


