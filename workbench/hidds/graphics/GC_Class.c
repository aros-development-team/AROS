/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics gc class implementation.
    Lang: english
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#define SDEBUG 0
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

static VOID gc_set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg);

#define IS_GC_ATTR(attr, idx) ( ( (idx) = (attr) - HiddGCAttrBase) < num_Hidd_GC_Attrs)

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddGCAttrBase;
static OOP_AttrBase HiddBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = {
    { IID_Hidd_GC,	&HiddGCAttrBase		},
    { IID_Hidd_BitMap,	&HiddBitMapAttrBase	},
    { NULL, NULL }
};

/*** GC::New() ************************************************************/

static OOP_Object *gc_new(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
    HIDDT_GC_Intern *data;


    EnterFunc(bug("GC::New()\n"));

    obj  = (OOP_Object *) OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);

    if(obj)
    {
        data = OOP_INST_DATA(cl, obj);
    
        /* clear all data and set some default values */

        memset(data, 0, sizeof (*data));

        data->fg        = 1;        /* foreground color                        */
        data->bg        = 0;        /* background color                        */
        data->drMode    = vHidd_GC_DrawMode_Copy;    /* drawmode               */
        data->font      = NULL;     /* current fonts                           */
        data->colMask   = ~0;       /* ColorMask prevents some color bits from changing*/
        data->linePat   = ~0;       /* LinePattern                             */
        data->planeMask = NULL;     /* Pointer to a shape bitMap               */

        /* Override defaults with user suplied attrs */

	OOP_SetAttrs(obj, msg->attrList);
/*        gc_set(cl, obj, &set_msg);*/

    } /* if(obj) */

    ReturnPtr("GC::New", OOP_Object *, obj);
}



/*** GC::Set() ************************************************************/

static VOID gc_set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    HIDDT_GC_Intern *data = OOP_INST_DATA(cl, obj);
    struct TagItem *tag, *tstate;
    ULONG  idx;

    EnterFunc(bug("GC::Set()\n"));

    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if(IS_GC_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_GC_Foreground : data->fg        = tag->ti_Data; break;
                case aoHidd_GC_Background : data->bg        = tag->ti_Data; break;
                case aoHidd_GC_DrawMode   : data->drMode    = tag->ti_Data; break;
                case aoHidd_GC_Font       : data->font      = (APTR) tag->ti_Data; break;
                case aoHidd_GC_ColorMask  : data->colMask   = tag->ti_Data; break;
                case aoHidd_GC_LinePattern: data->linePat   = (UWORD) tag->ti_Data; break;
                case aoHidd_GC_PlaneMask  : data->planeMask = (APTR) tag->ti_Data; break;

                case aoHidd_GC_ColorExpansionMode : data->colExp    = tag->ti_Data; break;
            }
        }
    }

    ReturnVoid("GC::Set");
}


/*** GC::Get() ************************************************************/

static VOID gc_get(OOP_Class *cl, OOP_Object *obj, struct pRoot_Get *msg)
{
    HIDDT_GC_Intern *data = OOP_INST_DATA(cl, obj);
    ULONG  idx;

    EnterFunc(bug("GC::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if(IS_GC_ATTR(msg->attrID, idx))
    {
        switch(idx)
        {

            case aoHidd_GC_Foreground : *msg->storage = data->fg; break;
            case aoHidd_GC_Background : *msg->storage = data->bg; break;
            case aoHidd_GC_DrawMode   : *msg->storage = data->drMode; break;
            case aoHidd_GC_Font       : *msg->storage = (ULONG) data->font; break;
            case aoHidd_GC_ColorMask  : *msg->storage = data->colMask; break;
            case aoHidd_GC_LinePattern: *msg->storage = data->linePat; break;
            case aoHidd_GC_PlaneMask  : *msg->storage = (ULONG) data->planeMask; break;
            case aoHidd_GC_ColorExpansionMode : *msg->storage = data->colExp; break;
	    default: OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg); break;
        }
    } else {
        OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);
    }

}

/*** GC::SetClipRect() ****************************************************/
VOID gc_setcliprect(OOP_Class *cl, OOP_Object *o, struct pHidd_GC_SetClipRect *msg)
{
     HIDDT_GC_Intern *data;
     
     data = OOP_INST_DATA(cl, o);
     
     data->clipX1 = msg->x1;
     data->clipY1 = msg->y1;
     data->clipX2 = msg->x2;
     data->clipY2 = msg->y2;
     
     data->doClip = TRUE;
}

VOID gc_unsetcliprect(OOP_Class *cl, OOP_Object *o, struct pHidd_GC_UnsetClipRect *msg)
{
     HIDDT_GC_Intern *data;
     
     data = OOP_INST_DATA(cl, o);
     
     data->doClip = FALSE;
    
}


/*** init_gcclass *************************************************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)
#define UtilityBase (csd->utilitybase)

#define NUM_ROOT_METHODS   3
#define NUM_GC_METHODS    2

OOP_Class *init_gcclass(struct class_static_data *csd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())gc_new         , moRoot_New    },
        {(IPTR (*)())gc_set         , moRoot_Set    },
        {(IPTR (*)())gc_get         , moRoot_Get    },
        {NULL, 0UL}
    };

    struct OOP_MethodDescr gc_descr[NUM_GC_METHODS + 1] =
    {
    	{(IPTR (*)())gc_setcliprect	, moHidd_GC_SetClipRect		},
    	{(IPTR (*)())gc_unsetcliprect	, moHidd_GC_UnsetClipRect	},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root        , NUM_ROOT_METHODS},
        {gc_descr  ,    IID_Hidd_GC     , NUM_GC_METHODS  },
        {NULL, NULL, 0}
    };

    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_ID,             (IPTR) CLID_Hidd_GC},
        {aMeta_InstSize,       (IPTR) sizeof(HIDDT_GC_Intern)},
        {TAG_DONE, 0UL}
    };
    
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_gcclass(csd=%p)\n", csd));

    if(MetaAttrBase) {
        if(OOP_ObtainAttrBases(attrbases))  {
            cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
            if(NULL != cl) {
                D(bug("GC class ok\n"));
                csd->gcclass = cl;
                cl->UserData = (APTR) csd;
                
                OOP_AddClass(cl);
		
            }
        }
    } /* if(MetaAttrBase) */
    
    if (NULL == cl)
	free_gcclass(csd);
    
    ReturnPtr("init_gcclass", OOP_Class *, cl);

}

/*** free_gcclass *************************************************************/

void free_gcclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_gcclass(csd=%p)\n", csd));

    if(NULL != csd) {
	if (NULL != csd->gcclass) {
    	    OOP_RemoveClass(csd->gcclass);
    	    OOP_DisposeObject((OOP_Object *) csd->gcclass);

    	    csd->gcclass = NULL;
	}
    }
    
    OOP_ReleaseAttrBases(attrbases);

    ReturnVoid("free_gcclass");
}

