/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Gfx mode class
    Lang: English.
*/

#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
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
    Object *gfxhidd = NULL;
    
    ULONG width, height;
    
    HIDDT_StdPixFmt stdpf;
    
    BOOL gotwidth = FALSE, gotheight = FALSE;
    BOOL ok = TRUE;

    gfxhidd = (Object *)GetTagData(aHidd_GfxMode_GfxHidd, NULL, msg->attrList);    
    if (NULL == gfxhidd) {
	kprintf("!!! NO GFXHIDD SUPPLPIED TO GfxMode::New() !!\n");
	return NULL;
    }
    
    /* Get Object from superclass */
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
    	return NULL;
    
    data = INST_DATA(cl, o);

    /* Get the number of pixel fromats for this gfxmode */
    data->numpfs = 0;
    for (tstate = msg->attrList; (tag = NextTagItem((const struct TagItem **)&tstate));  ) {
    	if (	aHidd_GfxMode_PixFmtTags == tag->ti_Tag 
	     ||	aHidd_GfxMode_StdPixFmt  == tag->ti_Tag ) {
	     
	    data->numpfs ++;
	}
    }
    
    if (0 == data->numpfs) {
    	kprintf("!!! NO PIXEL FORMATS SUPPLIED TO GfxMode::New() !!!\n");
	ok = FALSE;
    }
    
    /* Allocate array for pixel formats */
    if (ok) {
	data->pfarray = AllocMem(sizeof (*data->pfarray) * data->numpfs, MEMF_CLEAR);
	if (NULL == data->pfarray)
    	    ok = FALSE;
    }
    
    
    if (ok) {
	ULONG pfidx = 0;
	
	for (tstate = msg->attrList; ok && (tag = NextTagItem((const struct TagItem **)&tstate)); ) {
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
		    
			stdpf = (HIDDT_StdPixFmt)tag->ti_Data;
			data->pfarray[pfidx ++] = CSD(cl)->std_pixfmts[tag->ti_Data - num_Hidd_PseudoPixFmt];
			break;
		 
		    case aoHidd_GfxMode_PixFmtTags:
			data->pfarray[pfidx] = HIDD_Gfx_RegisterPixFmt(gfxhidd, (struct TagItem *)tag->ti_Data);
			if (NULL == data->pfarray[pfidx]) {
			    ok = FALSE;
			    break;
			}
			pfidx ++;
			break;
		    
	    	} /*switch */
	    
	    } /* if (is gfxmode attr) */
	
	} /* for (all attributes) */
		
    } /* if (ok) */
    
    if (ok) {
    
	if (gotwidth && gotheight) {
    
	    data->width		= width;
	    data->height	= height;
	    data->gfxhidd	= gfxhidd;
	    
	} else ok = FALSE;
	
    }
    
    if (!ok) {
	MethodID dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	
	CoerceMethod(cl, o, (Msg)&dispose_mid);
	o = NULL;
    }
    
    
    return o;
     
}     


static VOID gfxmode_dispose(Class *cl, Object *o, Msg msg)
{
    struct gfxmode_data *data;
    ULONG i;
     
    data = INST_DATA(cl, o);
    
    if (NULL != data->pfarray) {
     
	for (i = 0; i < data->numpfs; i ++) {
    
    	    if (NULL != data->pfarray[i]) {
		HIDD_Gfx_ReleasePixFmt(data->gfxhidd, data->pfarray[i]);
	    }
	}
    }
     
    DoSuperMethod(cl, o, (Msg)msg);
     
    return;
}

static VOID gfxmode_get(Class *cl, Object *o, struct pRoot_Get *msg)
{
    struct gfxmode_data *data;
    
    ULONG idx;
    
kprintf("gfxmode::get()\n");
    data = INST_DATA(cl, o);
    
    if (IS_GFXMODE_ATTR(msg->attrID, idx)) {
    	switch (idx) {
	    case aoHidd_GfxMode_Width:
	    	*msg->storage = data->width;
		break;

	    case aoHidd_GfxMode_Height:
	    	*msg->storage = data->height;
		break;
		
	    case aoHidd_GfxMode_NumPixFmts:
	    	*msg->storage = data->numpfs;
		break;
		
	     default:
	     	kprintf("TRYING TO GET UNKNOWN ATTR FROM GFXMODE CLASS\n");
		break;

	}
    
    } else {
    	kprintf("!!! TRYING TO GET UNKNOWN ATTR FROM GFXMODE OBJECTS\n");
    	DoSuperMethod(cl, o, (Msg)msg);
    }
    
    return;
    
}

Object *gfxmode_lookuppixfmt(Class *cl, Object *o, struct pHidd_GfxMode_LookupPixFmt *msg)
{
    struct gfxmode_data *data;
    
    data = INST_DATA(cl, o);
    
    if (msg->pixFmtNo >= data->numpfs) {
    	kprintf("!!! TO LARGE IDX IN CALL TO GfxMode::LookupPixFmt() !!!\n");
	return NULL;
    }
    
    return data->pfarray[msg->pixFmtNo];
}

/*** init_gfxmodeclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS 3
#define NUM_GFXMODE_METHODS 1

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
        {(IPTR (*)())gfxmode_lookuppixfmt, 	moHidd_GfxMode_LookupPixFmt	},
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
