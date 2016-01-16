/*
    Copyright © 2015-2016, The AROS Development	Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/realtime.h>

#include <intuition/gadgetclass.h>
#include <libraries/realtime.h>
#include <gadgets/tapedeck.h>

#include "animationclass.h"
    
void FreeBufferSignals(struct ProcessPrivate *priv)
{
    D(bug("[animation.datatype/BUFFER]: %s()\n", __PRETTY_FUNCTION__));

    if (priv->pp_BufferFill != -1)
        FreeSignal(priv->pp_BufferFill);
    if (priv->pp_BufferPurge != -1)
        FreeSignal(priv->pp_BufferPurge);
    if (priv->pp_BufferEnable != -1)
        FreeSignal(priv->pp_BufferEnable);
    if (priv->pp_BufferDisable != -1)
        FreeSignal(priv->pp_BufferDisable);
}

BOOL AllocBufferSignals(struct ProcessPrivate *priv)
{
    D(bug("[animation.datatype/BUFFER]: %s()\n", __PRETTY_FUNCTION__));

    if ((priv->pp_BufferEnable = AllocSignal(-1)) != -1)
    {
        D(bug("[animation.datatype/BUFFER]: %s: allocated enable signal (%x)\n", __PRETTY_FUNCTION__, priv->pp_BufferEnable));
        if ((priv->pp_BufferDisable = AllocSignal(-1)) != -1)
        {
            D(bug("[animation.datatype/BUFFER]: %s: allocated disable signal (%x)\n", __PRETTY_FUNCTION__, priv->pp_BufferDisable));
            if ((priv->pp_BufferFill = AllocSignal(-1)) != -1)
            {
                D(bug("[animation.datatype/BUFFER]: %s: allocated fill signal (%x)\n", __PRETTY_FUNCTION__, priv->pp_BufferFill));
                if ((priv->pp_BufferPurge = AllocSignal(-1)) != -1)
                {
                    D(bug("[animation.datatype/BUFFER]: %s: allocated purge signal (%x)\n", __PRETTY_FUNCTION__, priv->pp_BufferPurge));

                    priv->pp_BufferSigMask = (1 << priv->pp_BufferEnable) | (1 << priv->pp_BufferDisable) | (1 << priv->pp_BufferFill) | (1 << priv->pp_BufferPurge);

                    D(bug("[animation.datatype/BUFFER]: %s: signal mask (%x)\n", __PRETTY_FUNCTION__, priv->pp_BufferSigMask));

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

BOOL DoFramePurge(struct AnimFrame *purgeFrame)
{
    return FALSE;
}

/*
    Process to handle loading/discarding (buffering) of  animation frames
    realtime.library player & playback thread will signal us when needed.
*/

AROS_UFH3(void, bufferProc,
        AROS_UFHA(STRPTR,              argPtr, A0),
        AROS_UFHA(ULONG,               argSize, D0),
        AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct ProcessPrivate *priv = FindTask(NULL)->tc_UserData;
    struct AnimFrame *curFrame = NULL, *lastFrame = NULL;
    ULONG signal, bufferstep;

    D(bug("[animation.datatype/BUFFER]: %s()\n", __PRETTY_FUNCTION__));

    if (priv)
    {
        D(bug("[animation.datatype/BUFFER] %s: private data @ 0x%p\n", __PRETTY_FUNCTION__, priv));
        D(bug("[animation.datatype/BUFFER] %s: dt obj @ 0x%p, instance data @ 0x%p\n", __PRETTY_FUNCTION__, priv->pp_Object, priv->pp_Data));

        priv->pp_BufferFlags |= PRIVPROCF_RUNNING;

        if (AllocBufferSignals(priv))
        {
            D(bug("[animation.datatype/BUFFER]: %s: entering main loop ...\n", __PRETTY_FUNCTION__));

            bufferstep = priv->pp_Data->ad_BufferStep;

            while (TRUE)
            {
                priv->pp_BufferFlags &= ~PRIVPROCF_ACTIVE;

                if ((bufferstep >= 1) && (priv->pp_BufferFlags & PRIVPROCF_ENABLED) &&
                    (priv->pp_BufferLevel < priv->pp_BufferFrames) &&
                    (priv->pp_BufferLevel < priv->pp_Data->ad_FrameData.afd_Frames))
                {
                    D(bug("[animation.datatype/BUFFER]: %s: %d:%d\n", __PRETTY_FUNCTION__, priv->pp_BufferLevel, priv->pp_BufferFrames));
                    signal = (1 << priv->pp_BufferFill);
                }
                else
                {
                    bufferstep = priv->pp_Data->ad_BufferStep;
                    D(bug("[animation.datatype/BUFFER]: %s: waiting ...\n", __PRETTY_FUNCTION__));
                    signal = Wait(priv->pp_BufferSigMask | SIGBREAKF_CTRL_C);
                };

                D(bug("[animation.datatype/BUFFER]: %s: signalled (%08x)\n", __PRETTY_FUNCTION__, signal));

                if (signal & SIGBREAKF_CTRL_C)
                    break;

                if (signal & (1 << priv->pp_BufferEnable))
                    priv->pp_BufferFlags |= PRIVPROCF_ENABLED;
                else if (signal & (1 << priv->pp_BufferDisable))
                    priv->pp_BufferFlags &= ~PRIVPROCF_ENABLED;

                if ((priv->pp_BufferFlags & PRIVPROCF_ENABLED) && (signal & (1 <<priv->pp_BufferPurge)))
                {
                    struct Node *purgeFrame = NULL, *tmpFrame = NULL;

                    D(bug("[animation.datatype/BUFFER]: %s: Purging Frames...\n", __PRETTY_FUNCTION__));
                    ForeachNodeSafe(&priv->pp_Data->ad_FrameData.afd_AnimFrames, purgeFrame, tmpFrame)
                    {
                        if (DoFramePurge((struct AnimFrame *)purgeFrame))
                            priv->pp_BufferLevel--;
                    }
                }

                if ((priv->pp_BufferFlags & PRIVPROCF_ENABLED) && (signal & (1 <<priv->pp_BufferFill)))
                {
                    D(bug("[animation.datatype/BUFFER]: %s: Loading Frames...\n", __PRETTY_FUNCTION__));

                    priv->pp_BufferFlags |= PRIVPROCF_ACTIVE;

                    if (priv->pp_BufferLevel < priv->pp_Data->ad_FrameData.afd_Frames)
                    {
                        if ((curFrame) ||
                            ((curFrame = AllocMem(sizeof(struct AnimFrame), MEMF_ANY)) != NULL))
                        {
                            D(bug("[animation.datatype/BUFFER]: %s: using AnimFrame @ 0x%p\n", __PRETTY_FUNCTION__, curFrame));

                            curFrame->af_Frame.MethodID = ADTM_LOADFRAME;

                            if (lastFrame)
                            {
                                curFrame->af_Frame.alf_Frame = lastFrame->af_Frame.alf_Frame + 1;
                                curFrame->af_Frame.alf_TimeStamp = lastFrame->af_Frame.alf_TimeStamp + 1;
                            }
                            else
                            {
                                curFrame->af_Frame.alf_Frame = 0;
                                curFrame->af_Frame.alf_TimeStamp = 0;
                            }
                            curFrame->af_Frame.alf_BitMap = NULL;
                            curFrame->af_Frame.alf_CMap = NULL;
                            curFrame->af_Frame.alf_Sample = NULL;
                            curFrame->af_Frame.alf_UserData = NULL;

                            D(bug("[animation.datatype/BUFFER]: %s: Loading Frame #%d\n", __PRETTY_FUNCTION__, curFrame->af_Frame.alf_Frame));

                            if (DoMethodA(priv->pp_Object, (Msg)&curFrame->af_Frame))
                            {
                                priv->pp_BufferLevel++;
                                D(bug("[animation.datatype/BUFFER]: %s: Loaded! bitmap @ %p\n", __PRETTY_FUNCTION__, curFrame->af_Frame.alf_BitMap));
                                D(bug("[animation.datatype/BUFFER]: %s: frame #%d. stamp %d\n", __PRETTY_FUNCTION__, curFrame->af_Frame.alf_Frame, curFrame->af_Frame.alf_TimeStamp));
                                ObtainSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);
                                AddTail(&priv->pp_Data->ad_FrameData.afd_AnimFrames, &curFrame->af_Node);
                                ReleaseSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);
                                if (curFrame->af_Frame.alf_Frame < (priv->pp_Data->ad_FrameData.afd_Frames - 1))
                                    lastFrame =  curFrame;
                                else
                                    lastFrame =  NULL;
                                curFrame = NULL;
                            }
                        }
                    }

                    if (bufferstep > 0)
                        bufferstep = 1;

                    SetTaskPri((struct Task *)priv->pp_Data->ad_PlayerProc, 0);
                }
            }
            FreeBufferSignals(priv);
        }
        priv->pp_BufferFlags &= ~PRIVPROCF_RUNNING;
        priv->pp_Data->ad_BufferProc = NULL;
    }
    D(bug("[animation.datatype/BUFFER]: %s: exiting ...\n", __PRETTY_FUNCTION__));

    return;

    AROS_USERFUNC_EXIT
}