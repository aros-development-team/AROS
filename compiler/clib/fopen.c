/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function fopen()
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <errno.h>
#include <unistd.h>
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

    HISTORY
	17.01.97 digulla created

******************************************************************************/
{
    FILENODE * fn;
    int        fd;
    int        openmode;

    while (*mode)
    {
	switch (*mode)
	{
	case 'r': openmode = O_RDONLY; break;
	case 'w': openmode = O_WRONLY | O_CREAT; break;
	case 'a': openmode = O_WRONLY | O_APPEND | O_CREAT; break;
	case '+':
	    openmode = O_RDWR | (openmode & ~O_ACCMODE);
	    break;

	default:
	    /* ignore */
	    break;
	}

	mode ++;
    }

    if ((fd = open (pathname, openmode, 644)) == -1)
	return NULL;

    fn = (FILENODE *)GetTail (&__stdio_files);

    return FILENODE2FILE(fn);
} /* fopen */

