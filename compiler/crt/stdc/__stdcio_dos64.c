/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Internal 64-bit seek helpers for stdcio.library: use dos64.library
    when it is available, otherwise fall back to the 32-bit
    dos.library calls. Positions the fallback cannot express fail with
    IoErr() == ERROR_OBJECT_TOO_LARGE.
*/

#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "__stdcio_intbase.h"
#include "__stdio.h"
#include "__stdcio_dos64.h"

#include <proto/dos64.h>

static struct Library *__stdcio_dos64base(void)
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase;

    if (StdCIOBase->StdCIODOS64Base == NULL)
        StdCIOBase->StdCIODOS64Base = OpenLibrary("dos64.library", 50);

    return StdCIOBase->StdCIODOS64Base;
}

/* Detect 64-bit filesystem support once per stream and cache it */
void __stdcio_probe64(FILE *stream)
{
    struct Library *DOS64Base = __stdcio_dos64base();

    if (DOS64Base != NULL && stream->fh != BNULL
        && IsFileSystem64(stream->fh))
        stream->flags |= __STDCIO_STDIO_FS64;
}

static struct Library *__stdcio_dos64base_for(FILE *stream)
{
    if (!(stream->flags & __STDCIO_STDIO_FS64))
        return NULL;

    return __stdcio_dos64base();
}

QUAD __stdcio_seek64(FILE *stream, QUAD position, LONG mode)
{
    struct Library *DOS64Base = __stdcio_dos64base_for(stream);

    if (DOS64Base != NULL)
        return Seek64(stream->fh, mode, position);

    if (position != (QUAD)(LONG)position)
    {
        SetIoErr(ERROR_OBJECT_TOO_LARGE);
        return -1;
    }
    return Seek(stream->fh, (LONG)position, mode);
}

QUAD __stdcio_getpos64(FILE *stream)
{
    struct Library *DOS64Base = __stdcio_dos64base_for(stream);

    if (DOS64Base != NULL)
        return GetFilePosition64(stream->fh);

    return Seek(stream->fh, 0, OFFSET_CURRENT);
}

static int __close_stdcio_dos64(struct StdCIOIntBase *StdCIOBase)
{
    if (StdCIOBase->StdCIODOS64Base != NULL)
    {
        CloseLibrary(StdCIOBase->StdCIODOS64Base);
        StdCIOBase->StdCIODOS64Base = NULL;
    }
    return 1;
}

ADD2CLOSELIB(__close_stdcio_dos64, 0)
