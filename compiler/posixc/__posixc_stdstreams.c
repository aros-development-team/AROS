/*
    Copyright (C) 2012-2023, The AROS Development Team. All rights reserved.

    Get pointer to standard IO streams.
    These function is both in static linklib as in posixc.library.
*/
#include <libraries/posixc.h>

FILE *__posixc_getstdin(void)
{
    return __aros_getbase_PosixCBase()->_stdin;
}

FILE *__posixc_getstdout(void)
{
    return __aros_getbase_PosixCBase()->_stdout;
}

FILE *__posixc_getstderr(void)
{
    return __aros_getbase_PosixCBase()->_stderr;
}
