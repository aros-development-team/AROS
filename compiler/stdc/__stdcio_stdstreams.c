/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    Get pointer to standard IO streams.
    These function is both in static linklib as in stdcio.library.
*/
#include <libraries/stdcio.h>

#include <stdio.h>

FILE *__stdio_getstdin(void)
{
    return __aros_getbase_StdCIOBase()->_stdin;
}

FILE *__stdio_getstdout(void)
{
    return __aros_getbase_StdCIOBase()->_stdout;
}

FILE *__stdio_getstderr(void)
{
    return __aros_getbase_StdCIOBase()->_stderr;
}
