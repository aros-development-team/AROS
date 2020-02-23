/*
    Copyright © 2016-2020, The AROS Development	Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "animationclass.h"

/*
 *  converts/remaps a frame to a bitmap suitable for display..
 */

void cacheFrame(struct Animation_Data *animd, struct AnimFrame *frame)
{
    struct privRenderFrame rendFrameMsg;
    
    DFRAMES("[animation.datatype/CACHE]: %s()\n", __func__)

    if (frame->af_Frame.alf_CMap)
    {
        DFRAMES("[animation.datatype/CACHE]: %s:      CMap @ 0x%p\n", __func__, frame->af_Frame.alf_CMap)
        rendFrameMsg.MethodID = PRIVATE_MAPFRAMEPENS;
        rendFrameMsg.Frame = frame;
        DoMethodA(animd->ad_ProcessData->pp_Object, (Msg)&rendFrameMsg);
    }

    rendFrameMsg.MethodID = PRIVATE_RENDERFRAME;
    rendFrameMsg.Frame = frame;
    if ((rendFrameMsg.Target = (struct BitMap *)frame->af_CacheBM) == NULL)
    {
        frame->af_CacheBM = (char *)AllocBitMap(animd->ad_BitMapHeader.bmh_Width, animd->ad_BitMapHeader.bmh_Height, 24,
                                  BMF_CLEAR, animd->ad_CacheBM);
        rendFrameMsg.Target = (struct BitMap *)frame->af_CacheBM;
        frame->af_Flags = 0;
        DFRAMES("[animation.datatype/CACHE]: %s: allocated frame cache bm @ 0x%p (friend @ 0x%p)\n", __func__, frame->af_CacheBM, animd->ad_CacheBM)
    }
    DoMethodA(animd->ad_ProcessData->pp_Object, (Msg)&rendFrameMsg);
}

void freeFrame(struct Animation_Data *animd, struct AnimFrame *frame)
{
    DFRAMES("[animation.datatype/CACHE]: %s()\n", __func__)
    if (frame->af_CacheBM)
    {
        FreeBitMap((struct BitMap *)frame->af_CacheBM);
        frame->af_CacheBM = NULL;
    }
}
