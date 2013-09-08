/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function getchar().
*/
#include <libraries/stdcio.h>

#define _STDIO_H_NOMACRO

/*****************************************************************************

    NAME */
#include <stdio.h>

	int getchar (void)

/*  SYNOPSIS */

/*  FUNCTION
	Read one character from the standard input stream. If there
        is no character available or an error occurred, the function
        returns EOF.

    INPUTS

    RESULT
	The character read or EOF on end of file or error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fgetc(), getc(), fputc(), putc()

    INTERNALS

******************************************************************************/
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    return fgetc(StdCIOBase->_stdin);
} /* getc */
