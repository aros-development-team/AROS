/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Pixelformat class
    Lang: English.
*/

#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <hidd/graphics.h>

#define DEBUG 1
#include <aros/debug.h>

#include "graphics_intern.h"

static AttrBase HiddPixFmtAttrBase = 0;

struct pixfmt_data {
     HIDDT_PixelFormat pf;
    
};


#define GOT_ATTR(code) ((attrcheck & (1L << aoHidd_PixFmt_ ## code) == (1L << aoHidd_PixFmt_ ## code)))
#define FOUND_ATTR(code)  attrcheck |= (aoHidd_PixFmt_ ## code);


Object *pixfmt_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct pixfmt_data * data;
    struct TagItem *tag, *tstate;
    
    ULONG attrcheck;
    
    ULONG graphtype, depth, bytespp, bitspp;
    HIDDT_StdPixFmt stdpf;
    UWORD redshift, greenshift, blueshift, alphashift, clutshift;
    HIDDT_Pixel redmask, greenmask, bluemask, alphamask, clutmask;
    
    HIDDT_PixelFormat *pf;

kprintf("PIXFMT::NEW()\n");    
    for (tstate = msg->attrList; (tag = NextTagItem(&tstate)); ) {
    	ULONG idx;
	if (IS_PIXFMT_ATTR(tag->ti_Tag, idx)) {
	
	    switch (idx) {
	    	case aoHidd_PixFmt_GraphType:
		    graphtype = (ULONG)tag->ti_Data;
		    FOUND_ATTR(GraphType);
		    break;
		    
		case aoHidd_PixFmt_RedShift:
		    redshift = (ULONG)tag->ti_Data;
		    FOUND_ATTR(RedShift);
		    break;
		    
		case aoHidd_PixFmt_BlueShift:
		    blueshift = (ULONG)tag->ti_Data;
		    FOUND_ATTR(BlueShift);
		    break;
		    
		case aoHidd_PixFmt_GreenShift:
		    greenshift = (ULONG)tag->ti_Data;
		    FOUND_ATTR(GreenShift);
		    break;
		    
		case aoHidd_PixFmt_AlphaShift:
		    alphashift = (ULONG)tag->ti_Data;
		    FOUND_ATTR(AlphaShift);
		    break;
		    
		case aoHidd_PixFmt_RedMask:
		    redmask = (ULONG)tag->ti_Data;
		    FOUND_ATTR(RedMask);
		    break;

		case aoHidd_PixFmt_GreenMask:
		    greenmask = (ULONG)tag->ti_Data;
		    FOUND_ATTR(GreenMask);
		    break;

		case aoHidd_PixFmt_BlueMask:
		    bluemask = (ULONG)tag->ti_Data;
		    FOUND_ATTR(BlueMask);
		    break;

		case aoHidd_PixFmt_AlphaMask:
		    alphamask = (ULONG)tag->ti_Data;
		    FOUND_ATTR(AlphaMask);
		    break;

		case aoHidd_PixFmt_StdPixFmt:
		    stdpf = (HIDDT_StdPixFmt)tag->ti_Data;
		    FOUND_ATTR(StdPixFmt);
		    break;
		    
		case aoHidd_PixFmt_Depth:
		    depth = (ULONG)tag->ti_Data;
		    FOUND_ATTR(Depth);
		    break;
		    
		case aoHidd_PixFmt_BitsPerPixel:
		    bitspp = (ULONG)tag->ti_Data;
		    FOUND_ATTR(BitsPerPixel);
		    break;
		    
		case aoHidd_PixFmt_BytesPerPixel:
		    bytespp = (ULONG)tag->ti_Data;
		    FOUND_ATTR(BytesPerPixel);
		    break;
		    
		case aoHidd_PixFmt_CLUTShift:
		    clutshift = (ULONG)tag->ti_Data;
		    FOUND_ATTR(CLUTShift);
		    break;
		    
		case aoHidd_PixFmt_CLUTMask:
		    clutmask = (ULONG)tag->ti_Data;
		    FOUND_ATTR(CLUTMask);
		    break;
		    
	    
	    } /*switch */
	    
	} /* if (is pixfmt attr) */
	
    } /* for (all attributes) */
    
#warning Implement this
    /* If we have supplied a std pixfmt, then just use this to create the PixMt */
    
    /* Check that we have the necessary attributes */
    if ( ! ( GOT_ATTR(GraphType) && GOT_ATTR(Depth)
    	&& GOT_ATTR(BitsPerPixel) && GOT_ATTR(BytesPerPixel) ) ) {
    	kprintf("!!! GraphType, Depth, BitsPerPixel or BytesPerPixel not supplied to PixFmt\n");
	return NULL;
    }
    
    switch (graphtype) {
    	case vHidd_GT_TrueColor:
	    /* Check that we got all the truecolor describing stuff */
	    if ( ! (GOT_ATTR(RedMask)   && GOT_ATTR(GreenMask)
	    	 && GOT_ATTR(BlueMask)  && GOT_ATTR(AlphaMask)
		 && GOT_ATTR(RedShift)  && GOT_ATTR(GreenShift)
		 && GOT_ATTR(BlueShift) && GOT_ATTR(AlphaShift) ) ) {
		 
		 
		 kprintf("!!! Unsufficient true color format describing attrs to pixfmt\n");
		 return NULL;
	    }
	    break;
	
	case vHidd_GT_Palette:
	    if ( ! (GOT_ATTR(CLUTShift) && GOT_ATTR(CLUTMask)) ) {
		 kprintf("!!! Unsufficient palette format describing attrs to pixfmt\n");
		 return NULL;
	    }
	    break;
	
	case vHidd_GT_StaticPalette:
	
	    break;
	
    }
    	
    
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
     	return NULL;
     
     /* Get the attributes */
    data = INST_DATA(cl, o);
    
    pf = &data->pf;
    
    pf->flags	= graphtype;
    pf->depth	= depth;
    pf->size	= bitspp;
    pf->bytes_per_pixel = bytespp;
    
    
    switch (graphtype) {
    	case vHidd_GT_TrueColor:
	    pf->red_shift	= redshift;
	    pf->green_shift	= greenshift;
	    pf->blue_shift 	= blueshift;
	    pf->alpha_shift	= alphashift;
    
	    pf->red_mask	= redmask;
	    pf->green_mask	= greenmask;
	    pf->blue_mask	= bluemask;
	    pf->alpha_mask	= alphamask;
	    break;
	     
	case vHidd_GT_Palette:
	    pf->clut_shift	= clutshift;
	    pf->clut_mask	= clutmask;
	    break;
	    
	case vHidd_GT_StaticPalette:
	    break;
    
    }
    
    
    return o;
     
}     

static VOID pixfmt_get(Class *cl, Object *o, struct pRoot_Get *msg)
{
    HIDDT_PixelFormat *pf;
    struct pixfmt_data *data;
    ULONG idx;
    
    
    data = INST_DATA(cl, o);
    pf = &data->pf;
    
    if (IS_PIXFMT_ATTR(msg->attrID, idx)) {
    	switch (idx) {
	    case aoHidd_PixFmt_RedShift:
	    	*msg->storage = pf->red_shift;
	    	break;
	
	    case aoHidd_PixFmt_GreenShift:
	    	*msg->storage = pf->green_shift;
	    	break;
	
	    case aoHidd_PixFmt_BlueShift:
	    	*msg->storage = pf->blue_shift;
	    	break;
	
	    case aoHidd_PixFmt_AlphaShift:
	    	*msg->storage = pf->alpha_shift;
	    	break;
	
	    case aoHidd_PixFmt_RedMask:
	    	*msg->storage = pf->red_mask;
	    	break;
	
	    case aoHidd_PixFmt_GreenMask:
	    	*msg->storage = pf->green_mask;
	    	break;
	
	    case aoHidd_PixFmt_BlueMask:
	    	*msg->storage = pf->blue_mask;
	    	break;
	
	    case aoHidd_PixFmt_AlphaMask:
	    	*msg->storage = pf->alpha_mask;
	    	break;
	
	    case aoHidd_PixFmt_CLUTShift:
	    	*msg->storage = pf->clut_shift;
	    	break;
	
	    case aoHidd_PixFmt_CLUTMask:
	    	*msg->storage = pf->clut_mask;
	    	break;
	
	    case aoHidd_PixFmt_Depth:
	    	*msg->storage = pf->depth;
	    	break;
	
	    case aoHidd_PixFmt_BitsPerPixel:
	    	*msg->storage = pf->size;
	    	break;
	
	    case aoHidd_PixFmt_BytesPerPixel:
	    	*msg->storage = pf->bytes_per_pixel;
	    	break;
	    
	    case aoHidd_PixFmt_StdPixFmt:
	    	*msg->storage = pf->stdpixfmt;
	    	break;
	    
	    case aoHidd_PixFmt_GraphType:
	    	*msg->storage = pf->flags;
	    	break;
	    
	    default:
	    	kprintf("TRYING TO GET UNKNOWN PIXFMT ATTR\n");
		break;
	}
    
    } else {
    	DoSuperMethod(cl, o, (Msg)msg);
    }
    
    return;    
}
     

/*** init_pixfmtclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   2
#define NUM_PIXFMT_METHODS 0

Class *init_pixfmtclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())pixfmt_new    , moRoot_New	},
        {(IPTR (*)())pixfmt_get    , moRoot_Get	},
	{ NULL, 0UL }
    };
    
    struct MethodDescr pixfmt_descr[NUM_PIXFMT_METHODS + 1] = {
	{ NULL, 0UL }
    };
        
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {pixfmt_descr,  IID_Hidd_PixFmt, NUM_PIXFMT_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_ID,             (IPTR) CLID_Hidd_PixFmt},
        {aMeta_InstSize,       (IPTR) sizeof (struct pixfmt_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_pixfmtclass(csd=%p)\n", csd));

    if(MetaAttrBase)
    {
        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("PixFmt class ok\n"));
            csd->pixfmtclass = cl;
            cl->UserData     = (APTR) csd;
            
            /* Get attrbase for the PixFmt interface */
            HiddPixFmtAttrBase = ObtainAttrBase(IID_Hidd_PixFmt);
            if(HiddPixFmtAttrBase)
            {
                AddClass(cl);
            }
            else
            {
			
                free_pixfmtclass(csd);
                cl = NULL;
            }
        }
    } /* if(MetaAttrBase) */

    ReturnPtr("init_pixfmtclass", Class *,  cl);
}


/*** free_pixfmtclass *********************************************************/

void free_pixfmtclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_pixfmtclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->pixfmtclass);
        if (csd->pixfmtclass) {
	    DisposeObject((Object *) csd->pixfmtclass);
            csd->pixfmtclass = NULL;
	}
        if(HiddPixFmtAttrBase)
	    ReleaseAttrBase(IID_Hidd_PixFmt);
    }

    ReturnVoid("free_pixfmtclass");
}
