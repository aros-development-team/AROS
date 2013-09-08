/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fopen().
*/
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <errno.h>
#include <aros/libcall.h>

#include "__stdio.h"
#include "__stdcio_intbase.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	FILE * fopen (

/*  SYNOPSIS */
	const char * restrict pathname,
	const char * restrict mode)

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

    EXAMPLE

    BUGS
        Currently errno is not set on error.

    SEE ALSO
	fclose(), fread(), fwrite(), fgets(), fgetc(), fputs(), fputc()

    INTERNALS

******************************************************************************/
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();
    int fhmode;
    char l2, hasplus;
    FILE *file = NULL;

    l2 = mode[1];
    if (l2 == 'b') l2 = mode[2];
    hasplus = (l2 == '+');

    if (!StdCIOBase->streampool)
        StdCIOBase->streampool = CreatePool(MEMF_ANY, 20*sizeof(FILE), 2*sizeof(FILE));
    if (!StdCIOBase->streampool)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        goto error;
    }
    file = AllocPooled(StdCIOBase->streampool, sizeof(FILE));
    if (!file)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        goto error;
    }
    file->fh = (BPTR)NULL;

    switch(mode[0])
    {
    case 'r':
        file->flags = __STDCIO_STDIO_READ;
        fhmode = MODE_OLDFILE;
        break;
    case 'w':
        file->flags = __STDCIO_STDIO_WRITE;
        fhmode = MODE_NEWFILE;
        break;
    case 'a':
        file->flags = __STDCIO_STDIO_WRITE|__STDCIO_STDIO_APPEND;
        fhmode = MODE_READWRITE;
        break;
    default:
        goto error;
    }
    if (hasplus)
        file->flags |= __STDCIO_STDIO_RDWR;

    file->fh = Open(pathname, fhmode);
    if (!file->fh)
        goto error;

    if (file->flags & __STDCIO_STDIO_APPEND)
    {
        if (Seek(file->fh, 0, OFFSET_END) < 0)
            goto error;
    }

    AddTail((struct List *)&StdCIOBase->files, (struct Node *)file);

    return file;

 error:
    if (file)
    {
        if (file->fh) Close(file->fh);
        FreePooled(StdCIOBase->streampool, file, sizeof(FILE));
    }
    errno = __stdc_ioerr2errno(IoErr());
    return NULL;
} /* fopen */
