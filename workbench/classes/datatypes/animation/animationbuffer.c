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
    D(bug("[animation.datatype/BUFFER]: %s()\n", __func__);)

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
    D(bug("[animation.datatype/BUFFER]: %s()\n", __func__);)

    if ((priv->pp_BufferEnable = AllocSignal(-1)) != -1)
    {
        D(bug("[animation.datatype/BUFFER]: %s: allocated enable signal (%x)\n", __func__, priv->pp_BufferEnable);)
        if ((priv->pp_BufferDisable = AllocSignal(-1)) != -1)
        {
            D(bug("[animation.datatype/BUFFER]: %s: allocated disable signal (%x)\n", __func__, priv->pp_BufferDisable);)
            if ((priv->pp_BufferFill = AllocSignal(-1)) != -1)
            {
                D(bug("[animation.datatype/BUFFER]: %s: allocated fill signal (%x)\n", __func__, priv->pp_BufferFill);)
                if ((priv->pp_BufferPurge = AllocSignal(-1)) != -1)
                {
                    D(bug("[animation.datatype/BUFFER]: %s: allocated purge signal (%x)\n", __func__, priv->pp_BufferPurge);)

                    priv->pp_BufferSigMask = (1 << priv->pp_BufferEnable) | (1 << priv->pp_BufferDisable) | (1 << priv->pp_BufferFill) | (1 << priv->pp_BufferPurge);

                    D(bug("[animation.datatype/BUFFER]: %s: signal mask (%x)\n", __func__, priv->pp_BufferSigMask);)

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

struct AnimFrame *NextToBuffer(struct ProcessPrivate *priv, struct AnimFrame *newFrame)
{
    struct AnimFrame *startFrame, *prevFrame;
    BOOL loop = FALSE;

    DFRAMES("[animation.datatype/BUFFER]: %s()\n", __PRETTY_FUNCTION__)

    newFrame->af_Frame.MethodID = ADTM_LOADFRAME;

    ObtainSemaphoreShared(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);

    if ((startFrame = priv->pp_BufferFirst) == NULL)
        prevFrame = (struct AnimFrame *)&priv->pp_Data->ad_FrameData.afd_AnimFrames;
    else
        prevFrame = startFrame;

findprevframe:
    while ((prevFrame->af_Node.ln_Succ) && 
                ((prevFrame == (struct AnimFrame *)&priv->pp_Data->ad_FrameData.afd_AnimFrames) ||
                 ((prevFrame->af_Node.ln_Succ->ln_Succ) && (NODEID(prevFrame->af_Node.ln_Succ) == (NODEID(prevFrame) + 1)))))
    {
        prevFrame = (struct AnimFrame *)prevFrame->af_Node.ln_Succ;
    }

    if (NODEID(prevFrame) == (priv->pp_Data->ad_FrameData.afd_Frames - 1))
    {
        prevFrame = (struct AnimFrame *)&priv->pp_Data->ad_FrameData.afd_AnimFrames;
        if (!loop)
        {
            loop = TRUE;
            goto findprevframe;
        }
    }

    if (prevFrame != (struct AnimFrame *)&priv->pp_Data->ad_FrameData.afd_AnimFrames)
    {
        startFrame = prevFrame;
        newFrame->af_Frame.alf_Frame = NODEID(prevFrame) + 1;
    }
    else
        newFrame->af_Frame.alf_Frame = 0;

    NODEID(newFrame) = (UWORD) newFrame->af_Frame.alf_Frame;
    newFrame->af_Frame.alf_TimeStamp = newFrame->af_Frame.alf_Frame;

    ReleaseSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);

    return startFrame;
}

BOOL DoFramePurge(struct Animation_Data *animd, struct AnimFrame *purgeFrame)
{
    UWORD dispplayedframe = animd->ad_FrameData.afd_FrameCurrent;

    if ((purgeFrame->af_Frame.alf_BitMap != animd->ad_FrameBM) &&
        (purgeFrame != animd->ad_ProcessData->pp_PlaybackFrame) &&
        (purgeFrame != animd->ad_ProcessData->pp_BufferFirst) &&
        (NODEID(purgeFrame) != dispplayedframe) &&
        (NODEID(purgeFrame) != 0))
    {
        if ((NODEID(purgeFrame) > (dispplayedframe + animd->ad_ProcessData->pp_BufferFrames)) ||
            (NODEID(purgeFrame) < dispplayedframe))
        {
            if ((dispplayedframe > (animd->ad_FrameData.afd_Frames -  animd->ad_ProcessData->pp_BufferFrames)) &&
                (NODEID(purgeFrame) < (animd->ad_ProcessData->pp_BufferFrames - (animd->ad_FrameData.afd_Frames - dispplayedframe))))
                return FALSE;

            return TRUE;
        }
            
    }
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

    struct Task *thisTask = FindTask(NULL);
    struct ProcessPrivate *priv = thisTask->tc_UserData;
    struct AnimFrame *curFrame = NULL, *startFrame;
    ULONG signal, bufferstep;

    D(bug("[animation.datatype/BUFFER]: %s()\n", __func__);)

    if (priv)
    {
        D(
            bug("[animation.datatype/BUFFER] %s: private data @ 0x%p\n", __func__, priv);
            bug("[animation.datatype/BUFFER] %s: dt obj @ 0x%p, instance data @ 0x%p\n", __func__, priv->pp_Object, priv->pp_Data);
        )

        priv->pp_BufferFirst = NULL;
        priv->pp_BufferLevel = 0;
        priv->pp_BufferFlags |= PRIVPROCF_RUNNING;

        if (AllocBufferSignals(priv))
        {
            D(bug("[animation.datatype/BUFFER]: %s: entering main loop ...\n", __func__);)

            bufferstep = priv->pp_Data->ad_BufferStep;

            while (TRUE)
            {
                priv->pp_BufferFlags &= ~PRIVPROCF_ACTIVE;

                if ((priv->pp_BufferFlags & PRIVPROCF_ENABLED) &&
                    ((bufferstep >= 1) || (priv->pp_BufferLevel < priv->pp_BufferFrames)) &&
                    (priv->pp_BufferLevel < priv->pp_Data->ad_FrameData.afd_Frames))
                {
                    D(bug("[animation.datatype/BUFFER]: %s: %d:%d\n", __func__, priv->pp_BufferLevel, priv->pp_BufferFrames);)
                    signal = (1 << priv->pp_BufferFill);
                }
                else
                {
                    bufferstep = priv->pp_Data->ad_BufferStep;
                    D(bug("[animation.datatype/BUFFER]: %s: waiting ...\n", __func__);)
                    signal = Wait(priv->pp_BufferSigMask | SIGBREAKF_CTRL_C);
                };

                D(bug("[animation.datatype/BUFFER]: %s: signalled (%08x)\n", __func__, signal);)

                if (signal & SIGBREAKF_CTRL_C)
                    break;

                if (signal & (1 << priv->pp_BufferEnable))
                    priv->pp_BufferFlags |= PRIVPROCF_ENABLED;
                else if (signal & (1 << priv->pp_BufferDisable))
                    priv->pp_BufferFlags &= ~PRIVPROCF_ENABLED;

                if ((priv->pp_BufferFlags & PRIVPROCF_ENABLED) &&
                    (signal & (1 <<priv->pp_BufferPurge)) &&
                    (priv->pp_BufferFrames < priv->pp_Data->ad_FrameData.afd_Frames))
                {
                    struct AnimFrame *purgeFrame = NULL, *tmpFrame = NULL;

                    D(bug("[animation.datatype/BUFFER]: %s: Purging Frames...\n", __func__);)
                    if (AttemptSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock))
                    {
                        D(bug("[animation.datatype/BUFFER]: %s: locked frame list...\n", __func__);)
                        ForeachNodeSafe(&priv->pp_Data->ad_FrameData.afd_AnimFrames, purgeFrame, tmpFrame)
                        {
                            if (DoFramePurge(priv->pp_Data, (struct AnimFrame *)purgeFrame))
                            {
                                D(bug("[animation.datatype/BUFFER]: %s: unloading frame #%d\n", __func__, NODEID(purgeFrame));)

                                freeFrame(priv->pp_Data, purgeFrame);

                                purgeFrame->af_Frame.MethodID = ADTM_UNLOADFRAME;
                                DoMethodA(priv->pp_Object, (Msg)&purgeFrame->af_Frame);

                                D(bug("[animation.datatype/BUFFER]: %s: freeing frame @0x%p\n", __func__, purgeFrame);)
                                Remove(&purgeFrame->af_Node);
                                FreeMem(purgeFrame, sizeof(struct AnimFrame));

                                priv->pp_BufferLevel--;
                            }
                        }
                        ReleaseSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);
                    }
                }

                if ((priv->pp_BufferFlags & PRIVPROCF_ENABLED) &&
                    (signal & (1 <<priv->pp_BufferFill)))
                {
                    D(bug("[animation.datatype/BUFFER]: %s: Loading Frames...\n", __func__);)

                    priv->pp_BufferFlags |= PRIVPROCF_ACTIVE;

                    if ((priv->pp_BufferFrames > priv->pp_BufferLevel) &&
                        (priv->pp_BufferLevel < priv->pp_Data->ad_FrameData.afd_Frames))
                    {
                        if ((curFrame) ||
                            ((curFrame = AllocMem(sizeof(struct AnimFrame), MEMF_ANY|MEMF_CLEAR)) != NULL))
                        {
                            startFrame = NextToBuffer(priv, curFrame);

                            D(
                                bug("[animation.datatype/BUFFER]: %s: Loading Frame #%d (AnimFrame @ 0x%p)\n", __func__, curFrame->af_Frame.alf_Frame, curFrame);
                                bug("[animation.datatype/BUFFER]: %s:   startFrame @ 0x%p\n", __func__, startFrame);
                            )

                            if (DoMethodA(priv->pp_Object, (Msg)&curFrame->af_Frame))
                            {
                                priv->pp_BufferLevel++;
                                D(                                    bug("[animation.datatype/BUFFER]: %s: Loaded! bitmap @ %p\n", __func__, curFrame->af_Frame.alf_BitMap);
                                    bug("[animation.datatype/BUFFER]: %s:   frame #%d. stamp %d\n", __func__, curFrame->af_Frame.alf_Frame, curFrame->af_Frame.alf_TimeStamp);
                                    bug("[animation.datatype/BUFFER]: %s:   bitmap @ %p\n", __func__, curFrame->af_Frame.alf_BitMap);
                                )

                                ObtainSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);
                                Insert(&priv->pp_Data->ad_FrameData.afd_AnimFrames, &curFrame->af_Node, &startFrame->af_Node);
                                if (startFrame == priv->pp_BufferFirst)
                                {
                                    if (NODEID(curFrame) < (priv->pp_Data->ad_FrameData.afd_Frames - 1))
                                    {
                                        priv->pp_BufferFirst =  curFrame;
                                        while ((priv->pp_BufferFirst->af_Node.ln_Succ) && (NODEID(priv->pp_BufferFirst->af_Node.ln_Succ) == (NODEID(priv->pp_BufferFirst) + 1)))
                                            priv->pp_BufferFirst = (struct AnimFrame *)priv->pp_BufferFirst->af_Node.ln_Succ;
                                    }
                                    if ((priv->pp_BufferFirst) &&
                                        (NODEID(priv->pp_BufferFirst) == (priv->pp_Data->ad_FrameData.afd_Frames - 1)))
                                        priv->pp_BufferFirst =  NULL;
                                }
                                ReleaseSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);

                                cacheFrame(priv->pp_Data, curFrame);

                                curFrame = NULL;
                            }
                            else
                            {
                                curFrame->af_Frame.MethodID = ADTM_UNLOADFRAME;
                                DoMethodA(priv->pp_Object, (Msg)&curFrame->af_Frame);
                            }
                        }
                    }

                    if (bufferstep > 0)
                        bufferstep--;

                    SetTaskPri((struct Task *)priv->pp_Data->ad_PlayerProc, 0);
                }
            }
            FreeBufferSignals(priv);
        }
        priv->pp_BufferFlags &= ~PRIVPROCF_RUNNING;
        priv->pp_Data->ad_BufferProc = NULL;
    }
    D(bug("[animation.datatype/BUFFER]: %s: exiting ...\n", __func__);)

    return;

    AROS_USERFUNC_EXIT
}