/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Sync info class
    Lang: English.
*/

#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <hidd/graphics.h>

#define DEBUG 0
#include <aros/debug.h>

#include "graphics_intern.h"


static AttrBase HiddSyncAttrBase = 0;

static struct ABDescr attrbases[] = {
    { IID_Hidd_Sync,	&HiddSyncAttrBase	},
    { NULL, 0UL }
};

Object *sync_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct sync_data * data;
    BOOL ok = FALSE;
    
    DECLARE_ATTRCHECK(sync);
    
    EnterFunc(bug("Sync::New()\n"));
    
    /* Get object from superclass */
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
	return NULL;

    /* If we got a NULL attrlist we just allocate an empty object an exit */
    if (NULL == msg->attrList)
	return o;

    data = INST_DATA(cl, o);
    
    if (!parse_sync_tags(msg->attrList, data, ATTRCHECK(sync), CSD(cl) )) {
	kprintf("!!! ERROR PARSING SYNC ATTRS IN Sync::New() !!!\n");
    } else {
	ok = TRUE;
    }
    
    if (!ok) {
	MethodID dispose_mid;
	
	dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	CoerceMethod(cl, o, (Msg)&dispose_mid);
	o = NULL;
    }
    return o;
}

static VOID sync_get(Class *cl, Object *o, struct pRoot_Get *msg)
{
    struct sync_data *data;
    
    ULONG idx;
    
    data = INST_DATA(cl, o);
    
    if (IS_SYNC_ATTR(msg->attrID, idx)) {
    	switch (idx) {
	    case aoHidd_Sync_PixelTime:
		*msg->storage = (IPTR)data->pixtime;
		break;
		
	    case aoHidd_Sync_PixelClock: {
		DOUBLE pixtime, pixclock;
		
		pixtime = (DOUBLE)data->pixtime;
		
		pixtime /= 1000000000000;	/* pixtime is in 10E-12 secs */
		pixclock = 1 / pixtime;		/* convert to Hz */
		*msg->storage = (ULONG)pixclock;
		break;
	    }
		
	    case aoHidd_Sync_LeftMargin:
		*msg->storage = (IPTR)data->left_margin;
		break;
		
	    case aoHidd_Sync_RightMargin:
		*msg->storage = (IPTR)data->right_margin;
		break;
		
	    case aoHidd_Sync_HSyncLength:
		*msg->storage = (IPTR)data->hsync_length;
		break;
		
	    case aoHidd_Sync_UpperMargin:
		*msg->storage = (IPTR)data->upper_margin;
		break;
		
	    case aoHidd_Sync_LowerMargin:
		*msg->storage = (IPTR)data->lower_margin;
		break;
		
	    case aoHidd_Sync_VSyncLength:
		*msg->storage = (IPTR)data->vsync_length;
		break;
		
	    case aoHidd_Sync_HDisp:
		*msg->storage = (IPTR)data->hdisp;
		break;
		
	    case aoHidd_Sync_VDisp:
		*msg->storage = (IPTR)data->vdisp;
		break;
		
	    case aoHidd_Sync_HSyncStart:
		*msg->storage = (IPTR)(data->hdisp + data->right_margin);
		break;
		
	    case aoHidd_Sync_HSyncEnd:
		*msg->storage = (IPTR)(data->hdisp + data->right_margin + data->hsync_length);
		break;
		
	    case aoHidd_Sync_HTotal:
		*msg->storage = (IPTR)(data->hdisp + data->right_margin + data->hsync_length + data->left_margin);
		break;
		
	    case aoHidd_Sync_VSyncStart:
		*msg->storage = (IPTR)(data->vdisp + data->lower_margin);
		break;
		
	    case aoHidd_Sync_VSyncEnd:
		*msg->storage = (IPTR)(data->vdisp + data->lower_margin + data->vsync_length);
		break;
		
	    case aoHidd_Sync_VTotal:
		*msg->storage = (IPTR)(data->vdisp + data->lower_margin + data->vsync_length + data->upper_margin);
		break;
		
	     default:
	     	kprintf("!!! TRYING TO GET UNKNOWN ATTR FROM SYNC OBJECT !!!\n");
		break;

	}
    
    } else {
    	kprintf("!!! TRYING TO GET UNKNOWN ATTR FROM SYNC OBJECT !!!\n");
    	DoSuperMethod(cl, o, (Msg)msg);
    }
    
    return;
    
}


/*** init_syncclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS 3
#define NUM_SYNC_METHODS 0
Class *init_syncclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())sync_new, 	moRoot_New	},
        {(IPTR (*)())sync_get,	moRoot_Get	},
	{ NULL, 0UL }
    };
    
    struct MethodDescr sync_descr[NUM_SYNC_METHODS + 1] = {
	{ NULL, 0UL }
    };
        
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       	, NUM_ROOT_METHODS},
        {sync_descr,  	IID_Hidd_Sync	, NUM_SYNC_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_InstSize,       (IPTR) sizeof (struct sync_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_syncclass(csd=%p)\n", csd));

    if(MetaAttrBase) {
	if (ObtainAttrBases(attrbases)) {

    	    cl = NewObject(NULL, CLID_HiddMeta, tags);
    	    if(NULL != cl) {
        	D(bug("Sync class ok\n"));
        	csd->syncclass = cl;
        	cl->UserData     = (APTR) csd;
            }
        }
    } /* if(MetaAttrBase) */
    
    if (NULL == cl)
	free_syncclass(csd);

    ReturnPtr("init_syncclass", Class *,  cl);
}


/*** free_syncclass *********************************************************/

void free_syncclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_syncclass(csd=%p)\n", csd));

    if(NULL != csd) {
	if (NULL != csd->syncclass) {
	    DisposeObject((Object *) csd->syncclass);
            csd->syncclass = NULL;
	}
    }
    ReleaseAttrBases(attrbases);

    ReturnVoid("free_syncclass");
}
