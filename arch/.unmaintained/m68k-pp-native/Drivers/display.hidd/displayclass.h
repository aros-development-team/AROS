#ifndef HIDD_DISPLAYCLASS_H
#define HIDD_DISPLAYCLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some Display useful data.
    Lang: English.
*/

#include <exec/types.h>
#include <exec/nodes.h>

#define ONLY640 1

#ifdef ONLY640
#define NUM_MODES 1
#else
#define NUM_MODES 3
#endif

struct DisplayModeDesc
{
    char		*name;		/* Mode name */
    UWORD		Width;
    UWORD		Height;
    UBYTE		Depth;		/* BitsPerPixel */
    UBYTE		clock;		/* PixelClock used */
    ULONG		Flags;		/* Misc Flags */
    UWORD		HDisplay;
    UWORD		HSyncStart;
    UWORD		HSyncEnd;
    UWORD		HTotal;
    UWORD		HSkew;
    UWORD		VDisplay;
    UWORD		VSyncStart;
    UWORD		VSyncEnd;
    UWORD		VTotal;
};

struct DisplayModeEntry
{
    struct MinNode	Node;
    ULONG		mode;		/* Mode desc */
    struct DisplayModeDesc	*Desc;
};

#endif /* HIDD_DISPLAYCLASS_H */
