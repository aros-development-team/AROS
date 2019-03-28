/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <hidd/gfx.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/pointerclass.h>
#include <graphics/sprite.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "intuition_intern.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#include <aros/asmcall.h>

/***********************************************************************************/

/* Empty sprite */
const UWORD posctldata[] =
{
    0x0000,0x0000, /* posctl		     */
    0x0000,0x0000, /* Empty pixels, one line */
    0x0000,0x0000  /* Reserved		     */
};

/***********************************************************************************/

#define DEF_POINTER_DEPTH 4

IPTR PointerClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    OOP_MethodID HiddBitMapBase = ((struct IntIntuitionBase *)IntuitionBase)->ib_HiddBitMapBase;
    D(kprintf("[Pointer] %s()\n", __func__));

    if (cl)
    {
        struct TagItem spritetags[] =
        {
            {SPRITEA_Width        , 0   },
            {SPRITEA_OutputHeight , 0   },
            {SPRITEA_OldDataFormat, TRUE},
            {TAG_DONE        	    }
        };
        struct ExtSprite *sprite;
        struct TagItem *tags = msg->ops_AttrList;
        struct BitMap *bitmap = (struct BitMap *)GetTagData(POINTERA_BitMap, (IPTR)NULL, tags);
        
        /* Hack: we accept and pass on to AllocSpriteDataA() these attributes in order to
                 provide code reuse. Applications should not rely on it, this is private! */
        BOOL oldbitmap = GetTagData(SPRITEA_OldDataFormat, FALSE, tags);
        IPTR width = GetTagData(SPRITEA_Width, 16, tags);
        IPTR height = GetTagData(SPRITEA_OutputHeight, 0, tags);

        //ULONG xResolution = GetTagData(POINTERA_XResolution, POINTERXRESN_DEFAULT, tags);
        //ULONG yResolution = GetTagData(POINTERA_YResolution, POINTERYRESN_DEFAULT, tags);

        D(
            {
                struct TagItem *tagscan=tags;

                kprintf("[Pointer] %s: Pointer BitMap @ %p\n", __func__, bitmap);
                while(tagscan->ti_Tag != 0)
                {
                    kprintf("[Pointer] %s:   0x%08lx, %08lx\n", __func__, tagscan->ti_Tag, tagscan->ti_Data);
                    tagscan++;
                }
            }
         )

        if (bitmap)
        {
            if (oldbitmap) {
                spritetags[0].ti_Data = width;
                spritetags[1].ti_Data = height;
            } else {
                spritetags[0].ti_Data = GetTagData(POINTERA_WordWidth, ((GetBitMapAttr(bitmap, BMA_WIDTH) + 15) & ~15)>>4, tags) * 16;
                spritetags[1].ti_Tag = TAG_DONE;
            }
        }
        else
        {
            D(kprintf("[Pointer] %s: called without bitmap, using dummy sprite !\n", __func__));

            spritetags[0].ti_Data = 16;
            spritetags[1].ti_Data = 1; 
            bitmap = (struct BitMap *)posctldata;
        }

        sprite = AllocSpriteDataA(bitmap, spritetags);

        D(
          kprintf("[Pointer] %s: extSprite 0x%lx\n", __func__, sprite);
          kprintf("[Pointer] %s: MoveSprite data 0x%lx, height %ld, x %ld, y %ld, num %ld, wordwidth, 0x%lx, flags 0x%lx\n",
                  __func__,
                  sprite->es_SimpleSprite.posctldata,
                  sprite->es_SimpleSprite.height,
                  sprite->es_SimpleSprite.x,
                  sprite->es_SimpleSprite.y,
                  sprite->es_SimpleSprite.num,
                  sprite->es_wordwidth,
                  sprite->es_flags);
         )

        if (sprite)
        {
            struct SharedPointer *shared;

            /* If our pointer doesn't already have a user-supplied colormap, we attach it here.
               This makes the pointer to always have its own colors on hi- and truecolor screens.
               In addition it gets an alpha channel.
               Note that this relies on the fact that AllocSpriteDataA() always generates HIDD bitmap
               in sprite->es_BitMap. */
            if (!HIDD_BM_COLMAP(sprite->es_BitMap)) {
                ULONG i;
                HIDDT_Color col[DEF_POINTER_DEPTH] = {{0}};
                struct Color32 *q = GetPrivIBase(IntuitionBase)->Colors;

                for (i = 1; i < DEF_POINTER_DEPTH; i++ ) {
                    col[i].red   = q[i + 7].red >> 16;
                    col[i].green = q[i + 7].green >> 16;
                    col[i].blue  = q[i + 7].blue >> 16;
                    col[i].alpha = GetPrivIBase(IntuitionBase)->PointerAlpha;
                }
                HIDD_BM_SetColors(HIDD_BM_OBJ(sprite->es_BitMap), col, 0, DEF_POINTER_DEPTH);
            }
        
            shared = CreateSharedPointer(sprite,
                                         GetTagData(POINTERA_XOffset, 0, tags),
                                         GetTagData(POINTERA_YOffset, 0, tags),
                                         IntuitionBase);

            if (shared)
            {
                o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

                if (o)
                {
                    struct PointerData *data = INST_DATA(cl, o);
                    
                    data->shared_pointer = shared;
                    //data->xRes = xResolution;
                    //data->yRes = yResolution;
                    D(kprintf("[Pointer] %s: set extSprite 0x%lx and XOffset %ld YOffset %ld\n",
                                                                             __func__,
                                                                             shared->sprite, 
                                                                             shared->xoffset,
                                                                             shared->yoffset));
                }
                else
                {
                    D(kprintf("[Pointer] %s: releasing shared pointer\n", __func__));
                    ReleaseSharedPointer(shared, IntuitionBase);
                }
            }
            else
            {
                D(kprintf("[Pointer] %s: freeing sprite data\n", __func__));
                FreeSpriteData(sprite);
            }
        }
    }

    return (IPTR)o;
}

/***********************************************************************************/

IPTR PointerClass__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct PointerData *data = INST_DATA(cl, o);
    
    D(kprintf("[Pointer] %s()\n", __func__));

    switch (msg->opg_AttrID)
    {
    case POINTERA_SharedPointer:
        D(kprintf("[Pointer] %s: current extSprite 0x%lx\n", __func__, data->shared_pointer->sprite));
        *msg->opg_Storage = (IPTR)data->shared_pointer;
        break;

    case POINTERA_XOffset:
        D(kprintf("[Pointer] %s: current XOffset %ld\n", __func__, data->shared_pointer->xoffset));
        *msg->opg_Storage = data->shared_pointer->xoffset;
        break;

    case POINTERA_YOffset:
        D(kprintf("[Pointer] %s: current YOffset %ld\n", __func__, data->shared_pointer->yoffset));
        *msg->opg_Storage = data->shared_pointer->yoffset;
        break;

    default:
        return DoSuperMethodA(cl, o, (Msg)msg);
    }


    return (IPTR)1;
}

/***********************************************************************************/

IPTR PointerClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct PointerData *data = INST_DATA(cl, o);
    
    D(
      kprintf("[Pointer] %s()\n", __func__);
      kprintf("[Pointer] %s: extSprite 0x%lx\n", __func__, data->shared_pointer->sprite);
     )
    ReleaseSharedPointer(data->shared_pointer, IntuitionBase);

    return DoSuperMethodA(cl, o, msg);
}

/***********************************************************************************/
