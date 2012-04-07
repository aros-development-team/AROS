/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fopen().
*/

#include <fcntl.h>
#include "__stdio.h"


/*****************************************************************************

    NAME */
#include <stdio.h>

	FILE * fopen (

/*  SYNOPSIS */
	const char * pathname,
	const char * mode)

/*  FUNCTION
	Opens a file with the specified name in the specified mode.

    INPUTS
	pathname - Path and filename of the file you want to open.
	mode - How to open the file:

		r: Open for reading. The stream is positioned at the
			beginning of the file.

		r+: Open for reading and writing. The stream is positioned
			at the beginning of the file.

		w: Open for writing. If the file doesn't exist, then
			it is created. If it does already exist, then
			it is truncated. The stream is positioned at the
			beginning of the file.

		w+: Open for reading and writing. If the file doesn't
			exist, then it is created. If it does already
			exist, then it is truncated. The stream is
			positioned at the beginning of the file.

		a: Open for writing. If the file doesn't exist, then
			it is created. The stream is positioned at the
			end of the file.

		a+: Open for reading and writing. If the file doesn't
			exist, then it is created. The stream is positioned
			at the end of the file.

		b: Open in binary more. This has no effect and is ignored.

    RESULT
	A pointer to a FILE handle or NULL in case of an error. When NULL
	is returned, then errno is set to indicate the error.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS
	Most modes are not supported right now.

    SEE ALSO
	fclose(), fread(), fwrite(), open(), fgets(), fgetc(),
	fputs(), fputc(), getc(), putc()

    INTERNALS

******************************************************************************/
{
    int fd;
    int openmode = __smode2oflags(mode);

    if (openmode != -1)
    {
        fd = open(pathname, openmode, 644);
        if (fd == -1)
            return NULL;
    
        return fdopen(fd, NULL);
    }
    else
    {
        return NULL;
    }
} /* fopen */

