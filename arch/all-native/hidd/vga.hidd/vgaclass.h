#ifndef HIDD_VGACLASS_H
#define HIDD_VGACLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VGA useful data.
    Lang: English.
*/

#include <exec/interrupts.h>
#include <exec/types.h>
#include <exec/nodes.h>

/* I've tested additional modes on my ThinkPad 365x.
   The display appears either black or broken.
   Perhaps LCD controller doesn't recognise them.
   sonic */
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
    IPTR		HDisplay;
    IPTR		HSyncStart;
    IPTR		HSyncEnd;
    IPTR		HTotal;
    IPTR		HSkew;
    IPTR		VDisplay;
    IPTR		VSyncStart;
    IPTR		VSyncEnd;
    IPTR		VTotal;
};

struct vgaModeEntry
{
    struct MinNode	Node;
    ULONG		mode;		/* Mode desc */
    struct vgaModeDesc	*Desc;
};

struct Vga_Data
{
    struct Interrupt ResetInterrupt;
};

#endif /* HIDD_VGACLASS_H */
