#ifndef ___STDCIO_DOS64_H
#define ___STDCIO_DOS64_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Internal 64-bit seek helpers for stdcio.library (dos64.library when
    available, 32-bit dos.library fallback otherwise).
*/

#include <dos/dos.h>
#include <dos/dos64.h>

#include <stdio.h>

/* Detect 64-bit filesystem support for a freshly opened stream and
   cache it in the stream flags (__STDCIO_STDIO_FS64); the helpers
   below then pick the 64-bit or 32-bit path directly. */
void __stdcio_probe64(FILE *stream);

QUAD __stdcio_seek64(FILE *stream, QUAD position, LONG mode);
QUAD __stdcio_getpos64(FILE *stream);

#endif /* ___STDCIO_DOS64_H */
