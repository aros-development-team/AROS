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


Object *gfxmode_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct gfxmode_data * data;
    struct TagItem *tag, *tstate;
    
    ULONG width, height;
    UWORD depth;
    
    HIDDT_StdPixFmt stdpf;
    Object * pfptr;
    
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
		    
		    if (gotpf) /* ERROR: Allready got pf */
		    	return NULL;
		    stdpf = (HIDDT_StdPixFmt)tag->ti_Data;
		      	
#warning Implement this
/*		     CONVERT TO STANDARD PIXELFORMAT */
		    
		    gotpf = TRUE;  
		    break;
		 
		 
		case aoHidd_GfxMode_PixFmtPtr:
		    if (gotpf) /* ERROR: Allready got pf */
		    	return NULL;
		    
		    pfptr = (Object *)tag->ti_Data;
		    
		    gotpf = TRUE;
		    break;
		    
		case aoHidd_GfxMode_Depth:
		    depth = (UWORD)tag->ti_Data;
		    break;
		    
		    
		    
	    
	    } /*switch */
	    
	} /* if (is gfxmode attr) */
	
    } /* for (all attributes) */
    
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
     	return NULL;
     
     /* Get the attributes */
    data = INST_DATA(cl, o);
    
    data->width		= width;
    data->height	= height;
    data->pixfmt  	= pfptr;
    data->depth		= depth;
    
    return o;
     
}     
     

/*** init_gfxmodeclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   1
#define NUM_GFXMODE_METHODS 0

Class *init_gfxmodeclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())gfxmode_new    , moRoot_New    },
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
        {aMeta_ID,             (IPTR) CLID_Hidd_GfxMode},
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
            HiddGfxModeAttrBase = ObtainAttrBase(IID_Hidd_GfxMode);
            if(HiddGfxModeAttrBase)
            {
                AddClass(cl);
            }
            else
            {
			
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

    if(csd)
    {
        RemoveClass(csd->gfxmodeclass);
        if (csd->gfxmodeclass) {
	    DisposeObject((Object *) csd->gfxmodeclass);
            csd->gfxmodeclass = NULL;
	}
        if(HiddGfxModeAttrBase)
	    ReleaseAttrBase(IID_Hidd_GfxMode);
    }

    ReturnVoid("free_gfxmodeclass");
}
