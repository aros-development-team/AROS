/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - global variables for the startup code
*/
#include <exec/types.h>

/* pass these values to the command line handling function */
char *__argstr;
ULONG __argsize;

/* the command line handling functions will pass these values back to us */
char **__argv;
int  __argc;

/* Code can set this value to indicate the return value of the program */
LONG __startup_error;
