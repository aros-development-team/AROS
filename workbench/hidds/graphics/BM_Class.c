/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics bitmap class implementation.
    Lang: english
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

static AttrBase HiddBitMapAttrBase = 0;

/*** BitMap::New() ************************************************************/

static Object *bitmap_new(Class *cl, Object *obj, struct pRoot_New *msg)
{
    struct HIDDBitMapData *data;
    struct TagItem *tag, *tstate;

    BOOL   ok = FALSE;
    BOOL   allocBuffer = TRUE;
    UBYTE  alignOffset = 15; /* 7 = Byte-, 15 = Word-, 31 = Longword align etc. */
    UBYTE  alignDiv    =  2; /* 1 = Byte-,  2 = Word-,  4 = Longword align etc. */
    ULONG  i;
    UBYTE  **plane;


    EnterFunc(bug("BitMap::New()\n"));

    obj  = (Object *) DoSuperMethod(cl, obj, (Msg) msg);

    if(obj)
    {
        data = INST_DATA(cl, obj);
    
        /* clear all data and set some default values */
        memset(data, 0, sizeof(struct HIDDBitMapData));
        data->width       = 320;
        data->height      = 200;
        data->depth       = 8;
        data->displayable = FALSE;
        data->format      = HIDDV_BitMap_Format_Planar;

        tstate = msg->attrList;
        while((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            if(IS_BM_ATTR(tag->ti_Tag, idx))
            {
                switch(idx)
                {
                    case aoHidd_BitMap_Width       : data->width       = tag->ti_Data; break;
                    case aoHidd_BitMap_Height      : data->height      = tag->ti_Data; break;
                    case aoHidd_BitMap_Depth       : data->depth       = tag->ti_Data; break;
                    case aoHidd_BitMap_Displayable : data->displayable = (BOOL) tag->ti_Data; break;
                    case aoHidd_BitMap_Format      : data->format      = tag->ti_Data; break;
                    case aoHidd_BitMap_AllocBuffer : allocBuffer       = (BOOL) tag->ti_Data; break;

                    default: D(bug("  unknown attribute %li\n", tag->ti_Data)); break;
                } /* switch tag */
            } /* if (is BM attr) */
        }


        if(data->displayable)
        {
            /* no support for a displayable bitmap */
            ok = FALSE;
        }
        else
        {
            /* bitmap is not displayable */

            if(data->format & HIDDV_BitMap_Format_Chunky)
            {
                data->bytesPerPixel = (data->depth + 7) / 8;
                data->bytesPerRow   = data->bytesPerPixel * ((data->width + alignOffset) / alignDiv);
                if(allocBuffer)
                {
                    data->buffer = AllocVec(data->height * data->bytesPerRow, MEMF_CLEAR | MEMF_PUBLIC);
                    if(data->buffer) ok = TRUE;
                }
                else
                {
                    ok = TRUE;
                }
            }
            else
            {
                /* Planar format: buffer is a pointer to an array of planepointers */

                data->bytesPerRow = (data->width + alignOffset) / alignDiv;

                if(allocBuffer)
                {
                    data->buffer = AllocVec(data->depth * sizeof(UBYTE *), MEMF_CLEAR | MEMF_PUBLIC);
                    if(data->buffer)
                    {
                        plane = (UBYTE **) data->buffer;
                        ok    = TRUE;
    
                        for(i = 0; (i < data->depth) && (ok == TRUE); i++)
                        {
                            *plane = AllocVec(data->height * data->bytesPerRow, MEMF_CLEAR | MEMF_PUBLIC);
                            if(*plane == NULL) ok = FALSE;
                            plane++;
                        }
                    }
                }
                else
                {
                    ok = TRUE;
                }

            }
        }
    } /* if(obj) */


    /* free all on error */
    if(ok == FALSE)
    {
        if(obj) DisposeObject(obj);
        obj = NULL;
    }


    ReturnPtr("BitMap::New", Object *, obj);
}


/*** BitMap::Dispose() ********************************************************/

static void bitmap_dispose(Class *cl, Object *obj, Msg *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);

    ULONG i;
    UBYTE **plane;

    EnterFunc(bug("BitMap::Dispose()\n"));

    if(data->format & HIDDV_BitMap_Format_Planar)
    {
        if(data->buffer)
        {
            /* buffer is a pointer to an array of planepointer */
            plane = (UBYTE **) data->buffer;

            for(i = 0; (i < data->depth) && (*plane != NULL); i++)
            {
                FreeVec(*plane);
                plane++;
            }
        }
    }

    FreeVec(data->buffer);

    DoSuperMethod(cl, obj, (Msg) msg);

    ReturnVoid("BitMap::Dispose");
}


/*** BitMap::Get() ************************************************************/

static VOID bitmap_get(Class *cl, Object *obj, struct pRoot_Get *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);
    ULONG  idx;

    EnterFunc(bug("BitMap::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if(IS_BM_ATTR(msg->attrID, idx))
    {
        switch(idx)
        {
            case aoHidd_BitMap_Width       : *msg->storage = data->width; D(bug("  width: %i\n", data->width)); break;
            case aoHidd_BitMap_Height      : *msg->storage = data->height; break;
            case aoHidd_BitMap_Depth       : *msg->storage = data->depth; break;
            case aoHidd_BitMap_Displayable : *msg->storage = (ULONG) data->displayable; break;
            case aoHidd_BitMap_Format      : *msg->storage = data->format; break;
            case aoHidd_BitMap_BitMap      : *msg->storage = (ULONG) data->buffer; break;
    
            default: DoSuperMethod(cl, obj, (Msg) msg);
        }
    }

    ReturnVoid("BitMap::Get");
}


/*** BitMap::PrivateSet() *****************************************************/

/*

   This makes it easier to create a subclass of the graphics hidd.
   It only allowd to use this method in the p_RootNew method of a
   bitmap subclass.

*/

static VOID bitmap_private_set(Class *cl, Object *obj, struct pHidd_BitMap_PrivateSet *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);
    struct TagItem *tag, *tstate;
    ULONG  idx;

    EnterFunc(bug("BitMap::PrivateSet()\n"));

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_BitMap_Width         : data->width         = tag->ti_Data; break;
                case aoHidd_BitMap_Height        : data->height        = tag->ti_Data; break;
                case aoHidd_BitMap_Depth         : data->depth         = tag->ti_Data; break;
                case aoHidd_BitMap_Displayable   : data->displayable   = (BOOL) tag->ti_Data; break;
                case aoHidd_BitMap_Format        : data->format        = tag->ti_Data; break;
                case aoHidd_BitMap_BytesPerRow   : data->bytesPerRow   = tag->ti_Data; break;
                case aoHidd_BitMap_BytesPerPixel : data->bytesPerPixel = tag->ti_Data; break;
                case aoHidd_BitMap_ColorTab      : data->colorTab      = (APTR) tag->ti_Data; break;
                case aoHidd_BitMap_BaseAddress   : data->buffer        = (APTR) tag->ti_Data; break;
            }
        }
    }

    ReturnVoid("BitMap::PrivateSet");
}


/*** init_bitmapclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 1

Class *init_bitmapclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_new    , moRoot_New    },
        {(IPTR (*)())bitmap_dispose, moRoot_Dispose},
        {(IPTR (*)())bitmap_get    , moRoot_Get    },
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_private_set, moHidd_BitMap_PrivateSet},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {bitMap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Root},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_ID,             (IPTR) CLID_Hidd_BitMap},
        {aMeta_InstSize,       (IPTR) sizeof(struct HIDDBitMapData)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(csd=%p)\n", csd));

    if(MetaAttrBase)
    {
        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            csd->bitmapclass = cl;
            cl->UserData     = (APTR) csd;
            
            /* Get attrbase for the BitMap interface */
            HiddBitMapAttrBase = ObtainAttrBase(IID_Hidd_BitMap);
            if(HiddBitMapAttrBase)
            {
                AddClass(cl);
            }
            else
            {
                free_bitmapclass(csd);
                cl = NULL;
            }
        }
    } /* if(MetaAttrBase) */

    ReturnPtr("init_bitmapclass", Class *,  cl);
}


/*** free_bitmapclass *********************************************************/

void free_bitmapclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_bitmapclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->bitmapclass);
        if(csd->bitmapclass) DisposeObject((Object *) csd->bitmapclass);
        csd->bitmapclass = NULL;
        if(HiddBitMapAttrBase) ReleaseAttrBase(IID_Hidd_BitMap);
    }

    ReturnVoid("free_bitmapclass");
}
