/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new BitMap
    Lang: english
*/

#include <aros/debug.h>
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <cybergraphx/cybergraphics.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "dispinfo.h"

#define SET_TAG(tags, idx, tag, val)	\
    tags[idx].ti_Tag = tag ; tags[idx].ti_Data = (IPTR)val;

#define SET_BM_TAG(tags, idx, tag, val)	\
    SET_TAG(tags, idx, aHidd_BitMap_ ## tag, val)

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
	
	    BMF_SCREEN: causes graphics to allocate a bitmap which is actually
		displayable using RTG driver. You must also pass a displaymode
		ID number (and NOT a bitmap pointer) in friend_bitmap parameter.
		Note that this flag is not stored in Flags member of the BitMap
		structure.

	friend_bitmap - pointer to another bitmap, or NULL. If this pointer
	    is passed, then the bitmap data will be allocated in
	    the most efficient form for blitting to friend_bitmap.

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
	Currently AROS ignores BMF_DISPLAYABLE flag at all. In order to
	allocate a real displayable bitmap, you should pass BMF_SCREEN
	flag with ModeID specified in friend_bitmap parameter.
	This is a real difference of AROS, keep it in mind.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct BitMap *nbm;
    ULONG hiddmode = INVALID_ID;

    if (flags & BMF_SCREEN)
    {
    	hiddmode      = (ULONG)friend_bitmap;
	friend_bitmap = NULL;
    }

    ASSERT_VALID_PTR_OR_NULL(friend_bitmap);
    D(bug("AllocBitMap(%u, %u, %u, 0x%08lX)\n", sizex, sizey, depth, flags));
    D(bug("[AllocBitMap] ModeID: 0x%08lX, friend_bitmap: 0x%p\n", hiddmode, friend_bitmap));
    
    /*
	If the depth is too large or the bitmap should be displayable or
	there is a friend_bitmap bitmap and that's not a normal bitmap, then
	call the RTG driver.
    */
    if (
	depth > 8
	|| (flags & BMF_SCREEN)
	|| (friend_bitmap && friend_bitmap->Flags & BMF_AROS_HIDD)
    	#warning Should	we also check for BMF_MINPLANES ?
	|| (flags & BMF_SPECIALFMT) /* Cybergfx bitmap */
    )
    {

	struct TagItem bm_tags[8];	/* Tags for offscreen bitmaps */

	D(bug("[AllocBitMap] Allocating HIDD bitmap\n"));


	SET_BM_TAG( bm_tags, 0, Width,  sizex	);
	SET_BM_TAG( bm_tags, 1, Height, sizey	);

	if (flags & BMF_SCREEN)
	{
	    D(bug("[AllocBitMap] Allocating screen bitmap\n"));
	    /* Use the hiddmode instead of depth/friend_bitmap */
	    if  (INVALID_ID == hiddmode)
    		ReturnPtr("driver_AllocBitMap(Invalid modeID)", struct BitMap *, NULL);

	    SET_BM_TAG(bm_tags, 2, ModeID, AMIGA_TO_HIDD_MODEID(hiddmode));
	    SET_BM_TAG(bm_tags, 3, Displayable, TRUE);
	    SET_TAG(bm_tags, 4, TAG_DONE, 0);
	}
	else
	{
	    HIDDT_StdPixFmt stdpf;
	    
	    /* Set friend bitmap if given */
	    SET_TAG(bm_tags, 2, TAG_IGNORE, 0);
	    if (NULL != friend_bitmap)
	    {
		if (IS_HIDD_BM(friend_bitmap)) {
			D(bug("[AllocBitMap] Setting friend bitmap object: 0x%p\n", HIDD_BM_OBJ(friend_bitmap)));
		    SET_BM_TAG(bm_tags, 2, Friend, HIDD_BM_OBJ(friend_bitmap));
		}
	    }

	    if (flags & BMF_SPECIALFMT)
	    {
		stdpf = cyber2hidd_pixfmt(DOWNSHIFT_PIXFMT(flags), GfxBase);
		D(bug("[AllocBitMap] Setting pixelformat to %d\n", stdpf));
		SET_BM_TAG(bm_tags, 3, StdPixFmt, stdpf);
	    } else if (!friend_bitmap) {
		/* If there is neither pixelformat nor friend bitmap specified,
		   we have to use some default pixelformat depending on the depth */
	        if (depth > 24)
		    stdpf = vHidd_StdPixFmt_ARGB32;
		else if (depth > 16)
		    stdpf = vHidd_StdPixFmt_0RGB32;
		else if (depth > 15)
		    stdpf = vHidd_StdPixFmt_RGB16;
		else if (depth > 8)
		    stdpf = vHidd_StdPixFmt_RGB15;
		else
		    stdpf = vHidd_StdPixFmt_LUT8;
		D(bug("[AllocBitMap] Setting pixelformat to %d\n", stdpf));
		SET_BM_TAG(bm_tags, 3, StdPixFmt, stdpf);
	    } else {
		/* If we have a friend bitmap, pixelformat will be
		   picked up from it */
		SET_TAG(bm_tags, 3, TAG_IGNORE, 0);
	    }

	    SET_TAG(bm_tags, 4, TAG_DONE, 0);
	}

	nbm = AllocMem (sizeof (struct BitMap), MEMF_ANY|MEMF_CLEAR);
	D(bug("[AllocBitMap] Allocated bitmap structure: 0x%p\n", nbm));
	if (NULL != nbm)
	{

    	    OOP_Object *bm_obj;
    	    OOP_Object *gfxhidd;

    	    gfxhidd  = SDD(GfxBase)->gfxhidd;

    	    /* Create HIDD bitmap object */
    	    if (NULL != gfxhidd)
	    {
		D(bug("[AllocBitMap] Creating bitmap object\n"));
    		bm_obj = HIDD_Gfx_NewBitMap(gfxhidd, bm_tags);
		D(bug("[AllocBitMap] Created bitmap object 0x%p\n", bm_obj));
    		if (NULL != bm_obj)
    		{

    		    OOP_Object      	*pf;
    		    OOP_Object      	*colmap = 0;
    		    HIDDT_ColorModel 	 colmod;
    		    BOOL    	    	 ok = FALSE;
		    IPTR    	    	 width, height, val;


    		    /*  It is possible that the HIDD had to allocate
    			a larger depth than that supplied, so
    			we should get back the correct depth.
    			This is because layers.library might
    			want to allocate offscreen bitmaps to
    			store obscured areas, and then those
    			offscreen bitmaps should be of the same depth as
    			the onscreen ones.
    		    */

		    OOP_GetAttr(bm_obj, aHidd_BitMap_Width, &width);
		    OOP_GetAttr(bm_obj, aHidd_BitMap_Height, &height);
    		    OOP_GetAttr(bm_obj, aHidd_BitMap_PixFmt, (IPTR *)&pf);

    		    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &val);
		    depth = val;
		    
    		    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &val);
    	    	    colmod = val;
		    
    		    OOP_GetAttr(bm_obj, aHidd_BitMap_ColorMap, (IPTR *)&colmap);

    			/* Store it in plane array */
    		    HIDD_BM_OBJ(nbm) 	    = bm_obj;
    		    HIDD_BM_COLMOD(nbm)     = colmod;
    		    HIDD_BM_COLMAP(nbm)     = colmap;
		    HIDD_BM_REALDEPTH(nbm)  = depth;
		    HIDD_BM_HIDDMODE(nbm)   = hiddmode; /* Note that it's Amiga ModeID, not a raw HIDD ModeID */
		    
    		    nbm->Rows   = height;
    		    nbm->BytesPerRow = WIDTH_TO_BYTES(width);
		#if BMDEPTH_COMPATIBILITY
		    nbm->Depth  = (depth > 8) ? 8 : depth;
		#else
    		    nbm->Depth  = depth;
		#endif
    		    nbm->Flags  = flags | BMF_AROS_HIDD;

    		    /* If this is a displayable bitmap, create a color table for it */
    		    if (flags & BMF_SCREEN)
		    {
		    	HIDD_BM_FLAGS(nbm) |= HIDD_BMF_SCREEN_BITMAP;

		    	if (friend_bitmap)
			{
			    OOP_Object *oldcolmap;
			    
			    oldcolmap = HIDD_BM_SetColorMap(HIDD_BM_OBJ(nbm), HIDD_BM_COLMAP(friend_bitmap));
			    if (oldcolmap) OOP_DisposeObject(oldcolmap);

    	    	    	    HIDD_BM_COLMAP(nbm)     = HIDD_BM_COLMAP(friend_bitmap);			    
			    HIDD_BM_PIXTAB(nbm)     = HIDD_BM_PIXTAB(friend_bitmap);
     			    HIDD_BM_COLMOD(nbm)     = HIDD_BM_COLMOD(friend_bitmap);
    	    	    	    HIDD_BM_REALDEPTH(nbm)  = HIDD_BM_REALDEPTH(friend_bitmap);

			    HIDD_BM_FLAGS(nbm) |= HIDD_BMF_SHARED_PIXTAB;
			    
			    ok = TRUE;
			}
			else
			{
    			    /* Allcoate a pixtab */
    			    HIDD_BM_PIXTAB(nbm) = AllocVec(sizeof (HIDDT_Pixel) * AROS_PALETTE_SIZE, MEMF_ANY);
			
    			    if (NULL != HIDD_BM_PIXTAB(nbm))
			    {
    				/* Set this palette to all black by default */

    				HIDDT_Color col;
    				ULONG   	i;

    				col.red     = 0;
    				col.green   = 0;
    				col.blue    = 0;
    				col.alpha   = 0;

    				if (vHidd_ColorModel_Palette == colmod ||
			            vHidd_ColorModel_TrueColor == colmod)
				{
    				    ULONG numcolors;

				    numcolors = 1L << ((depth <= 8) ? depth : 8);

    				    /* Set palette to all black */
    				    for (i = 0; i < numcolors; i ++)
				    {
    					HIDD_BM_SetColors(HIDD_BM_OBJ(nbm), &col, i, 1);
    	    	    	    		HIDD_BM_PIXTAB(nbm)[i] = col.pixval;
    				    }

    				}
    				ok = TRUE;

    			    } /* if (pixtab successfully allocated) */
			
			}
			
    		    } /* if (flags & BMF_SCREEN) */
    		    else
    		    {
    			if (friend_bitmap)
    			{
    			    /* We got a friend_bitmap bitmap. We inherit its colormap
    			       !!! NOTE !!! If this is used after the friend_bitmap bitmap is freed
    			       it means trouble, as the colortab mem
    			       will no longer be valid
    			    */
    			    if (IS_HIDD_BM(nbm))
			    {

    				HIDD_BM_COLMAP(nbm) 	= HIDD_BM_COLMAP(friend_bitmap);
    				HIDD_BM_COLMOD(nbm) 	= HIDD_BM_COLMOD(friend_bitmap);
    				HIDD_BM_PIXTAB(nbm) 	= HIDD_BM_PIXTAB(friend_bitmap);
    	    	    	    	HIDD_BM_REALDEPTH(nbm)  = HIDD_BM_REALDEPTH(friend_bitmap);
    				ok = TRUE;
    			    }


    			}
			else
			{
    	    	    	    HIDD_BM_REALDEPTH(nbm) = depth;		    
			    ok = TRUE;
			}
    		    }

    		    if (ok)
		    {
			if (flags & BMF_CLEAR)
			{
		    	    BltBitMap(nbm
				    , 0, 0
				    , nbm
				    , 0, 0
				    , width, height
				    , 0x00
				    , 0xFF
				    , NULL
			    );
			}
    			ReturnPtr("driver_AllocBitMap", struct BitMap *, nbm);
    		    }

    		    OOP_DisposeObject(bm_obj);
		    
    		} /* if (bitmap object allocated) */

    	    } /* if (gfxhidd) */
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
	    ULONG plane;

	    nbm->BytesPerRow = ((sizex + 15) >> 4) * 2;
	    nbm->Rows	     = sizey;
	    nbm->Flags	     = flags | BMF_STANDARD;
	    nbm->Depth	     = depth;
	    nbm->pad	     = 0;

	    for (plane=0; plane<depth; plane++)
	    {
		nbm->Planes[plane] = AllocRaster (sizex, sizey);

		if (!nbm->Planes[plane])
		    break;

		if (flags & BMF_CLEAR)
		    memset (nbm->Planes[plane], 0, RASSIZE(sizex,sizey));
	    }

	    if (plane != depth)
	    {
		for (plane=0; plane<depth; plane++)
		    if (nbm->Planes[plane])
			FreeRaster (nbm->Planes[plane], sizex, sizey);

		FreeMem (nbm, sizeof (struct BitMap));

		nbm = 0;
	    }
	}
    }

    return nbm;
    
    AROS_LIBFUNC_EXIT
    
} /* AllocBitMap */
