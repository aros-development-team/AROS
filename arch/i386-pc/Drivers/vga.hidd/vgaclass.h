#ifndef HIDD_VGACLASS_H
#define HIDD_VGACLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VGA useful data.
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

struct vgaModeDesc
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

struct vgaModeEntry
{
    struct MinNode	Node;
    ULONG		mode;		/* Mode desc */
    struct vgaModeDesc	*Desc;
};

#endif /* HIDD_VGACLASS_H */
