/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: special main function for code which has to use special *nix features
    Lang: english
*/

#include <proto/posixc.h>

extern int main(int argc, char *argv[]);

/* FIXME: This solution for -nix flag is now specific for posixc.library
   Could a more general approach be provided so other libs can add their own
   flags without needing to do something in compiler/startup
   Is this wanted ?
 */

int __nixmain(int argc, char *argv[])
{
    return __posixc_nixmain(main, argc, argv);
}

int (*__main_function_ptr)(int argc, char *argv[]) = __nixmain;

