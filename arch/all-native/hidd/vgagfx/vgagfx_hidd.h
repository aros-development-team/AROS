#ifndef VGAGFX_HIDD_H
#define VGAGFX_HIDD_H

/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VGA Gfx Hidd data.
    Lang: English.
*/

#include <exec/interrupts.h>
#include <exec/types.h>
#include <exec/nodes.h>

/***** VGA gfx HIDD *******************/

/* IDs */
#define IID_Hidd_Gfx_VGA		"hidd.gfx.vga"
#define CLID_Hidd_Gfx_VGA	"hidd.gfx.vga"

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

struct VGAGfxDriverData
{
    struct Interrupt ResetInterrupt;
};

#endif /* VGAGFX_HIDD_H */
