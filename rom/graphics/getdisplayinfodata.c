/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetDisplayInfoData()
    Lang: english
*/

#include <aros/debug.h>
#include <proto/graphics.h>
#include <graphics/displayinfo.h>
#include <hidd/graphics.h>
#include <proto/oop.h>
#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "compositing_driver.h"
#include "dispinfo.h"

/****************************************************************************************/

static const ULONG size_checks[] = 
{
    sizeof(struct DisplayInfo),
    sizeof(struct DimensionInfo),
    sizeof(struct MonitorInfo),
    sizeof(struct NameInfo),
};

#define KNOWN_IDS 4

static ULONG check_sizes(ULONG tagID, ULONG size);
static UBYTE popcount(IPTR x);

#define DLONGSZ     	    (sizeof (ULONG) * 2)
#define DTAG_TO_IDX(dtag)   (((dtag) & 0x7FFFF000) >> 12)

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH5(ULONG, GetDisplayInfoData,

/*  SYNOPSIS */
        AROS_LHA(DisplayInfoHandle, handle, A0),
        AROS_LHA(UBYTE *, buf, A1),
        AROS_LHA(ULONG, size, D0),
        AROS_LHA(ULONG, tagID, D1),
        AROS_LHA(ULONG, ID, D2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 126, Graphics)

/*  FUNCTION
	Fills buffer with information about displayinfo handle.

    INPUTS
        handle - displayinfo handle
        buf    - pointer to destination buffer
        size   - buffer size in bytes
        tagID  - data chunk type
                 DTAG_DISP (DisplayInfo)
                 DTAG_DIMS (DimensionInfo)
                 DTAG_MNTR (MonitorInfo)
                 DTAG_NAME (NameInfo)
        ID     - displayinfo identifier, optionally used if handle is NULL

    RESULT
        result - if positive, number of bytes actually transferred
                 if zero, no information for ID was available

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        FindDisplayInfo(), NextDisplayInfo(), graphics/displayinfo.h

    INTERNALS
	This function provides private data in reserved fields of DimensionInfo.
	It is required by other AROS components. Please keep this in sync when
	changing the code.

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct QueryHeader  *qh;
    ULONG   	    	structsize;
    OOP_Object      	*gfxhidd, *sync, *pf;
    HIDDT_ModeID    	hiddmode;
    struct HIDD_ModeProperties HIDDProps = {0};

    if (NULL == handle)
    {
        /* FindDisplayInfo() handles INVALID_ID itself */
	handle = FindDisplayInfo(ID);
    }
    else
    {
    	/*
    	 * ID is likely INVALID_ID, but we need its value.
    	 * Get it from the handle.
    	 */
    	ID = DIH(handle)->id | DIH(handle)->drv->id;
    }

    if (NULL == handle)
    {
	D(bug("!!! COULD NOT GET HANDLE IN GetDisplayInfoData()\n"));
	return 0;
    }

    gfxhidd  = DIH(handle)->drv->gfxhidd;
    hiddmode = DIH(handle)->id;

    /* Get mode info from the HIDD */
    if (!HIDD_Gfx_GetMode(gfxhidd, hiddmode, &sync, &pf))
    {
	D(bug("NO VALID MODE PASSED TO GetDisplayInfoData() !!!\n"));
	return 0;
    }

    D(bug("GetDisplayInfoData(handle=%d, tagID=%x)\n"
    	, (ULONG)handle, tagID));

    /* Build the queryheader */
    structsize = check_sizes(tagID, size);
    if (!structsize)
	return 0;
    qh = AllocMem(structsize, MEMF_CLEAR);
    if (!qh)
        return 0;

    /* Fill in the queryheader */
    qh->StructID  = tagID;
    qh->DisplayID = ID;
    qh->SkipID	  = TAG_SKIP;

    qh->Length	  = (structsize + (DLONGSZ - 1)) / DLONGSZ;

    switch (tagID)
    {
    	case DTAG_DISP:
	{
	    struct DisplayInfo *di;
	    const struct MonitorSpec *ms;
	    IPTR redmask, greenmask, bluemask;
	    IPTR width, height;
	    IPTR val = 0;

	    OOP_GetAttr(sync, aHidd_Sync_MonitorSpec, (IPTR *)&ms);
	    HIDD_Gfx_ModeProperties(gfxhidd, hiddmode, &HIDDProps, sizeof(HIDDProps));
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

	    di = (struct DisplayInfo *)qh;

	    /* All modes returned from the HIDD are available */
	    di->NotAvailable = FALSE;

	    /*
	     * Set the propertyflags. We always set DIPF_IS_SPRITES because we always emulate
	     * one sprite for mouse pointer.
	     */
	    di->PropertyFlags = DIPF_IS_SPRITES | HIDDProps.DisplayInfoFlags;

	    /*
	     * Some more tweaks.
	     * All non-planar modes are considered Workbench-compatible. This is done
	     * for compatibility with existing RTG drivers which never set this flag themselves.
	     * This can be considered historical, in initial API design HIDD_Gfx_ModeProperties()
	     * did not exist at all and this flag was simply always set.
	     * In fact all modes can be considered Workbench-compatible. This flag is
	     * known to be used by original AmigaOS screenmode prefs program to filter out
	     * some modes. This program has many replacement which do not obey this flag,
	     * enabling opening Workbench on HAM screen for example.
	     * But we delegate setting this flag to the driver for planar modes. We do it
	     * for 100% compatibility of chipset driver. What if some m68k program has its own
	     * weird assumptions about this bit ?
	     * One more thing to note: we don't set DIPF_IS_FOREIGN bit. It is actually used
	     * only by Picasso96 and only for modes that are not graphics.library compatible.
	     * Many m68k RTG games rely on this flag not being set.
	     */
	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &val);
	    if (val != vHidd_StdPixFmt_Plane)
		di->PropertyFlags |= DIPF_IS_WB;

	    /* Compute red green and blue bits */
	    OOP_GetAttr(pf, aHidd_PixFmt_RedMask,   &redmask);
	    OOP_GetAttr(pf, aHidd_PixFmt_GreenMask, &greenmask);
	    OOP_GetAttr(pf, aHidd_PixFmt_BlueMask,  &bluemask);

	    /* Use gcc builtin function */
	    /* weissms: do not use, can cause undefined symbol _GLOBAL_OFFSET_TABLE_
	     * should work if we use a real cross compiler with static libgcc
	    di->RedBits   = __builtin_popcount(redmask);
	    di->GreenBits = __builtin_popcount(greenmask);
	    di->BlueBits  = __builtin_popcount(bluemask);
	    */

	    di->RedBits   = popcount(redmask);
	    di->GreenBits = popcount(greenmask);
	    di->BlueBits  = popcount(bluemask);

	    /*
	     * If number of colors is too large, PaletteRange is set to 65535.
	     * This is the behavior of original AmigaOS(tm).
	     */
	    val = di->RedBits * di->GreenBits * di->BlueBits;
	    di->PaletteRange = (val > 65535) ? 65535 : val;

	    OOP_GetAttr(sync, aHidd_Sync_PixelClock, &val);
	    OOP_GetAttr(sync, aHidd_Sync_PixelClock, &val);

	    /*
	     * Display resolution in ticks
	     */
	    di->Resolution.x = (1280 * 11 * ms->ratioh / width) >> RATIO_FIXEDPART;
	    di->Resolution.y = (1024 * 11 * ms->ratiov / height) >> RATIO_FIXEDPART;

	    if (val)
	        di->PixelSpeed = 1000000000 / val;

	    /*
	     * If the driver says it does not support sprites, we return one sprite.
	     * We emulate it by software in such a case because it's necessary for
	     * the mouse pointer.
	     */
	    di->NumStdSprites = (HIDDProps.DisplayInfoFlags & DIPF_IS_SPRITES) ? HIDDProps.NumHWSprites : 1;

	    /* At the moment sprites always have the same resolution as display */
	    di->SpriteResolution = di->Resolution;

	    break;
	}
	    
	case DTAG_DIMS:
	{
	    struct DimensionInfo *di;
	    IPTR depth, width, height;
	    IPTR minwidth, minheight;
	    IPTR maxwidth, maxheight;
	    
	    OOP_GetAttr(pf,   aHidd_PixFmt_Depth, &depth);
	    OOP_GetAttr(sync, aHidd_Sync_HDisp,   &width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp,   &height);
	    OOP_GetAttr(sync, aHidd_Sync_HMin,   &minwidth);
	    OOP_GetAttr(sync, aHidd_Sync_VMin,   &minheight);
	    OOP_GetAttr(sync, aHidd_Sync_HMax,   &maxwidth);
	    OOP_GetAttr(sync, aHidd_Sync_VMax,   &maxheight);

	    di = (struct DimensionInfo *)qh;
	    di->MaxDepth = depth;
	    
	    di->MinRasterWidth	= minwidth;
	    di->MinRasterHeight	= minheight;
	    di->MaxRasterWidth	= maxwidth;
	    di->MaxRasterHeight	= maxheight;
	    
	    di->Nominal.MinX	= 0;
	    di->Nominal.MinY	= 0;
	    di->Nominal.MaxX	= width  - 1;
	    di->Nominal.MaxY	= height - 1;

	    di->MaxOScan	= di->Nominal;
	    di->VideoOScan	= di->Nominal;
	    di->TxtOScan	= di->Nominal;
	    di->StdOScan	= di->Nominal;
/*
 * FIXME: our graphics subsystem does not have overscan API.
	    di->MaxOScan.MinX	= di->Nominal.MinX;
	    di->MaxOScan.MinY	= di->Nominal.MinY;
	    di->MaxOScan.MaxX	= di->Nominal.MaxX;
	    di->MaxOScan.MaxY	= di->Nominal.MaxY;

	    di->VideoOScan.MinX	= di->Nominal.MinX;
	    di->VideoOScan.MinY	= di->Nominal.MinY;
	    di->VideoOScan.MaxX	= di->Nominal.MaxX;
	    di->VideoOScan.MaxY	= di->Nominal.MaxY;

	    di->TxtOScan.MinX	= di->Nominal.MinX;
	    di->TxtOScan.MinY	= di->Nominal.MinY;
	    di->TxtOScan.MaxX	= di->Nominal.MaxX;
	    di->TxtOScan.MaxY	= di->Nominal.MaxY;

	    di->StdOScan.MinX	= di->Nominal.MinX;
	    di->StdOScan.MinY	= di->Nominal.MinY;
	    di->StdOScan.MaxX	= di->Nominal.MaxX;
	    di->StdOScan.MaxY	= di->Nominal.MaxY;
*/

	    /*
	     * reserved[0] is HIDD composition flags for intuition.library/OpenScreen().
	     * It can't be done in another way because only we (graphics.library) know about existence
	     * of software screen composition.
	     */
	    if (DIH(handle)->drv->composer)
	    {
	    	OOP_GetAttr(DIH(handle)->drv->composer, aHidd_Compositing_Capabilities, &di->reserved[0]);
	    }
	    else
	    {
		HIDD_Gfx_ModeProperties(gfxhidd, hiddmode, &HIDDProps, sizeof(HIDDProps));
		di->reserved[0] = HIDDProps.CompositionFlags;
	    }

	    /* This is for cybergraphics.library */
	    di->reserved[1] = (IPTR)pf;

	    break;
	}

	case DTAG_MNTR:
	{
	    struct MonitorInfo *mi = (struct MonitorInfo *)qh;
	    IPTR width, height;

	    OOP_GetAttr(sync, aHidd_Sync_MonitorSpec, (IPTR *)&mi->Mspc);
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

	    /*
	    mi->ViewPosition.X = ?;
	    mi->ViewPosition.Y = ?;
	    mi->ViewResolution.X = ?;
	    mi->ViewResolution.Y = ?;
	    mi->MinRow = ?;
	    mi->MouseTicks.X = ?;
	    mi->MouseTicks.Y = ?;
	    mi->DefaultViewPosition.X = ?;
	    mi->DefaultViewPosition.Y = ?;
	    */

            /* Resolution in ticks
             */
            mi->ViewResolution.x = (1280 * 11 * mi->Mspc->ratioh / width) >> RATIO_FIXEDPART;
            mi->ViewResolution.y = (1024 * 11 * mi->Mspc->ratiov / height) >> RATIO_FIXEDPART;

	    if (mi->Mspc)
	    {
	        mi->TotalRows         = mi->Mspc->total_rows;
	        mi->TotalColorClocks  = mi->Mspc->total_colorclocks;
	        mi->ViewPositionRange = mi->Mspc->ms_LegalView;
	    }

	    /*
	     * FIXME: For now we don't have a concept of preferred ModeID.
	     * However, see graphics_driver.c/driver_Setup(), it can be useful.
	     */
	    mi->PreferredModeID = ID;

	    if (DIH(handle)->drv->composer)
	    {
	    	/*
	    	 * If we have software screen composition, we know we can compose.
		 * We use MCOMPAT_MIXED here because of changed understanding of what is "monitor".
		 * In AmigaOS(tm) a "monitor" is actually a sync (video mode). Different "monitors"
	     	 * are actually different modes of the same display (PAL, NTSC, VGA, etc).
	     	 * In AROS a "monitor" is a single physical display device. Of course we can do
	     	 * composition only on a single display, but we can compose together different
	     	 * syncs.
	     	 */
	    	mi->Compatibility = MCOMPAT_MIXED;
	    }
	    else
	    {
	    	/* Otherwise query the driver */
		HIDD_Gfx_ModeProperties(gfxhidd, hiddmode, &HIDDProps, sizeof(HIDDProps));

		if (HIDDProps.CompositionFlags)
		    mi->Compatibility = (HIDDProps.CompositionFlags & COMPF_SAME) ? MCOMPAT_SELF : MCOMPAT_MIXED;
		else
		    mi->Compatibility = MCOMPAT_NOBODY;
	    }

	    break;
	}

	case DTAG_NAME:
	{
	    struct NameInfo *ni;
	    IPTR depth, stdpixfmt;
	    STRPTR sync_description;
	    
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpixfmt);

	    OOP_GetAttr(sync, aHidd_Sync_Description, (IPTR *)&sync_description);
	    ni = (struct NameInfo *)qh;

	    if (sync_description && sync_description[0] &&
	    	(IS_REAL_STDPIXFMT(stdpixfmt) || (stdpixfmt == vHidd_StdPixFmt_Unknown)))
	    {
	    	STRPTR pixfmt_name = "";
		
		switch(stdpixfmt)
		{
		    case vHidd_StdPixFmt_RGB16:
		    case vHidd_StdPixFmt_RGB15:
		    case vHidd_StdPixFmt_RGB24:
		    	pixfmt_name = "RGB";
			break;

		    case vHidd_StdPixFmt_RGB16_LE:
		    case vHidd_StdPixFmt_RGB15_LE:
		    	pixfmt_name = "RGB PC";
			break;
			
		    case vHidd_StdPixFmt_BGR24:
		    case vHidd_StdPixFmt_BGR16:
		    case vHidd_StdPixFmt_BGR15:
		    	pixfmt_name = "BGR";
			break;

		    case vHidd_StdPixFmt_BGR16_LE:
		    case vHidd_StdPixFmt_BGR15_LE:
		    	pixfmt_name = "BGR PC";
			break;
			
		    case vHidd_StdPixFmt_ARGB32:
		    	pixfmt_name = "ARGB";
			break;
			
		    case vHidd_StdPixFmt_BGRA32:
		    	pixfmt_name = "BGRA";
			break;
			
		    case vHidd_StdPixFmt_RGBA32:
		    	pixfmt_name = "RGBA";
			break;
			
		    case vHidd_StdPixFmt_0RGB32:
		    	pixfmt_name = "0RGB";
			break;
			
		    case vHidd_StdPixFmt_BGR032:
		    	pixfmt_name = "BGR0";
			break;
			
		    case vHidd_StdPixFmt_RGB032:
		    	pixfmt_name = "RGB0";
			break;
			
		}
		if (stdpixfmt != vHidd_StdPixFmt_Plane)
		    snprintf(ni->Name, DISPLAYNAMELEN, "%s %2dbit %s",
		    	 sync_description, (int)depth, pixfmt_name);
		else
		    strncpy(ni->Name, sync_description, DISPLAYNAMELEN);
	    }
	    else
	    {
	        IPTR width = 0;
		IPTR height = 0;

		OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
		OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
	    	snprintf(ni->Name, DISPLAYNAMELEN, "AROS: %ldx%ldx%ld", width, height, depth);
	    }
	    break;
	}

	default:
	    D(bug("!!! UNKNOWN tagID IN CALL TO GetDisplayInfoData() !!!\n"));
	    break;
    }
    
    D(bug("GDID: %d\n", structsize));
    
    if (size > structsize)
        size = structsize;
    CopyMem(qh, buf, size);
    FreeMem(qh, structsize);
    /* NULL-terminate the name in case if it was trimmed */
    if (tagID == DTAG_NAME)
        buf[size - 1] = 0;

    return size;

    AROS_LIBFUNC_EXIT

} /* GetDisplayInfoData */

/****************************************************************************************/

static ULONG check_sizes(ULONG tagID, ULONG size)
{
    ULONG idx;

    idx = DTAG_TO_IDX(tagID);

    if (idx > KNOWN_IDS)
    {
    	D(bug("!!! INVALID tagID TO GetDisplayInfoData"));
	return 0;
    }

    return size_checks[idx];
}

/* taken from http://en.wikipedia.org/wiki/Hamming_weight */
static UBYTE popcount(IPTR x) {
    UBYTE count;
    for (count=0; x; count++)
        x &= x-1;
    return count;
}
