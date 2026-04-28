/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Internal header for scroller.gadget
*/

#ifndef SCROLLER_INTERN_H
#define SCROLLER_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/scroller.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#define G(obj) ((struct Gadget *)(obj))

struct ScrollerData
{
    ULONG total;        /* Total number of units in the scrollable content  */
    ULONG visible;      /* Number of units visible at once                  */
    ULONG top;          /* Index of the first visible unit                  */
    ULONG orientation;  /* SORIENT_VERT or SORIENT_HORIZ                   */
    BOOL  arrows;       /* Whether to display arrow buttons at the ends     */
    ULONG arrowdelta;   /* Units to scroll when an arrow button is pressed  */
};

#endif /* SCROLLER_INTERN_H */
