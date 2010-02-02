/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: pointerprefs.c 30792 2009-03-07 22:40:04Z neil $

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 0
#include <aros/debug.h>
#include <aros/macros.h>
#include <prefs/prefhdr.h>
#include <prefs/pointer.h>

#include <intuition/iprefs.h>

/*********************************************************************************************/

static void InstallPointer(struct PointerPrefs *pp, UWORD which)
{
    struct IPointerPrefs ip;
    struct BitMap bm;
    struct ColorSpec *ic;
    struct RGBTable *colors = (struct RGBTable *)&pp[1];
    UWORD i;
    UWORD width  = AROS_BE2WORD(pp->pp_Width);
    UWORD height = AROS_BE2WORD(pp->pp_Height);
    UWORD depth  = AROS_BE2WORD(pp->pp_Depth);
    UWORD ncols = (1 << depth) - 1;
    ULONG ic_size = (1 << depth) * sizeof(struct ColorSpec);
    UBYTE *planes;

    InitBitMap(&bm, depth, width, height);
    ip.BitMap      = &bm;
    ip.XOffset     = AROS_BE2WORD(pp->pp_X);
    ip.YOffset     = AROS_BE2WORD(pp->pp_Y);
    ip.BytesPerRow = bm.BytesPerRow;
    ip.Size        = AROS_BE2WORD(pp->pp_Size);
    ip.YSize       = AROS_BE2WORD(pp->pp_YSize);
    ip.Which       = which;

    D(bug("[PointerPrefs] Which: %d\n", ip.Which));
    D(bug("[PointerPrefs] Data size: %d\n", ip.Size));
    D(bug("[PointerPrefs] Bitmap: %dx%dx%d\n", width, height, depth));
    D(bug("[PointerPrefs] YSize: %d\n", ip.YSize));
    D(bug("[PointerPrefs] Hotspot: (%d, %d)\n", ip.XOffset, ip.YOffset));

    ic = AllocMem(ic_size, MEMF_ANY);
    if (ic) {
        for (i = 0; i < ncols; i++) {
            D(bug("[PointerPrefs] Color %u RGB 0x%02X%02X%02X\n", i, colors->t_Red, colors->t_Green, colors->t_Blue));
	    ic[i].ColorIndex = i + 8; /* Pointer colors have numbers 8-10 in the intuition's internal table */
	    ic[i].Red   = colors->t_Red   * 0x0101;
	    ic[i].Green = colors->t_Green * 0x0101;
	    ic[i].Blue  = colors->t_Blue  * 0x0101;
	    colors++;
	}
        ic[ncols].ColorIndex = -1; /* Terminator */

	planes = (UBYTE *)colors;
	for (i = 0; i < depth; i++) {
	    bm.Planes[i] = planes;
	    planes += bm.BytesPerRow * height;
	}

        /* First change palette, then image. It is important in order to get correct colors on
           truecolor screens */
        SetIPrefs(ic, ic_size, IPREFS_TYPE_OLD_PALETTE);
        SetIPrefs(&ip, sizeof(struct IPointerPrefs), IPREFS_TYPE_POINTER);
	FreeMem(ic, ic_size);
    }
}

static LONG stopchunks[] =
{
    ID_PREF, ID_PNTR,
    ID_PREF, ID_NPTR
};

static void LoadPointerPrefs(STRPTR filename, WORD which, WORD installas);

static void LoadPointerFile(STRPTR filename, ULONG which, UWORD installas, UWORD x, UWORD y)
{
    /* TODO: this may be NOT an IFF IREF file. It may also be raw image file.
             We need to detect the file format and use datatypes if appropriate. */
    LoadPointerPrefs(filename, which, installas);
}

static void LoadPointerPrefs(STRPTR filename, WORD which, WORD installas)
{
    struct IFFHandle *iff;
    struct PointerPrefs *pp;
    struct NewPointerPrefs *npp;

    D(bug("filename=%s\n",filename));
    iff = CreateIFF(filename, stopchunks, 1);
    if (iff) {
    	while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
	{
	    struct ContextNode   *cn;

	    cn = CurrentChunk(iff);
	    
	    switch(cn->cn_ID) {
	    case ID_PNTR:
	        pp = LoadChunk(iff, sizeof(struct PointerPrefs), MEMF_CHIP);
		if (pp) {
		    D(bug("[PointerPrefs] Got AmigaOS 3.0 pointer chunk, code is %d\n", code));
		    if ((which == -1) || (which == pp->pp_Which)) {
		        InstallPointer(pp, AROS_BE2WORD((installas == -1) ? pp->pp_Which : installas));
		    }
		    FreeVec(pp);
		}
		break;
	    case ID_NPTR:
	        npp = LoadChunk(iff, sizeof(struct NewPointerPrefs), MEMF_ANY);
		if (npp) {
		    D(bug("[PointerPrefs] Got new pointer chunk\n"));
		    SetIPrefs(&npp->npp_AlphaValue, sizeof(UWORD), IPREFS_TYPE_POINTER_ALPHA);
		    LoadPointerFile(npp->npp_File, npp->npp_WhichInFile, npp->npp_Which, npp->npp_X, npp->npp_Y);
		    FreeVec(npp);
		}
		break;
	    }
	}
	KillIFF(iff);
    }
}

void PointerPrefs_Handler(STRPTR filename)
{
    D(bug("In IPrefs:PointerPrefs_Handler\n"));
    LoadPointerPrefs(filename, -1, -1);
}
