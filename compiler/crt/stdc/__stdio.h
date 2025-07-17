#ifndef ___STDIO_H
#define ___STDIO_H

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: internal header file for stdio
    Lang: English
*/

#include <dos/dos.h>
#include <aros/types/file_s.h>

#define __STDCIO_STDIO_EOF          0x0001L
#define __STDCIO_STDIO_ERROR        0x0002L
#define __STDCIO_STDIO_READ         0x0004L
#define __STDCIO_STDIO_WRITE        0x0008L
#define __STDCIO_STDIO_RDWR         __STDCIO_STDIO_WRITE | __STDCIO_STDIO_READ
#define __STDCIO_STDIO_APPEND       0x0010
#define __STDCIO_STDIO_TMP          0x0020L
#define __STDCIO_STDIO_DONTCLOSE    0x0040L
#define __STDCIO_STDIO_DONTFREE     0x0080L
#define __STDCIO_STDIO_FLUSHONREAD  0x0100L
#define __STDCIO_STDIO_WIDE         0x0200L   /* Stream is wide-oriented */
#define __STDCIO_STDIO_UNGETWC      0x0400L   /* ungetwc_char contains a pushed-back wide char */

#endif /* ___STDIO_H */

