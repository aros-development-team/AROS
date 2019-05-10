/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Gfx Hidd initialization code.
    Lang: English.
*/

#include "gfx_debug.h"

#define __OOP_NOMETHODBASES__

#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "gfx_intern.h"

#include LC_LIBDEFS_FILE

#undef csd

/****************************************************************************************/

/* Since the shift/mask values of a pixel format are designed for pixel
   access, not byte access, they are endianess dependant */
   
#if AROS_BIG_ENDIAN
#include "stdpixfmts_be.h"
#else
#include "stdpixfmts_le.h"
#endif

/****************************************************************************************/

/*
    Parses the tags supplied in 'tags' and puts the result into 'pf'.
    It also checks to see if all needed attrs are supplied.
    It uses 'attrcheck' for this, so you may find attrs outside
    of this function, and mark them as found before calling this function
*/

#define PFAF(x) (1L << aoHidd_PixFmt_ ## x)
#define PF_COMMON_AF (   PFAF(Depth) | PFAF(BitsPerPixel) | PFAF(BytesPerPixel)	\
		       | PFAF(ColorModel) | PFAF(BitMapType) )
		       
#define PF_TRUECOLOR_AF ( PFAF(RedMask)  | PFAF(GreenMask)  | PFAF(BlueMask)  | PFAF(AlphaMask) | \
			  PFAF(RedShift) | PFAF(GreenShift) | PFAF(BlueShift) | PFAF(AlphaShift))
			  
#define PF_PALETTE_AF ( PFAF(CLUTMask) | PFAF(CLUTShift) | PFAF(RedMask) | PFAF(GreenMask) | \
			PFAF(BlueMask) )
		       
#define PFAO(x) (aoHidd_PixFmt_ ## x)
  
/****************************************************************************************/

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf,
    	    	       ULONG ATTRCHECK(pixfmt), struct class_static_data *csd)
{
    IPTR attrs[num_Hidd_PixFmt_Attrs] = {0};
    struct Library *OOPBase = csd->cs_OOPBase;

    if (0 != OOP_ParseAttrs(tags, attrs, num_Hidd_PixFmt_Attrs,
    	    	    	    &ATTRCHECK(pixfmt), HiddPixFmtAttrBase))
    {
	D(bug("!!! parse_pixfmt_tags: ERROR PARSING TAGS THROUGH OOP_ParseAttrs !!!\n"));
	return FALSE;
    }

    if (PF_COMMON_AF != (PF_COMMON_AF & ATTRCHECK(pixfmt)))
    {
	D(bug("!!! parse_pixfmt_tags: Missing PixFmt attributes passed to parse_pixfmt_tags(): %x !!!\n", ATTRCHECK(pixfmt)));
	return FALSE;
    }
    
    /* Set the common attributes */
    pf->depth		= attrs[PFAO(Depth)];
    pf->size		= attrs[PFAO(BitsPerPixel)];
    pf->bytes_per_pixel	= attrs[PFAO(BytesPerPixel)];
    /* Fill in only real StdPixFmt specification. Special values (Native and Native32)
       are not allowed here */
    if (attrs[PFAO(StdPixFmt)] >= num_Hidd_PseudoStdPixFmt)
        pf->stdpixfmt = attrs[PFAO(StdPixFmt)];
    
    SET_PF_COLMODEL(  pf, attrs[PFAO(ColorModel)]);
    SET_PF_BITMAPTYPE(pf, attrs[PFAO(BitMapType)]);
    
    if (ATTRCHECK(pixfmt) & PFAF(SwapPixelBytes))
    {
    	SET_PF_SWAPPIXELBYTES_FLAG(pf, attrs[PFAO(SwapPixelBytes)]);
    }
    
    /* Set the colormodel specific stuff */
    switch (HIDD_PF_COLMODEL(pf))
    {
    	case vHidd_ColorModel_TrueColor:
	    /* Check that we got all the truecolor describing stuff */
	    if (PF_TRUECOLOR_AF != (PF_TRUECOLOR_AF & ATTRCHECK(pixfmt)))
	    {
		 D(bug("!!! Unsufficient true color format describing attrs to pixfmt in parse_pixfmt_tags() !!!\n"));
		 return FALSE;
	    }
	    
	    /* Set the truecolor stuff */
	    pf->red_mask	= attrs[PFAO(RedMask)];
	    pf->green_mask	= attrs[PFAO(GreenMask)];
	    pf->blue_mask	= attrs[PFAO(BlueMask)];
	    pf->alpha_mask	= attrs[PFAO(AlphaMask)];
	    
	    pf->red_shift	= attrs[PFAO(RedShift)];
	    pf->green_shift	= attrs[PFAO(GreenShift)];
	    pf->blue_shift	= attrs[PFAO(BlueShift)];
	    pf->alpha_shift	= attrs[PFAO(AlphaShift)];
	    break;
	
	case vHidd_ColorModel_Palette:
	case vHidd_ColorModel_StaticPalette:
	    if ( PF_PALETTE_AF != (PF_PALETTE_AF & ATTRCHECK(pixfmt)))
	    {
		 D(bug("!!! Unsufficient palette format describing attrs to pixfmt in parse_pixfmt_tags() !!!\n"));
		 return FALSE;
	    }
	    
	    /* set palette stuff */
	    pf->clut_mask	= attrs[PFAO(CLUTMask)];
	    pf->clut_shift	= attrs[PFAO(CLUTShift)];

	    pf->red_mask	= attrs[PFAO(RedMask)];
	    pf->green_mask	= attrs[PFAO(GreenMask)];
	    pf->blue_mask	= attrs[PFAO(BlueMask)];

	    break;
	    
    } /* shift (colormodel) */
    
    return TRUE;
}

/****************************************************************************************/

/* 
    Create an empty object and initialize it the "ugly" way. This only works with
    CLID_Hidd_PixFmt and CLID_Hidd_Sync classes
*/

/****************************************************************************************/

static OOP_Object *create_and_init_object(OOP_Class *cl, UBYTE *data, ULONG datasize,
    	    	    	    	    	  struct class_static_data *csd)
{
    OOP_Object *o;
    struct Library *OOPBase = csd->cs_OOPBase;
			
    o = OOP_NewObject(cl, NULL, NULL);
    if (NULL == o)
    {
	D(bug("!!! UNABLE TO CREATE OBJECT IN create_and_init_object() !!!\n"));
	return NULL;
    }
	    
    memcpy(o, data, datasize);
    
    return o;
}

/****************************************************************************************/

static VOID delete_pixfmts(struct class_static_data *csd)
{
    struct Node *n, *safe;
    struct Library *OOPBase = csd->cs_OOPBase;

    ForeachNodeSafe(&csd->pflist, n, safe)
	OOP_DisposeObject((OOP_Object *)PIXFMT_OBJ(n));
}

/****************************************************************************************/

static BOOL create_std_pixfmts(struct class_static_data *csd)
{
    ULONG i;
    struct pixfmt_data *pf;
    
    memset(csd->std_pixfmts, 0, sizeof (OOP_Object *) * num_Hidd_StdPixFmt);
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++)
    {
        pf = (struct pixfmt_data *)create_and_init_object(csd->pixfmtclass, (UBYTE *)&stdpfs[i],  sizeof (stdpfs[i]), csd);

	if (!pf)
	{
	    D(bug("FAILED TO CREATE PIXEL FORMAT %d\n", i));
	    delete_pixfmts(csd);
	    ReturnBool("create_stdpixfmts", FALSE);
	}

	csd->std_pixfmts[i] = &pf->pf;
	/* We don't use semaphore protection here because we do this only during class init stage */
	pf->refcount = 1;
	AddTail((struct List *)&csd->pflist, (struct Node *)&pf->node);
    }
    ReturnBool("create_stdpixfmts", TRUE);
}

/****************************************************************************************/

static const CONST_STRPTR interfaces[NUM_ATTRBASES] =
{
    IID_Hidd_BitMap,
    IID_Hidd_BMHistogram,
    IID_Hidd_Gfx,
    IID_Hidd_GC,
    IID_Hidd_ColorMap,
    IID_HW,
    IID_Hidd,
    IID_Hidd_Overlay,
    IID_Hidd_Sync,
    IID_Hidd_PixFmt,
    IID_Hidd_PlanarBM,
    IID_Hidd_ChunkyBM
};

static ULONG ObtainAttrBases(OOP_AttrBase *bases, CONST_STRPTR const *interfaces, ULONG count, struct Library *OOPBase)
{
    ULONG i;
    ULONG failed = 0;
    
    for (i = 0; i < count; i++)
    {
    	bases[i] = OOP_ObtainAttrBase(interfaces[i]);
    	if (!bases[i])
    	    failed++;
    }
    
    return failed;
}

static void ReleaseAttrBases(OOP_AttrBase *bases, CONST_STRPTR const *interfaces, ULONG count, struct Library *OOPBase)
{
    ULONG i;
    
    for (i = 0; i < count; i++)
    {
    	if (bases[i])
    	    OOP_ReleaseAttrBase(interfaces[i]);
    }
}

static ULONG GetMethodBases(OOP_MethodID *bases, CONST_STRPTR const *interfaces, ULONG count, struct Library *OOPBase)
{
    ULONG i;
    ULONG failed = 0;

    for (i = 0; i < count; i++)
    {
    	bases[i] = OOP_GetMethodID(interfaces[i], 0);
    	if (bases[i] == -1)
    	    failed++;
    }

    return failed;
}

static int GFX_ClassInit(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    struct Library *OOPBase = csd->cs_OOPBase;
    OOP_Object *hwroot;

    D(bug("[HiddGfx] %s(0x%p)\n", __func__, LIBBASE));

    D(bug("[HiddGfx] %s: csd @ 0x%p\n", __func__, csd));

    D(bug("[HiddGfx] %s: GC class @ 0x%p\n", __func__, csd->gcclass));
    D(bug("[HiddGfx] %s: BitMap class @ 0x%p\n", __func__, csd->bitmapclass));

    csd->cs_GfxBase = NULL;
    NEWLIST(&csd->pflist);

    if (!(csd->cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY)))
    {
        D(bug("[HiddGfx] %s: failed to open utility.library\n", __func__));
	ReturnInt("init_gfxhiddclass", ULONG, FALSE);
    }
 
    if (ObtainAttrBases(csd->attrBases, interfaces, NUM_ATTRBASES, OOPBase))
    {
        D(bug("[HiddGfx] %s: Failed to obtain class Attribute Bases\n", __func__));
	ReturnInt("init_gfxhiddclass", ULONG, FALSE);
    }

    if (GetMethodBases(csd->methodBases, interfaces, NUM_METHODBASES, OOPBase))
    {
        D(bug("[HiddGfx] %s: Failed to obtain class Method Bases\n", __func__));
	ReturnInt("init_gfxhiddclass", ULONG, FALSE);
    }

    InitSemaphore(&csd->sema);
    InitSemaphore(&csd->pfsema);
    InitSemaphore(&csd->rgbconvertfuncs_sem);

    D(bug("[HiddGfx] %s: Creating std pixelfmts\n", __func__));
    create_std_pixfmts(csd);

    D(bug("[HiddGfx] %s: Prepairing Gfx HW Root\n", __func__));
    hwroot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if ((hwroot) && (HW_AddDriver(hwroot, csd->gfxhwclass, NULL)))
    {
        D(
         bug("[HiddGfx] %s: Initialization complete\n", __func__);
         bug("[HiddGfx] %s: Gfx HW Root @ 0x%p\n", __func__, csd->gfxhwinstance);
        )
        return TRUE;
    }

    ReturnInt("init_gfxhiddclass", BOOL, FALSE);    
}

/****************************************************************************************/

static int GFX_ClassFree(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    struct Library *OOPBase = csd->cs_OOPBase;
    
    EnterFunc(bug("free_gfxhiddclass(csd=%p)\n", csd));

    delete_pixfmts(csd);
    ReleaseAttrBases(csd->attrBases, interfaces, NUM_ATTRBASES, OOPBase);
    CloseLibrary(csd->cs_UtilityBase);

    ReturnInt("free_gfxhiddclass", BOOL, TRUE);
}

/****************************************************************************************/

ADD2INITLIB(GFX_ClassInit, -2)
ADD2EXPUNGELIB(GFX_ClassFree, -2)
