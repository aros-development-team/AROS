#ifndef _LIBRARIES_STDCIO_H
#define _LIBRARIES_STDCIO_H

/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    Public part of StdCIO libbase.
    Take care of backwards compatibility when changing something in this file.
*/

#include <exec/libraries.h>
#include <libraries/stdc.h>

#define _STDIO_H_NOMACRO
#include <stdio.h>

struct StdCIOBase
{
    struct Library lib;
    struct StdCBase *StdCBase;

    FILE *_stdin, *_stdout, *_stderr;
};

__BEGIN_DECLS

struct StdCIOBase *__aros_getbase_StdCIOBase(void);

__END_DECLS

#endif /* _LIBRARIES_STDCIO_H */
