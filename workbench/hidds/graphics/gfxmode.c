/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Gfx mode class
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

static AttrBase HiddGfxModeAttrBase = 0;
static AttrBase HiddPixFmtAttrBase  = 0;

static struct ABDescr attrbases[] = {
    { IID_Hidd_GfxMode,	&HiddGfxModeAttrBase	},
    { IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
    { NULL, 0UL }
};

#define IS_GFXMODE_ATTR(attr, idx) \
	( ( ( idx ) = (attr) - HiddGfxModeAttrBase) < num_Total_GfxMode_Attrs)

Object *gfxmode_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct gfxmode_data * data;
    struct TagItem *tag, *tstate;
    Object *pfobj = NULL;
    
    Object *gfxhidd = NULL;
    
    ULONG width, height;
    
    HIDDT_StdPixFmt stdpf;
    struct TagItem * pftags = NULL;
    
    BOOL gotwidth = FALSE, gotheight = FALSE, gotpf = FALSE;
    
    for (tstate = msg->attrList; (tag = NextTagItem(&tstate)); ) {
    	ULONG idx;
	
	if (IS_GFXMODE_ATTR(tag->ti_Tag, idx)) {
	    switch (idx) {
	    	case aoHidd_GfxMode_Width:
		    width = (ULONG)tag->ti_Data;
		    gotwidth = TRUE;
		    break;
		    
		case aoHidd_GfxMode_Height:
		    height = (ULONG)tag->ti_Data;
		    gotheight = TRUE;
		    break;
		    
		case aoHidd_GfxMode_StdPixFmt:
		    
		    if (gotpf) {/* ERROR: Allready got pf */
		    	kprintf("!!! GfxMode::New() duplicate PixFmt tags\n");
		    	return NULL;
		    }
		    stdpf = (HIDDT_StdPixFmt)tag->ti_Data;
		      	
#warning Implement this
/*		     CONVERT TO STANDARD PIXELFORMAT */
		    
		    gotpf = TRUE;  
		    break;
		 
		 
		case aoHidd_GfxMode_PixFmtTags:
		    if (gotpf) { /* ERROR: Allready got pf */
		    	kprintf("!!! GfxMode::New() duplicate PixFmt tags\n");
		    	return NULL;
		    }
		    
		    pftags = (struct TagItem *)tag->ti_Data;
		    
		    gotpf = TRUE;
		    break;
		    
		case aoHidd_GfxMode_GfxHidd:
		    gfxhidd = (Object *)tag->ti_Data;
		    break;
	    
	    } /*switch */
	    
	} /* if (is gfxmode attr) */
	
    } /* for (all attributes) */
    
    if (NULL == gfxhidd) {
    	kprintf("!!! GfxMode::New() No GfxHidd pointer supplied\n");
	return NULL;
    }
    
    if (!gotpf) {
    	kprintf("!!! GfxMode::New() No PixFmt description supplied\n");
	return NULL;
    }
    
    
    if (NULL != pftags) {
    	/* Register a pixfmt object */
	pfobj = HIDD_Gfx_RegisterPixFmt(gfxhidd, pftags);
	
    } else {
    	/* Try to get a standard pixel format */
	pfobj = CSD(cl)->std_pixfmts[stdpf - num_Hidd_PseudoPixFmt];
    }
    
    if (NULL == pfobj)
    	return NULL;

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
     	return NULL;
     
     
    
     /* Get the attributes */
    data = INST_DATA(cl, o);
    
    data->width		= width;
    data->height	= height;
    data->pixfmt  	= pfobj;
    
    return o;
     
}     


static VOID gfxmode_dispose(Class *cl, Object *o, Msg msg)
{
     struct gfxmode_data *data;
     
     data = INST_DATA(cl, o);
     
     HIDD_Gfx_ReleasePixFmt(data->gfxhidd, data->pixfmt);
     
     DoSuperMethod(cl, o, (Msg)msg);
     
     return;
}

static VOID gfxmode_get(Class *cl, Object *o, struct pRoot_Get *msg)
{
    struct gfxmode_data *data;
    
    ULONG idx;
    
    if (IS_GFXMODE_ATTR(msg->attrID, idx)) {
    	switch (idx) {
	    case aoHidd_GfxMode_Width:
	    	*msg->storage = data->width;
		break;

	    case aoHidd_GfxMode_Height:
	    	*msg->storage = data->height;
		break;
		
	     default:
	     	kprintf("TRYING TO GET UNKNOWN ATTR FROM GFXMODE CLASS\n");
		break;

	}
    
    } else if (IS_PIXFMT_ATTR(msg->attrID, idx)) {
    	/* Just as a service, we export the pixel format interface */
	HIDDT_PixelFormat *pf;
	
	pf = (HIDDT_PixelFormat *)data->pixfmt;
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
    
    
    
    
    data = INST_DATA(cl, o);
    
    
}

/*** init_gfxmodeclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS 3
#define NUM_GFXMODE_METHODS 0

Class *init_gfxmodeclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())gfxmode_new, 	moRoot_New	},
        {(IPTR (*)())gfxmode_dispose,	moRoot_Dispose	},
        {(IPTR (*)())gfxmode_get,	moRoot_Get	},
	{ NULL, 0UL }
    };
    
    struct MethodDescr gfxmode_descr[NUM_GFXMODE_METHODS + 1] = {
	{ NULL, 0UL }
    };
        
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {gfxmode_descr,  IID_Hidd_GfxMode, NUM_GFXMODE_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_InstSize,       (IPTR) sizeof (struct gfxmode_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_gfxmodeclass(csd=%p)\n", csd));

    if(MetaAttrBase)
    {
    

        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("GfxMode class ok\n"));
            csd->gfxmodeclass = cl;
            cl->UserData     = (APTR) csd;
            
            /* Get attrbase for the GfxMode interface */
	    if (!ObtainAttrBases(attrbases)) {
			
                free_gfxmodeclass(csd);
                cl = NULL;
            }
        }
    } /* if(MetaAttrBase) */

    ReturnPtr("init_gfxmodeclass", Class *,  cl);
}


/*** free_gfxmodeclass *********************************************************/

void free_gfxmodeclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_gfxmodeclass(csd=%p)\n", csd));
    
    ReleaseAttrBases(attrbases);

    if(csd)
    {
        RemoveClass(csd->gfxmodeclass);
        if (csd->gfxmodeclass) {
	    DisposeObject((Object *) csd->gfxmodeclass);
            csd->gfxmodeclass = NULL;
	}
    }

    ReturnVoid("free_gfxmodeclass");
}
