/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: pointerprefs.c 30792 2009-03-07 22:40:04Z neil $

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 1
#include <aros/debug.h>
#include <aros/macros.h>
#include <prefs/prefhdr.h>
#include <prefs/pointer.h>

#include <intuition/iprefs.h>

/*********************************************************************************************/

#define POINTER_COLORS 3

static void InstallPointer(struct PointerPrefs *pp)
{
    struct IPointerPrefs ip;
    struct ColorSpec ic[POINTER_COLORS + 1];
    struct BitMap bm;
    struct RGBTable *colors = (struct RGBTable *)&pp[1];
    UWORD i;
    UWORD width  = AROS_BE2WORD(pp->pp_Width);
    UWORD height = AROS_BE2WORD(pp->pp_Height);
    UWORD depth  = AROS_BE2WORD(pp->pp_Depth);
    UWORD ncols = (1 << depth) - 1;

    InitBitMap(&bm, depth, width, height);

    ip.BitMap      = &bm;
    ip.XOffset     = AROS_BE2WORD(pp->pp_X);
    ip.YOffset     = AROS_BE2WORD(pp->pp_Y);
    ip.BytesPerRow = bm.BytesPerRow;
    ip.Size        = AROS_BE2WORD(pp->pp_Size);
    ip.YSize       = AROS_BE2WORD(pp->pp_YSize);
    ip.Which       = AROS_BE2WORD(pp->pp_Which);

    D(bug("[PointerPrefs] Which: %d\n", ip.Which));
    D(bug("[PointerPrefs] Data size: %d\n", ip.Size));
    D(bug("[PointerPrefs] Bitmap: %dx%dx%d\n", width, height, depth));
    D(bug("[PointerPrefs] YSize: %d\n", ip.YSize));
    D(bug("[PointerPrefs] Hotspot: (%d, %d)\n", ip.XOffset, ip.YOffset));

    if (ncols > POINTER_COLORS)
        ncols = POINTER_COLORS;
    for (i = 0; i < ncols; i++) {
        D(bug("[PointerPrefs] Color %u: 0x%02X %02X %02X\n", i, colors->t_Red, colors->t_Green, colors->t_Blue));
	ic[i].ColorIndex = i + 8; /* Pointer colors have numbers 8-10 in the intuition's internal table */
	ic[i].Red   = colors->t_Red   * 0x01010101;
	ic[i].Green = colors->t_Green * 0x01010101;
	ic[i].Blue  = colors->t_Blue  * 0x01010101;
	colors++;
    }
    ic[POINTER_COLORS].ColorIndex = -1;
    
    /* First change palette, then image. It is important in order to get correct colors on
       truecolor screens */
    SetIPrefs(&ic, sizeof(ic), IPREFS_TYPE_OLD_PALETTE);
//  SetIPrefs(&ip, sizeof(struct IPointerPrefs), IPREFS_TYPE_POINTER);
}

static LONG stopchunks[] =
{
    ID_PREF, ID_PNTR
};

void PointerPrefs_Handler(STRPTR filename)
{	
    struct IFFHandle *iff;
    struct PointerPrefs *pp;

    D(bug("In IPrefs:PointerPrefs_Handler\n"));
    D(bug("filename=%s\n",filename));

    iff = CreateIFF(filename, stopchunks, 1);
    if (iff) {
        while ((pp = LoadChunk(iff, sizeof(struct PointerPrefs))) != NULL) {
	    D(bug("[PointerPrefs] Got pointer chunk\n"));
	    InstallPointer(pp);
	    FreeVec(pp);
	}
	KillIFF(iff);
    }

}
