/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

/***********************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/***********************************************************************************/

UWORD posctldata[] =
{
    0x0000,0x0000
};

/***********************************************************************************/

IPTR PointerClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    D(kprintf("PointerClass: OM_NEW\n"));

    if (cl)
    {
	struct TagItem *tags = msg->ops_AttrList;
	struct BitMap *bitmap = (struct BitMap *)GetTagData(POINTERA_BitMap, (IPTR)NULL, tags);

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
		    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

		    if (o)
		    {
			struct PointerData *data = INST_DATA(cl, o);
			
			data->shared_pointer = shared;
			//data->xRes = xResolution;
			//data->yRes = yResolution;
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

    return (IPTR)o;
}

/***********************************************************************************/

IPTR PointerClass__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct PointerData *data = INST_DATA(cl, o);
    
    D(kprintf("PointerClass: OM_GET\n"));

    switch (msg->opg_AttrID)
    {
    case POINTERA_SharedPointer:
	*msg->opg_Storage = (IPTR)data->shared_pointer;
	break;

    case POINTERA_XOffset:
	*msg->opg_Storage = data->shared_pointer->xoffset;
	break;

    case POINTERA_YOffset:
	*msg->opg_Storage = data->shared_pointer->yoffset;
	break;

    default:
	return DoSuperMethodA(cl, o, (Msg)msg);
    }
    D(kprintf("PointerClass: current extSprite 0x%lx and XOffset %ld YOffset %ld\n",data->shared_pointer->sprite,data->shared_pointer->xoffset,data->shared_pointer->yoffset));

    return (IPTR)1;
}

/***********************************************************************************/

IPTR PointerClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct PointerData *data = INST_DATA(cl, o);
    
    D(kprintf("PointerClass: OM_DISPOSE\n"));
    D(kprintf("PointerClass: extSprite 0x%lx\n",data->shared_pointer->sprite));
    ReleaseSharedPointer(data->shared_pointer, IntuitionBase);

    return DoSuperMethodA(cl, o, msg);
}

/***********************************************************************************/
