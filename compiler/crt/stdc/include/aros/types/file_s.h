#ifndef _AROS_TYPES_FILE_S_H
#define _AROS_TYPES_FILE_S_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: __sFILE struct definition
*/

#include <aros/types/wint_t.h>
#include <aros/types/mbstate_t.h>
#include <dos/bptr.h>
#include <exec/lists.h>

struct __sFILE
{
    struct MinNode  node;
    union {
        BPTR fh;
        int fd;
    };
    ULONG           fhFlags;
    int             flags;

    mbstate_t       mbs;        /* multibyte conversion state */
    wint_t          unget_wc;   /* pushed-back wide char (if __STDCIO_STDIO_UNGETWC set) */
};

#endif /* _AROS_TYPES_FILE_S_H */
