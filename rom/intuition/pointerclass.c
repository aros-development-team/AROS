/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

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

struct PointerData
{
    struct SharedPointer *shared_pointer;
};

/***********************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/***********************************************************************************/

#define P(o) ((struct PointerData *)INST_DATA(cl,o))

/***********************************************************************************/

UWORD posctldata[] =
{
    0x0000,0x0000
};

AROS_UFH3S(IPTR, dispatch_pointerclass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    D(kprintf("PointerClass: cl 0x%lx o 0x%lx msg 0x%lx\n",cl,o,msg));

    switch (msg->MethodID)
    {
    case OM_NEW:
        D(kprintf("PointerClass: OM_NEW\n"));

        if (cl)
        {
            struct TagItem *tags = ((struct opSet *)msg)->ops_AttrList;
            struct BitMap *bitmap = (struct BitMap *)GetTagData(POINTERA_BitMap, NULL, tags);

            //ULONG xResolution = GetTagData(POINTERA_XResolution, POINTERXRESN_DEFAULT, tags);
            //ULONG yResolution = GetTagData(POINTERA_YResolution, POINTERYRESN_DEFAULT, tags);

            D(
            {
                struct TagItem *tagscan=tags;

                while(tagscan->ti_Tag != 0)
                {
                    kprintf("  0x%08lx, %08lx\n",tagscan->ti_Tag,tagscan->ti_Data);
                    tagscan++;
                }
            }
            )

            if (1)  // bitmap
            {
                struct TagItem spritetags[] =
                {
                    {SPRITEA_Width  , 0 },
                    {TAG_SKIP	    , 0 },
                    {TAG_SKIP	    , 0 },
                    {TAG_DONE        	}
                };
                struct ExtSprite *sprite;
                struct BitMap *spritedata=(struct BitMap *)bitmap;

                if(spritedata != NULL)
                {
            	    spritetags[0].ti_Data = GetTagData(POINTERA_WordWidth,
                                            	       ((GetBitMapAttr(bitmap, BMA_WIDTH) + 15) & ~15)>>4, tags) * 16;
                    spritetags[1].ti_Tag = TAG_SKIP;
                    spritetags[2].ti_Tag = TAG_SKIP;
                }
                else
                {
                    D(kprintf("PointerClass: OM_NEW called without bitmap, using dummy sprite !\n"));

                    spritetags[0].ti_Data = 16;
                    spritetags[1].ti_Tag = SPRITEA_OutputHeight;
                    spritetags[1].ti_Data = 1; 
                    spritetags[2].ti_Tag = SPRITEA_OldDataFormat;
                    spritetags[2].ti_Data = TRUE;
                    bitmap = (struct BitMap *)posctldata;

                }

                sprite = AllocSpriteDataA(bitmap, spritetags);

                D(kprintf("PointerClass: extSprite 0x%lx\n",sprite));
                D(kprintf("MoveSprite data 0x%lx, height %ld, x %ld, y %ld, num %ld, wordwidth, 0x%lx, flags 0x%lx\n",
                          sprite->es_SimpleSprite.posctldata,
                          sprite->es_SimpleSprite.height,
                          sprite->es_SimpleSprite.x,
                          sprite->es_SimpleSprite.y,
                          sprite->es_SimpleSprite.num,
                          sprite->es_wordwidth,
                          sprite->es_flags));

                if (sprite)
                {
                    struct SharedPointer *shared = CreateSharedPointer(sprite,
                                                        	       GetTagData(POINTERA_XOffset, 0, tags),
                                                        	       GetTagData(POINTERA_YOffset, 0, tags),
                                                        	       IntuitionBase);

                    if (shared)
                    {
                        retval = (IPTR)DoSuperMethodA(cl, o, msg);

                        if (retval)
                        {
                            o = (Object *)retval;
                            P(o)->shared_pointer = shared;
                            //P(o)->xRes = xResolution;
                            //P(o)->yRes = yResolution;
                            D(kprintf("PointerClass: set extSprite 0x%lx and XOffset %ld YOffset %ld\n",shared->sprite,shared->xoffset,shared->yoffset));
                        }
                        else
                        {
                            D(kprintf("PointerClass: free sprite\n"));
                            ReleaseSharedPointer(shared, IntuitionBase);
                        }
                    }
                    else
                    {
                        D(kprintf("PointerClass: free sprite\n"));
                        FreeSpriteData(sprite);
                    }
                }
            }
            else
            {
            D(kprintf("PointerClass: OM_NEW called without bitmap !\n"));
            }
        }
        break;

    case OM_GET:
        {
            struct opGet *gmsg = (struct opGet *)msg;
	    
            D(kprintf("PointerClass: OM_GET\n"));
            retval = 1;
            switch (gmsg->opg_AttrID)
            {
        	case POINTERA_SharedPointer:
                    *gmsg->opg_Storage = (IPTR)P(o)->shared_pointer;
                    break;

        	case POINTERA_XOffset:
                    *gmsg->opg_Storage = P(o)->shared_pointer->xoffset;
                    break;

        	case POINTERA_YOffset:
                    *gmsg->opg_Storage = P(o)->shared_pointer->yoffset;
                    break;

        	default:
                    retval = DoSuperMethodA(cl, o, msg);
                    break;
            }
            D(kprintf("PointerClass: current extSprite 0x%lx and XOffset %ld YOffset %ld\n",P(o)->shared_pointer->sprite,P(o)->shared_pointer->xoffset,P(o)->shared_pointer->yoffset));
        }
        break;

    case OM_DISPOSE:
        D(kprintf("PointerClass: OM_DISPOSE\n"));
        D(kprintf("PointerClass: extSprite 0x%lx\n",P(o)->shared_pointer->sprite));
        ReleaseSharedPointer(P(o)->shared_pointer, IntuitionBase);
        /* fall through */
    default:
        D(kprintf("PointerClass: DoSuperMethod MethodID 0x%lx\n",msg->MethodID));
        retval = DoSuperMethodA(cl, o, msg);
        break;

    } /* switch */


    D(kprintf("PointerClass: retval 0x%lx\n",retval));
    return (retval);

    AROS_USERFUNC_EXIT
} /* dispatch_pointerclass */

/***********************************************************************************/

#undef IntuitionBase

/***********************************************************************************/

struct IClass *InitPointerClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the image class...
    */
    if ((cl = MakeClass(POINTERCLASS, ROOTCLASS, NULL, sizeof(struct PointerData), 0)))
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_pointerclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/***********************************************************************************/
