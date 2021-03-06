/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.

    C99 function fopen().
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <errno.h>
#include <aros/libcall.h>

#include "__stdio.h"
#include "__stdcio_intbase.h"

#include "debug.h"

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
    FILE *file = NULL;
    int fhmode;
    char l2, hasplus;

    D(bug("[%s] %s(0x%p, 0x%p)\n", STDCNAME, __func__, pathname, mode));

    l2 = mode[1];
    if (l2 == 'b') l2 = mode[2];
    hasplus = (l2 == '+');

    if (!StdCIOBase->streampool)
    {
        StdCIOBase->streampool = CreatePool(MEMF_ANY, 20*sizeof(FILE), 2*sizeof(FILE));
        D(bug("[%s] %s: CreatePool streampool @ 0x%p\n", STDCNAME, __func__, StdCIOBase->streampool));
    }
    if (!StdCIOBase->streampool)
    {
        D(bug("[%s] %s: failed to alloc streampool\n", STDCNAME, __func__));
        SetIoErr(ERROR_NO_FREE_STORE);
        goto error;
    }
    file = AllocPooled(StdCIOBase->streampool, sizeof(FILE));
    D(bug("[%s] %s: file @ 0x%p (%d bytes)\n", STDCNAME, __func__, file, sizeof(FILE)));
    if (!file)
    {
        D(bug("[%s] %s: failed to alloc file storage\n", STDCNAME, __func__));
        SetIoErr(ERROR_NO_FREE_STORE);
        goto error;
    }
    file->fh = (BPTR)NULL;

    switch(mode[0])
    {
    case 'r':
        file->flags = __STDCIO_STDIO_READ;
        fhmode = MODE_OLDFILE;
        D(bug("[%s] %s: open mode = READ (using MODE_OLDFILE)\n", STDCNAME, __func__));
        break;
    case 'w':
        file->flags = __STDCIO_STDIO_WRITE;
        D(bug("[%s] %s: open mode = WRITE (using MODE_NEWFILE)\n", STDCNAME, __func__));
        fhmode = MODE_NEWFILE;
        break;
    case 'a':
        file->flags = __STDCIO_STDIO_WRITE|__STDCIO_STDIO_APPEND;
        D(bug("[%s] %s: open mode = APPEND (using MODE_READWRITE)\n", STDCNAME, __func__));
        fhmode = MODE_READWRITE;
        break;
    default:
        goto error;
    }
    if (hasplus)
        file->flags |= __STDCIO_STDIO_RDWR;

    file->fh = Open(pathname, fhmode);
    D(bug("[%s] %s: Open fh = 0x%p\n", STDCNAME, __func__, file->fh));
    if (!file->fh)
        goto error;

    if (file->flags & __STDCIO_STDIO_APPEND)
    {
        D(bug("[%s] %s: seeking to file end ...\n", STDCNAME, __func__));
        if (Seek(file->fh, 0, OFFSET_END) < 0)
            goto error;
    }

    AddTail((struct List *)&StdCIOBase->files, (struct Node *)file);

    D(bug("[%s] %s: returning 0x%p\n", STDCNAME, __func__, file));

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
