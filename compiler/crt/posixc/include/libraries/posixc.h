#ifndef _LIBRARIES_POSIXC_H
#define _LIBRARIES_POSIXC_H

/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    Public part of PosixC libbase.
    Take care of backwards compatibility when changing something in this file.
*/

#include <exec/libraries.h>
#include <libraries/stdc.h>
#include <libraries/stdcio.h>

#define _STDIO_H_NOMACRO
#include <stdio.h>

struct PosixCBase
{
    struct Library lib;
    struct StdCBase *StdCBase;
    struct StdCIOBase *StdCIOBase;

    FILE *_stdin, *_stdout, *_stderr;
};

struct PosixCBase *__aros_getbase_PosixCBase(void);

#endif /* _LIBRARIES_POSIXC_H */
