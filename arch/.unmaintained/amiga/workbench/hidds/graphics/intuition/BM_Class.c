/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics-amiga-intui bitmap class implementation.
    Lang: english
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <intuition/screens.h>

#include <oop/oop.h>

#include <hidd/graphics.h>
#include <hidd/graphics-amiga-intuition.h>

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
    struct TagItem tags[] = {
                             {aHidd_BitMap_AllocBuffer, FALSE      },
                             {TAG_MORE                , (IPTR) NULL}
                            };

    struct HIDDBitMapAmigaIntuiData *data;
    struct TagItem *tag, *tstate, *newTagList;
    BOOL   ok = FALSE;

    struct pRoot_New               new_msg;
    struct pHidd_BitMap_PrivateSet private_set_msg;

    ULONG width, height, depth, format;
    BOOL  displayable;

    EnterFunc(bug("BitMapAmigaIntui::New()\n"));

    new_msg.mID         = GetMethodID(IID_Root, moRoot_New);
    private_set_msg.mID = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PrivateSet);


    displayable = (BOOL) GetTagData(aHidd_BitMap_Displayable, 0, msg->attrList);
    if(!displayable)
    {
        /* let the superclass do all work */

        obj = (Object *) DoSuperMethod(cl, obj, (Msg) msg);
        if(obj) ok = TRUE;
    }
    else
    {
        /*
            aHidd_BitMap_AllocBuffer and aHidd_BitMap_Displayable must be
            FALSE. To make this easy first a copy of the taglist is
            created. In the new list Hidd_BitMap_Displayable is set to
            FALSE. If the taglist contains a "aHidd_BitMap_AllocBuffer"
            tag then ti_Data is set to FALSE else tags(see above) is
            connected with the new list.

            NOTE: If msg.attrList contains more than one
                  "aHidd_BitMap_AllocBuffer" we have a problem.
        */

        newTagList = CloneTagItems(msg->attrList);
        if(newTagList)
        {
            tag = FindTagItem(aHidd_BitMap_Displayable, newTagList);
            if(tag)
            {
                tag->ti_Data     = FALSE;
            }

            tag = FindTagItem(aHidd_BitMap_AllocBuffer, newTagList);
            if(tag)
            {
                tag->ti_Data     = FALSE;
                new_msg.attrList = newTagList;
            }
            else
            {
                tags[1].ti_Data  = (IPTR) newTagList;
                new_msg.attrList = tags;
            }

            obj  = (Object *) DoSuperMethod(cl, obj, (Msg) &new_msg);

            FreeTagItems(newTagList);
        }

        if(obj)
        {
            data = INST_DATA(cl, obj);
        
            /* clear all data and set some default values */
            memset(data, 0, sizeof(struct HIDDBitMapAmigaIntuiData));

            GetAttr(obj, aHidd_BitMap_Width,  &width);
            GetAttr(obj, aHidd_BitMap_Height, &height);
            GetAttr(obj, aHidd_BitMap_Depth,  &depth);
            GetAttr(obj, aHidd_BitMap_Format, &format);

            if(format == vHIDD_BitMap_Format_Planar)
            {
                /* only displayable support for planar format */

                struct TagItem tags[] =
                {
                    {SA_Left     , 0},
                    {SA_Top      , 0},
                    {SA_Width    , width},
                    {SA_Height   , height},
                    {SA_Depth    , depth},
                    {TAG_SKIP    , 1},      /* Use SA_DisplayID only wenn a display mode */
                    {SA_DisplayID, 0},      /* is specified                              */
                    {SA_Title    , (ULONG) "AROS graphics hidd"},
                    {SA_Type     , CUSTOMSCREEN},
            
                    /* Will be use later   */
                    /* SA_Quiet    , TRUE, */
                    /* SA_BitMap   , 0     On V37 double buffering could only
                                           implemented with a custom bitmap */
                    {TAG_DONE,   0}
                };
    
                data->screen = OpenScreenTagList(NULL, tags);
                if(data->screen)
                {
                    struct TagItem set[] =
                    {
                        {aHidd_BitMap_BaseAddress, (IPTR) &data->screen->RastPort.BitMap->Planes[0]},
                        {aHidd_BitMap_Format     , vHIDD_BitMap_Format_Planar},
                        {aHidd_BitMap_Displayable, TRUE                      },
                        {aHidd_BitMap_BytesPerRow, data->screen->RastPort.BitMap->BytesPerRow},
                        {TAG_END                 , 0                         }
                    };

                    private_set_msg.attrList = set;
                    DoSuperMethod(cl, obj, (Msg) &private_set_msg);
                    ok = TRUE;
                } /* if(data->screen) */
            } /* if(format == vHIDD_BitMap_Format_Planar) */
        } /* if(obj) */
    } /* if(!displayable) */

    /* free all on error */
    if(ok == FALSE)
    {
        if(obj) DisposeObject(obj);
        obj = NULL;
    }

    ReturnPtr("BitMapAmigaIntui::New", Object *, obj);
}


/*** BitMap::Dispose() ********************************************************/

static void bitmap_dispose(Class *cl, Object *obj, Msg *msg)
{
    struct HIDDBitMapAmigaIntuiData *data = INST_DATA(cl, obj);

    struct pHidd_BitMap_PrivateSet private_set_msg;


    EnterFunc(bug("BitMapAmigaIntui::Dispose()\n"));

    if(data->screen)
    {
        /*
           We use an own buffer so set the BaseAddress to NULL to prevent
           the superclass to free our own buffer.
        */

        struct TagItem set[] =
        {
            {aHidd_BitMap_BaseAddress, NULL},
            {TAG_END                 , 0   }
        };

        CloseScreen(data->screen);

        private_set_msg.mID      = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PrivateSet);
        private_set_msg.attrList = set;
        DoSuperMethod(cl, obj, (Msg) &private_set_msg);
    }

    DoSuperMethod(cl, obj, (Msg) msg);

    ReturnVoid("BitMapAmigaIntui::Dispose");
}


/*** init_bitmapclass *********************************************************/

#undef OOPBase
#undef SysBase

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   2
#define NUM_BITMAP_METHODS 0

Class *init_bitmapclass(struct class_static_data *csd)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_new    , moRoot_New    },
        {(IPTR (*)())bitmap_dispose, moRoot_Dispose},
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
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
        {aMeta_SuperID,        (IPTR) CLID_Hidd_BitMap},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_ID,             (IPTR) CLID_Hidd_BitMapAmigaIntui},
        {aMeta_InstSize,       (IPTR) sizeof(struct HIDDBitMapAmigaIntuiData)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_bitmapclassAmigaIntui(csd=%p)\n", csd));

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

    ReturnPtr("init_bitmapclassAmigaIntui", Class *,  cl);
}


/*** free_bitmapclass *********************************************************/

void free_bitmapclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_bitmapclassAmigaIntui(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->bitmapclass);
        DisposeObject((Object *) csd->bitmapclass);
        csd->bitmapclass = NULL;
        if(HiddBitMapAttrBase) ReleaseAttrBase(IID_Hidd_BitMap);
    }

    ReturnVoid("free_bitmapclassAmigaIntui");
}
