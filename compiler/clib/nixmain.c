/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: special main function for code which has to use special *nix features
    Lang: english
*/

extern int main(int argc, char *argv[]);

int __nixmain(int argc, char *argv[])
{
    return main(argc, argv);
}

int (*__main_function_ptr)(int argc, char *argv[]) = __nixmain;

