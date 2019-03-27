/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 0
#include <aros/debug.h>
#include <aros/macros.h>
#include <datatypes/pictureclass.h>
#include <prefs/prefhdr.h>
#include <prefs/pointer.h>

#include <intuition/iprefs.h>
#include <proto/datatypes.h>

/*********************************************************************************************/

static void InstallBitMap(struct BitMap *bm, UWORD which, UWORD x, UWORD y, UWORD size, UWORD ysize)
{
    struct IPointerPrefs ip;
    
    ip.BitMap      = bm;
    ip.XOffset     = x;
    ip.YOffset     = y;
    ip.BytesPerRow = bm->BytesPerRow;
    ip.Size        = size;
    ip.YSize       = ysize;
    ip.Which       = which;
    
    SetIPrefs(&ip, sizeof(struct IPointerPrefs), IPREFS_TYPE_POINTER);
}

static void InstallPointer(struct PointerPrefs *pp, UWORD which)
{
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
    UWORD x = AROS_BE2WORD(pp->pp_X);
    UWORD y = AROS_BE2WORD(pp->pp_Y);
    UWORD size = AROS_BE2WORD(pp->pp_Size);
    UWORD ysize = AROS_BE2WORD(pp->pp_YSize);

    InitBitMap(&bm, depth, width, height);

    D(bug("[PointerPrefs] Which: %d\n", which));
    D(bug("[PointerPrefs] Bitmap: %dx%dx%d\n", width, height, depth));
    D(bug("[PointerPrefs] Size: %d\n", size));
    D(bug("[PointerPrefs] YSize: %d\n", ysize));
    D(bug("[PointerPrefs] Hotspot: (%d, %d)\n", x, y));

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
        InstallBitMap(&bm, which, x, y, size, ysize);
        FreeMem(ic, ic_size);
    }
}

static LONG stopchunks[] =
{
    ID_PREF, ID_PNTR,
    ID_PREF, ID_NPTR
};

static void LoadPointerPrefs(STRPTR filename, WORD which, WORD installas, LONG numstopchunks);

static void LoadPointerFile(STRPTR filename, ULONG which, UWORD installas, UWORD x, UWORD y)
{
    Object *o;
    struct BitMap *bm = NULL;
    UWORD h_which;

    D(bug("[PointerPrefs] Load file: %s\n", filename));
    o = NewDTObject(filename, DTA_GroupID, GID_PICTURE, PDTA_Remap, FALSE, TAG_DONE);
    D(bug("[PointerPrefs] Datatype object: 0x%p\n", o));
    /* If datatypes failed, try to load AmigaOS 3.x prefs file */
    if (!o) {
        /* Set numstopchunks to 1 because we want to avoid recursion
           if someone specifies new pointer prefs file as a target */
        LoadPointerPrefs(filename, which, installas, 1);
        return;
    }

    if (DoDTMethod(o, NULL, NULL, DTM_PROCLAYOUT, NULL, TRUE)) {
        D(bug("[PointerPrefs] Layout complete\n"));

        h_which = AROS_BE2WORD(installas);
        GetDTAttrs(o, PDTA_DestBitMap, &bm, TAG_DONE);
        D(bug("[PointerPrers] BitMap: 0x%p\n", bm));
        D(bug("[PointerPrefs] Which: %d\n", h_which));
        D(bug("[PointerPrefs] Size: %dx%d\n", bm->BytesPerRow * 8, bm->Rows));
        D(bug("[PointerPrefs] Hotspot: (%d, %d)\n", x, y));
        /* FIXME: What are actually Size and YSize ? */
        InstallBitMap(bm, h_which, x, y, 0, 0);
    }

    DisposeDTObject(o);
}

static void LoadPointerPrefs(STRPTR filename, WORD which, WORD installas, LONG numstopchunks)
{
    struct IFFHandle *iff;
    struct PointerPrefs *pp;
    struct NewPointerPrefs *npp;

    D(bug("[PointerPrefs] filename=%s\n",filename));
    iff = CreateIFF(filename, stopchunks, numstopchunks);
    if (iff) {
        while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
        {
            struct ContextNode   *cn;

            cn = CurrentChunk(iff);
            
            switch(cn->cn_ID) {
            case ID_PNTR:
                pp = LoadChunk(iff, sizeof(struct PointerPrefs), MEMF_CHIP);
                if (pp) {
                    D(bug("[PointerPrefs] Got AmigaOS 3.0 pointer chunk, code is %d\n", AROS_BE2WORD(pp->pp_Which)));
                    if ((which == -1) || (which == pp->pp_Which)) {
                        InstallPointer(pp, AROS_BE2WORD((installas == -1) ? pp->pp_Which : installas));
                    }
                    FreeVec(pp);
                }
                break;
            case ID_NPTR:
                npp = LoadChunk(iff, sizeof(struct NewPointerPrefs), MEMF_ANY);
                if (npp) {
                    UWORD alpha = AROS_BE2WORD(npp->npp_AlphaValue);
                    UWORD x     = AROS_BE2WORD(npp->npp_X);
                    UWORD y     = AROS_BE2WORD(npp->npp_Y);

                    D(bug("[PointerPrefs] Got new pointer chunk\n"));
                    D(bug("[PointerPrefs] Which %u, alpha: 0x%04X\n", AROS_BE2WORD(npp->npp_Which), alpha));
                    SetIPrefs(&alpha, sizeof(alpha), IPREFS_TYPE_POINTER_ALPHA);
                    LoadPointerFile(npp->npp_File, npp->npp_WhichInFile, npp->npp_Which, x, y);
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
    LoadPointerPrefs(filename, -1, -1, 2);
}
