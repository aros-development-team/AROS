/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Get pointer to standard IO streams.
    These function is both in static linklib as in posixc.library.
*/
#include <libraries/posixc.h>

FILE *__stdio_getstdin(void)
{
    return __aros_getbase_PosixCBase()->_stdin;
}

FILE *__stdio_getstdout(void)
{
    return __aros_getbase_PosixCBase()->_stdout;
}

FILE *__stdio_getstderr(void)
{
    return __aros_getbase_PosixCBase()->_stderr;
}
