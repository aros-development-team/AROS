/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new BitMap
    Lang: english
*/

#include <aros/debug.h>
#include <string.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <cybergraphx/cybergraphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "dispinfo.h"

#define SET_TAG(tags, idx, tag, val)	\
    tags[idx].ti_Tag = tag ; tags[idx].ti_Data = (IPTR)val;

#define SET_BM_TAG(tags, idx, tag, val)	\
    SET_TAG(tags, idx, aHidd_BitMap_ ## tag, val)

static HIDDT_StdPixFmt cyber2hidd_pixfmt[] =
{
    vHidd_StdPixFmt_LUT8,
    vHidd_StdPixFmt_RGB15,
    vHidd_StdPixFmt_BGR15,
    vHidd_StdPixFmt_RGB15_LE,
    vHidd_StdPixFmt_BGR15_LE,
    vHidd_StdPixFmt_RGB16,
    vHidd_StdPixFmt_BGR16,
    vHidd_StdPixFmt_RGB16_LE,
    vHidd_StdPixFmt_BGR16_LE,
    vHidd_StdPixFmt_RGB24,
    vHidd_StdPixFmt_BGR24,
    vHidd_StdPixFmt_ARGB32,
    vHidd_StdPixFmt_BGRA32,
    vHidd_StdPixFmt_RGBA32
};

/*****************************************************************************

    NAME */
#include <graphics/gfx.h>
#include <proto/graphics.h>

	AROS_LH5(struct BitMap *, AllocBitMap,

/*  SYNOPSIS */
	AROS_LHA(ULONG          , sizex, D0),
	AROS_LHA(ULONG          , sizey, D1),
	AROS_LHA(ULONG          , depth, D2),
	AROS_LHA(ULONG          , flags, D3),
	AROS_LHA(struct BitMap *, friend_bitmap, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 153, Graphics)

/*  FUNCTION
	Allocates and initializes a bitmap structure. Allocates and
	initializes bitplane data, and sets the bitmap's planes to point to
	it.

    INPUTS
	sizex, sizey - The width and height in pixels
	
	depth - The depth of the bitmap. A depth of 1 will allocate a
	    bitmap for two colors, a depth of 24 will allocate a bitmap for
	    16 million colors. Pixels with AT LEAST this many bits will be
	    allocated.
	
	flags - One of these flags:

	    BMF_CLEAR: Fill the bitmap with color 0.

	    BMF_DISPLAYABLE: to specify that this bitmap data should
		be allocated in such a manner that it can be displayed.
		Displayable data has more severe alignment restrictions
		than non-displayable data in some systems.
		Note that it may be not enough to specify only this flag
		to make the bitmap really displayable. See BMF_SCREEN
		description.

	    BMF_INTERLEAVED: tells graphics that you would like your
		bitmap to be allocated with one large chunk of display
		memory for all bitplanes. This minimizes color flashing on
		deep displays. If there is not enough contiguous RAM for an
		interleaved bitmap, graphics.library will fall back to a
		non-interleaved one.

	    BMF_MINPLANES: causes graphics to only allocate enough
		space in the bitmap structure for "depth" plane pointers.
		This is for system use and should not be used by
		applications use as it is inefficient, and may waste
		memory.

	    BMF_SPECIALFMT: causes graphics to allocate a bitmap
	    	of a standard CyberGraphX format. The format
		(PIXFMT_????) must be stored in the 8 most significant bits.

	    BMF_RTGTAGS,
	    BMF_RTGCHECK,
	    BMF_FRIENDSTAG: Setting these flags to 1's while BMF_SPECIALFMT
	        and BMF_INVALID are set to 0 means that friend_bitmap
		points to a taglist instead of BitMap structure.

	friend_bitmap - pointer to another bitmap, or NULL. If this pointer
	    is passed, then the bitmap data will be allocated in
	    the most efficient form for blitting to friend_bitmap.

	    This pointer can also point to a TagList, if specified by flags.
	    In this case it may contain the following tags:

	      - BMATags_Friend (struct BitMap *)
	            An actual pointer to friend bitmap. Defaults to NULL.

	      - BMATags_Depth (ULONG)
	            Depth of the bitmap to create. Defaults to depth argument
		    of AllocBitMap().

	      - BMATags_Clear (BOOL)
		    Tells if the newly created bitmap should be explicitly
		    cleared. Defaults to the value of BMF_CLEAR flag in
		    AllocBitMap() arguments.

	      - BMATags_Displayable (BOOL)
		    Tells if the bitmap should be displayable by the hardware.
		    Defaults to the value of BMF_DISPLAYABLE flag in AllocBitMap()
		    arguments.

	      - BMATags_NoMemory (BOOL)
		    Tells AllocBitMap() not to allocate actual bitmap storage. Only
		    header is allocated and set up. Default value is FALSE.
	
	      - BMATags_DisplayID (ULONG)
		    Allocate a displayable bitmap for specified display mode.

    RESULT
	A pointer to the new bitmap.

    NOTES
	When allocating using a friend_bitmap bitmap, it is not safe to assume
	anything about the structure of the bitmap data if that friend_bitmap
	BitMap might not be a standard Amiga bitmap (for instance, if the
	workbench is running on a non-Amiga display device, its
	Screen->RastPort->BitMap won't be in standard Amiga format. The
	only safe operations to perform on a non-standard BitMap are:

	    blitting it to another bitmap, which must be either a
		standard Amiga bitmap, or a friend_bitmap of this bitmap.

	    blitting from this bitmap to a friend_bitmap bitmap or to a
		standard Amiga bitmap.

	    attaching it to a rastport and making rendering calls.

	Good arguments to pass for the friend_bitmap are your window's
	RPort->BitMap, and your screen's RastPort->BitMap. Do NOT pass
	&(screenptr->BitMap)!

	BitMaps not allocated with BMF_DISPLAYABLE may not be used as
	Intuition Custom BitMaps or as RasInfo->BitMaps.  They may be
	blitted to a BMF_DISPLAYABLE BitMap, using one of the BltBitMap()
	family of functions.
	
	When allocating a displayable bitmap, make sure that its size is
	within limits allowed by the display driver. Use GetDisplayInfoData()
	with DTAG_DIMS in order to obtain the needed information.

    EXAMPLE

    BUGS

    SEE ALSO
	FreeBitMap()

    INTERNALS
        In order to allocate a displayable bitmap, you need to pass
	BMF_MINPLANES in addition to BMF_DISPLAYABLE flag. This is
	standard CGX convention. You may use BMF_REQUESTVMEM definition
	for this.

	BMF_SCREEN implies these flags.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct BitMap *nbm;
    HIDDT_ModeID hiddmode = vHidd_ModeID_Invalid;
    struct DisplayInfoHandle *dh;
    struct monitor_driverdata *drv = NULL;
    ULONG clear = flags & BMF_CLEAR;
    BOOL alloc = TRUE;

    if (BITMAPFLAGS_ARE_EXTENDED(flags))
    {
        const struct TagItem *tstate = (const struct TagItem *)friend_bitmap;
	struct TagItem *tag;

	friend_bitmap = NULL;

	while ((tag = NextTagItem(&tstate)))
	{
	    switch (tag->ti_Tag) {
	    case BMATags_Friend:
	        friend_bitmap = (struct BitMap *)tag->ti_Data;
		break;

	    case BMATags_Depth:
		depth = tag->ti_Data;
		break;

	    case BMATags_Clear:
		clear = tag->ti_Data;
		break;

	    case BMATags_NoMemory:
		alloc = !tag->ti_Data;
		break;

	    case BMATags_DisplayID:
		dh = FindDisplayInfo(tag->ti_Data);
		if (!dh)
		    return NULL;

		drv      = dh->drv;
		hiddmode = dh->id;
		flags   |= BMF_REQUESTVMEM;

		break;
	    }
	}
    }

    ASSERT_VALID_PTR_OR_NULL(friend_bitmap);
    D(bug("AllocBitMap(%u, %u, %u, 0x%08lX)\n", sizex, sizey, depth, flags));
    D(bug("[AllocBitMap] ModeID: 0x%08lX, friend_bitmap: 0x%p\n", hiddmode, friend_bitmap));

    /*
	If the depth is too large or the bitmap should be displayable or
	there is a friend_bitmap bitmap and that's not a normal bitmap, then
	call the RTG driver.
    */
    if ((depth > 8) || (hiddmode != vHidd_ModeID_Invalid) ||
	(friend_bitmap && (friend_bitmap->Flags & BMF_SPECIALFMT)) ||
	(flags & BMF_SPECIALFMT))
    {
	struct TagItem bm_tags[7];
	HIDDT_StdPixFmt stdpf = vHidd_StdPixFmt_Unknown;

	D(bug("[AllocBitMap] Allocating HIDD bitmap\n"));

	/* Set size */
	SET_BM_TAG( bm_tags, 0, Width,  sizex	);
	SET_BM_TAG( bm_tags, 1, Height, sizey	);

	/* Set friend bitmap */
	SET_TAG(bm_tags, 3, TAG_IGNORE, 0);
	if (friend_bitmap && IS_HIDD_BM(friend_bitmap))
	{
	    D(bug("[AllocBitMap] Setting friend bitmap: 0x%p\n", friend_bitmap));
	    SET_BM_TAG(bm_tags, 3, Friend, HIDD_BM_OBJ(friend_bitmap));

	    /* If we have no ModeID specified, obtain it from friend */
	    if (hiddmode == vHidd_ModeID_Invalid)
		hiddmode = HIDD_BM_HIDDMODE(friend_bitmap);

	    if (depth <= 8) /* CHECKME: only set depth if planar? */
	    	depth = HIDD_BM_REALDEPTH(friend_bitmap);

	    /* Obtain also GFX driver from friend bitmap */
	    drv = HIDD_BM_DRVDATA(friend_bitmap);
	}

	SET_BM_TAG( bm_tags, 2, Depth,  depth	);

	/* Now let's deal with pixelformat */
	if (flags & BMF_SPECIALFMT)
	{
	    ULONG cgxpf = DOWNSHIFT_PIXFMT(flags);
	    
	    if (cgxpf <= PIXFMT_RGBA32)
	        stdpf = cyber2hidd_pixfmt[cgxpf];
	}
	else if ((!friend_bitmap) && (hiddmode == vHidd_ModeID_Invalid))
	{
	    /* If there is neither pixelformat nor friend bitmap nor ModeID specified,
	       we have to use some default pixelformat depending on the depth */
	    if (depth > 24)
		stdpf = vHidd_StdPixFmt_ARGB32;
	    else if (depth > 16)
		stdpf = vHidd_StdPixFmt_RGB24;
	    else if (depth > 15)
		stdpf = vHidd_StdPixFmt_RGB16;
	    else if (depth > 8)
		stdpf = vHidd_StdPixFmt_RGB15;
	    else
		stdpf = vHidd_StdPixFmt_LUT8;
	}
	/* If we have a friend bitmap, pixelformat will be
	   picked up from it */

	if (stdpf != vHidd_StdPixFmt_Unknown)
	{
	    D(bug("[AllocBitMap] Setting pixelformat to %d\n", stdpf));
	    SET_BM_TAG(bm_tags, 4, StdPixFmt, stdpf);
	    hiddmode = vHidd_ModeID_Invalid;
	}
	else if (hiddmode != vHidd_ModeID_Invalid)
	{
	    D(bug("[AllocBitMap] Setting ModeID to 0x%08lX\n", hiddmode));
	    SET_BM_TAG(bm_tags, 4, ModeID, hiddmode);
	}
	else
	{
	    /*
	     * SET_TAG() is TWO operators, so we absolutely need parenthesis here.
	     * Remember this!
	     */
	    SET_TAG(bm_tags, 4, TAG_IGNORE, 0);
	    hiddmode = vHidd_ModeID_Invalid;
	}

	/* Set Displayable attribute */
	SET_BM_TAG(bm_tags, 5, Displayable, ((flags & BMF_REQUESTVMEM) == BMF_REQUESTVMEM));
	D(bug("[AllocBitMap] Displayable: %d\n", bm_tags[5].ti_Data));

	SET_TAG(bm_tags, 6, TAG_DONE, 0);

	nbm = AllocMem (sizeof (struct BitMap), MEMF_ANY|MEMF_CLEAR);
	D(bug("[AllocBitMap] Allocated bitmap structure: 0x%p\n", nbm));
	
	if (nbm)
	{
    	    OOP_Object *bm_obj = NULL;
	    BOOL ok = TRUE;

	    /* Use the memory driver if we didn't get another object in any way */
	    if (!drv)
    	        drv = (struct monitor_driverdata *)CDD(GfxBase);

	    if (alloc)
	    {
		bm_obj = HIDD_Gfx_NewBitMap(drv->gfxhidd, bm_tags);
		D(bug("[AllocBitMap] Created bitmap object 0x%p\n", bm_obj));
		if (!bm_obj)
		    ok = FALSE;
	    }

    	    if (ok)
	    {
		IPTR width = sizex;
		IPTR height = sizey;
		HIDDT_ColorModel colmod = -1;

		if (alloc)
		{
    		    OOP_Object      *pf;
    		    OOP_Object      *colmap = NULL;
		    IPTR val, bmtype;

    		    /*  It is possible that the HIDD had to allocate
    		        a larger depth than that supplied, so
    		        we should get back the correct depth.
    		        This is because layers.library might
    		        want to allocate offscreen bitmaps to
    		        store obscured areas, and then those
    		        offscreen bitmaps should be of the same depth as
    		        the onscreen ones. */
    		    OOP_GetAttr(bm_obj, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    		    OOP_GetAttr(pf, aHidd_PixFmt_BitMapType, &bmtype);

		    OOP_GetAttr(bm_obj, aHidd_BitMap_Width, &width);
		    OOP_GetAttr(bm_obj, aHidd_BitMap_Height, &height);
		    /* aHidd_PixFmt_Depth is max supported depth if planar bitmap */
		    if (bmtype == vHidd_BitMapType_Planar)
		    	OOP_GetAttr(bm_obj, aHidd_BitMap_Depth, &val);
		    else
		    	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &val);
		    depth = val;

    		    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &colmod);

    		    OOP_GetAttr(bm_obj, aHidd_BitMap_ColorMap, (IPTR *)&colmap);

    		    /* Store it in plane array */
    		    HIDD_BM_OBJ(nbm)        = bm_obj;
    		    HIDD_BM_COLMOD(nbm)     = colmod;
    		    HIDD_BM_COLMAP(nbm)     = colmap;
		    HIDD_BM_REALDEPTH(nbm)  = depth;
		}
		else
		    /* There's nothing to clear if we don't allocate an object */
		    clear = FALSE;

		HIDD_BM_DRVDATA(nbm)    = drv;
		HIDD_BM_HIDDMODE(nbm)   = hiddmode;

    		nbm->Rows   = height;
    		nbm->BytesPerRow = WIDTH_TO_BYTES(width);
#if BMDEPTH_COMPATIBILITY
		nbm->Depth  = (depth > 8) ? 8 : depth;
#else
    		nbm->Depth  = depth;
#endif
    		nbm->Flags  = flags | BMF_SPECIALFMT;

    		/* If this is a displayable bitmap, create a color table for it */
    		if (alloc && (friend_bitmap ||
                    (flags & BMF_REQUESTVMEM) == BMF_REQUESTVMEM))
		{
		    HIDD_BM_FLAGS(nbm) |= HIDD_BMF_SCREEN_BITMAP;

    		    if (friend_bitmap && IS_HIDD_BM(friend_bitmap))
		    {
    		        /* We got a friend_bitmap bitmap. We inherit its
                           colormap.
    		           !!! NOTE !!! If this is used after the
                           friend_bitmap bitmap is freed it means trouble,
                           as the colortab mem will no longer be valid */
			OOP_Object *oldcolmap;
			    
			oldcolmap = HIDD_BM_SetColorMap(HIDD_BM_OBJ(nbm), HIDD_BM_COLMAP(friend_bitmap));
			if (oldcolmap)
			    OOP_DisposeObject(oldcolmap);

    	    	    	HIDD_BM_COLMAP(nbm)     = HIDD_BM_COLMAP(friend_bitmap);			    
			HIDD_BM_PIXTAB(nbm)     = HIDD_BM_PIXTAB(friend_bitmap);
     			HIDD_BM_COLMOD(nbm)     = HIDD_BM_COLMOD(friend_bitmap);
    	    	    	HIDD_BM_REALDEPTH(nbm)  = HIDD_BM_REALDEPTH(friend_bitmap);

			HIDD_BM_FLAGS(nbm) |= HIDD_BMF_SHARED_PIXTAB;
		    }
		    else
		    {
    			/* Allocate a pixtab */
    			HIDD_BM_PIXTAB(nbm) = AllocVec(sizeof (HIDDT_Pixel) * AROS_PALETTE_SIZE, MEMF_ANY);

    			if (HIDD_BM_PIXTAB(nbm))
			{
    			    /* Set this palette to all black by default */

    			    HIDDT_Color col;
    			    ULONG   	i;

    			    col.red     = 0;
    			    col.green   = 0;
    			    col.blue    = 0;
    			    col.alpha   = 0;

    			    if (vHidd_ColorModel_Palette == colmod || vHidd_ColorModel_TrueColor == colmod)
			    {
    				ULONG numcolors;

				numcolors = 1L << ((depth <= 8) ? depth : 8);

    				/* Set palette to all black */
    				for (i = 0; i < numcolors; i ++) {
    				    HIDD_BM_SetColors(bm_obj, &col, i, 1);
    	    	    	    	    HIDD_BM_PIXTAB(nbm)[i] = col.pixval;
    				}
    			    }
    			} /* if (pixtab successfully allocated) */
			else
			    ok = FALSE;
		    }
    		}

    		if (ok)
		{
		    if (clear)
		    	BltBitMap(nbm, 0, 0, nbm, 0, 0, width, height, 0x00, 0xFF, NULL);
    		    ReturnPtr("driver_AllocBitMap", struct BitMap *, nbm);
    		}

    		HIDD_Gfx_DisposeBitMap(drv->gfxhidd, bm_obj);
    	    } /* if (bitmap object allocated) */

    	    FreeMem(nbm, sizeof (struct BitMap));
	    nbm = NULL;

	} /* if (nbm) */
	
    }
    else /* Otherwise init a plain Amiga bitmap */
    {
	nbm = AllocMem (sizeof(struct BitMap) + ((depth > 8) ? (depth - 8) * sizeof(PLANEPTR) : 0),
	    	    	MEMF_ANY | MEMF_CLEAR);

	if (nbm)
	{
	    nbm->BytesPerRow = ((sizex + 15) >> 4) * 2;
	    nbm->Rows	     = sizey;
	    nbm->Flags	     = flags | BMF_STANDARD;
	    nbm->Depth	     = depth;
	    nbm->pad	     = 0;

	    if (alloc)
	    {
		ULONG plane;

		for (plane=0; plane<depth; plane++)
		{
		    nbm->Planes[plane] = AllocRaster (sizex, sizey);

		    if (!nbm->Planes[plane])
			break;

		    if (clear)
			memset (nbm->Planes[plane], 0, RASSIZE(sizex,sizey));
		}

		if (plane != depth)
		{
		    for (plane=0; plane<depth; plane++)
			if (nbm->Planes[plane])
			    FreeRaster (nbm->Planes[plane], sizex, sizey);

		    FreeMem (nbm, sizeof (struct BitMap));

		    nbm = NULL;
		}
	    }
	}
    }

    return nbm;

    AROS_LIBFUNC_EXIT
} /* AllocBitMap */
