/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getscreenmode.gadget - Internal definitions
*/

#ifndef GETSCREENMODE_INTERN_H
#define GETSCREENMODE_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/getscreenmode.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#define G(obj)  ((struct Gadget *)(obj))

struct GetScreenModeData
{
    STRPTR          titletext;      /* Title text for requester */
    ULONG           displayid;      /* Display mode ID */
    ULONG           displaywidth;   /* Display width */
    ULONG           displayheight;  /* Display height */
    ULONG           displaydepth;   /* Display depth */
    ULONG           overscantype;   /* Overscan type */
    BOOL            autoscroll;     /* AutoScroll enabled */
    BOOL            dowidth;        /* Allow width selection */
    BOOL            doheight;       /* Allow height selection */
    BOOL            dodepth;        /* Allow depth selection */
    BOOL            dooverscan;     /* Allow overscan selection */
    BOOL            doautoscroll;   /* Allow autoscroll selection */
};

#endif /* GETSCREENMODE_INTERN_H */
