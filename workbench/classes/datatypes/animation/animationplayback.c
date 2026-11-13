/*
    Copyright © 2015-2020, The AROS Development	Team. All rights reserved.
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
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/realtime.h>
#include <proto/layers.h>
#include <proto/datatypes.h>

#include <intuition/gadgetclass.h>
#include <libraries/realtime.h>
#include <gadgets/tapedeck.h>

#include "animationclass.h"

AROS_UFH3(ULONG, playerHookFunc, 
    AROS_UFHA(struct    Hook *,     hook, A0), 
    AROS_UFHA(struct Player *, obj, A2), 
    AROS_UFHA(struct pmTime *, msg, A1))
{ 
    AROS_USERFUNC_INIT
 
    struct Animation_Data *animd = (struct Animation_Data *)hook->h_Data;
    IPTR buffSigs = 0, playbacksigs = 0;

#if (0)
    // only enable if you like spam.
    bug("[animation.datatype]: %s(%08x)\n", __func__, msg->pmt_Method);
#endif

    switch (msg->pmt_Method)
    {
	case PM_TICK:
            animd->ad_TimerData.atd_Tick++;

            if (animd->ad_TimerData.atd_Tick >= animd->ad_TimerData.atd_TicksPerFrame)
            {
                animd->ad_TimerData.atd_Tick = 0;
                // increment the frame counter if we are supposed to...
                if (!(animd->ad_ProcessData->pp_PlayerFlags & PRIVPROCF_BUSY) || (animd->ad_Flags & ANIMDF_FRAMESKIP))
                {
                    animd->ad_FrameData.afd_FrameCurrent++;
                    if (animd->ad_FrameData.afd_FrameCurrent >= animd->ad_FrameData.afd_Frames)
                    {
                        if (animd->ad_Flags & ANIMDF_REPEAT)
                            animd->ad_FrameData.afd_FrameCurrent = 1;
                        else
                            animd->ad_FrameData.afd_FrameCurrent = animd->ad_FrameData.afd_Frames - 1;
                    }
                }
                if (animd->ad_ProcessData->pp_PlaybackTick != -1)
                    playbacksigs |=  (1 << animd->ad_ProcessData->pp_PlaybackTick);
            }
            if (animd->ad_TimerData.atd_Tick == 0)
            {
                // flush unused buffers...
                if (animd->ad_ProcessData->pp_BufferPurge != -1)
                    buffSigs |= (1 << animd->ad_ProcessData->pp_BufferPurge);
            }
	    break;

	case PM_SHUTTLE:
            D(bug("[animation.datatype] %s: PM_SHUTTLE\n", __func__);)
            animd->ad_FrameData.afd_FrameCurrent = msg->pmt_Time/animd->ad_TimerData.atd_TicksPerFrame;
            animd->ad_TimerData.atd_Tick = 0;
            if (animd->ad_ProcessData->pp_PlaybackSync != -1)
                playbacksigs |=  (1 << animd->ad_ProcessData->pp_PlaybackSync);
	    break;

	case PM_STATE:
	    break;
    }

    if (buffSigs && (animd->ad_BufferProc))
        Signal((struct Task *)animd->ad_BufferProc, buffSigs);
    if (playbacksigs && (animd->ad_PlayerProc))
        Signal((struct Task *)animd->ad_PlayerProc, playbacksigs);

    return 0;

    AROS_USERFUNC_EXIT
}
    
void FreePlaybackSignals(struct ProcessPrivate *priv)
{
    D(bug("[animation.datatype/PLAY]: %s()\n", __func__);)

    if (priv->pp_PlaybackTick != -1)
        FreeSignal(priv->pp_PlaybackTick);
    if (priv->pp_PlaybackEnable != -1)
        FreeSignal(priv->pp_PlaybackEnable);
    if (priv->pp_PlaybackDisable != -1)
        FreeSignal(priv->pp_PlaybackDisable);
}

BOOL AllocPlaybackSignals(struct ProcessPrivate *priv)
{
    if ((priv->pp_PlaybackEnable = AllocSignal(-1)) != -1)
    {
        D(bug("[animation.datatype/PLAY]: %s: allocated enable signal (%x)\n", __func__, priv->pp_PlaybackEnable);)
        if ((priv->pp_PlaybackDisable = AllocSignal(-1)) != -1)
        {
            D(bug("[animation.datatype/PLAY]: %s: allocated disable signal (%x)\n", __func__, priv->pp_PlaybackDisable);)
            if ((priv->pp_PlaybackTick = AllocSignal(-1)) != -1)
            {
                D(bug("[animation.datatype/PLAY]: %s: allocated tick signal (%x)\n", __func__, priv->pp_PlaybackTick);)
                if ((priv->pp_PlaybackSync = AllocSignal(-1)) != -1)
                {
                    D(bug("[animation.datatype/PLAY]: %s: allocated sync signal (%x)\n", __func__, priv->pp_PlaybackSync);)

                    priv->pp_PlaybackSigMask = (1 << priv->pp_PlaybackEnable) | (1 << priv->pp_PlaybackDisable) | (1 << priv->pp_PlaybackTick) | (1 << priv->pp_PlaybackSync);

                    D(bug("[animation.datatype/PLAY]: %s: signal mask (%x)\n", __func__, priv->pp_PlaybackSigMask);)

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

struct AnimFrame *NextFrame(struct ProcessPrivate *priv, struct AnimFrame *frameCurrent, UWORD *frame)
{
    struct AnimFrame *frameFound = NULL,
                     *frameFirst = NULL, *framePrev = NULL;
    UWORD frameID = 0;

    DFRAMES("[animation.datatype/PLAY]: %s(0x%p, %d)\n", __func__, frameCurrent, *frame)

    ObtainSemaphoreShared(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);

    if ((!frameCurrent) ||
        (*frame < GetNODEID(frameCurrent)))
        frameCurrent = frameFound = (struct AnimFrame *)&priv->pp_Data->ad_FrameData.afd_AnimFrames;
    else
        frameFound = frameCurrent;

    while ((frameFound->af_Node.ln_Succ) && (frameFound->af_Node.ln_Succ->ln_Succ))
    {
        frameFound = (struct AnimFrame *)frameFound->af_Node.ln_Succ;

        DFRAMES("[animation.datatype/PLAY] %s:   frame #%d @ 0x%p\n", __func__, GetNODEID(frameFound), frameFound)
        if (!frameFirst)
            frameFirst = frameFound;

        if (GetNODEID(frameFound) >= *frame)
        {
            break;
        }
        framePrev = frameFound;
    }

    if (!(frameFound) ||
        (frameCurrent == frameFound) ||
        (frameFound == (struct AnimFrame *)&priv->pp_Data->ad_FrameData.afd_AnimFrames) ||
        (GetNODEID(frameFound) > *frame))
    {
        frameFound = NULL;

        if (!(priv->pp_Data->ad_Flags & ANIMDF_FRAMESKIP))
        {
            if ((frameFirst) && (GetNODEID(frameFirst) == (GetNODEID(frameCurrent) + 1)))
                frameFound = frameFirst;
        }
        else if ((framePrev) && (GetNODEID(framePrev) > 0))
            frameFound = framePrev;

        if (!(frameFound) &&
            (frameCurrent) &&
            (GetNODEID(frameCurrent) < priv->pp_Data->ad_FrameData.afd_Frames))
            frameFound = frameCurrent;
    }

    if (frameFound)
    {
        frameID = GetNODEID(frameFound);
        if (frameFound != frameCurrent)
            priv->pp_PlaybackFrame = frameFound;
        else
            frameFound = NULL;
    }
    *frame = frameID;

    DFRAMES("[animation.datatype/PLAY] %s: found #%d @ 0x%p\n", __func__, *frame, frameFound)

    ReleaseSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);

    return frameFound;
}

BOOL GetFrameCacheBitmap(struct BitMap **frameBM,  struct AnimFrame *frame)
{
    if (frame->af_Flags & AFFLAGF_READY)
    {
        *frameBM = (struct BitMap *)frame->af_CacheBM;
        return TRUE;
    }
    return FALSE;
}

AROS_UFH3(void, playerProc,
        AROS_UFHA(STRPTR,              argPtr, A0),
        AROS_UFHA(ULONG,               argSize, D0),
        AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct ProcessPrivate *priv = FindTask(NULL)->tc_UserData;
    struct AnimFrame *curFrame = NULL;
    struct gpRender gprMsg;
    struct TagItem attrtags[] =
    {
        { TAG_IGNORE,   0},
        { TAG_IGNORE,   0},
        { TAG_DONE,     0}
    };
    UWORD frame = 0;
    ULONG signal, buffsigs;

    D(bug("[animation.datatype/PLAY]: %s()\n", __func__);)

    if (priv)
    {
        D(
            bug("[animation.datatype/PLAY] %s: private data @ 0x%p\n", __func__, priv);
            bug("[animation.datatype/PLAY] %s: dt obj @ 0x%p, instance data @ 0x%p\n", __func__, priv->pp_Object, priv->pp_Data);
        )

        priv->pp_PlaybackFrame = NULL;
        priv->pp_PlayerFlags |= PRIVPROCF_RUNNING;

        if (AllocPlaybackSignals(priv))
        {
            D(bug("[animation.datatype/PLAY]: %s: entering main loop ...\n", __func__);)
            while (TRUE)
            {
                priv->pp_PlayerFlags &= ~PRIVPROCF_ACTIVE;

                signal = priv->pp_PlaybackSigMask | SIGBREAKF_CTRL_C;
                signal = Wait(signal);

                D(bug("[animation.datatype/PLAY]: %s: signalled (%08x)\n", __func__, signal);)

                if (signal & SIGBREAKF_CTRL_C)
                    break;

                priv->pp_PlayerFlags |= PRIVPROCF_ACTIVE;

                if (signal & (1 << priv->pp_PlaybackEnable))
                    priv->pp_PlayerFlags |= PRIVPROCF_ENABLED;
                else if (signal & (1 << priv->pp_PlaybackDisable))
                    priv->pp_PlayerFlags &= ~PRIVPROCF_ENABLED;

                if ((priv->pp_PlayerFlags & PRIVPROCF_ENABLED) && ((signal & ((1 << priv->pp_PlaybackTick)|(1 << priv->pp_PlaybackSync))) != 0))
                {
                    priv->pp_PlayerFlags |= PRIVPROCF_BUSY;
                    frame = priv->pp_Data->ad_FrameData.afd_FrameCurrent;
                    buffsigs = 0;

                    DFRAMES("[animation.datatype/PLAY] %s: Frame #%d\n", __func__, frame)

                    curFrame = NextFrame(priv, priv->pp_PlaybackFrame, &frame);

                    if ((priv->pp_BufferFrames > priv->pp_BufferLevel) &&
                        (priv->pp_BufferLevel < priv->pp_Data->ad_FrameData.afd_Frames))
                    {
                        buffsigs |= (1 << priv->pp_BufferFill);
                    }

                    if (!(curFrame))
                    {
                        ObtainSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);
                        if ((signal & (1 << priv->pp_PlaybackSync)) ||
                            (priv->pp_Data->ad_Flags & ANIMDF_FRAMESKIP))
                        {
                            priv->pp_BufferSpecific = frame;
                        }

                        if ((priv->pp_PlaybackFrame) &&
                            ((GetNODEID(priv->pp_PlaybackFrame) < frame) ||
                            (!(priv->pp_Data->ad_Flags & ANIMDF_FRAMESKIP) && (GetNODEID(priv->pp_PlaybackFrame) < (priv->pp_Data->ad_FrameData.afd_Frames - 1)))))
                        {
                            priv->pp_BufferFirst = priv->pp_PlaybackFrame;
                        }
                        else if (priv->pp_Data->ad_Flags & ANIMDF_FRAMESKIP)
                            priv->pp_BufferFirst = NULL;

                        ReleaseSemaphore(&priv->pp_Data->ad_FrameData.afd_AnimFramesLock);

                        buffsigs |= ((1 << priv->pp_BufferPurge) | (1 << priv->pp_BufferFill));
                    }

                    if ((buffsigs) && (priv->pp_Data->ad_BufferProc))
                    {
                        Signal((struct Task *)priv->pp_Data->ad_BufferProc, buffsigs);
                        if (buffsigs & (1 << priv->pp_BufferPurge))
                            SetTaskPri((struct Task *)priv->pp_Data->ad_PlayerProc, -2);
                    }

                    // frame has changed ... render it ..
                    if ((curFrame) && GetFrameCacheBitmap(&priv->pp_Data->ad_FrameBM,  curFrame))
                    {
                        D(
                            bug("[animation.datatype/PLAY]: %s: Rendering Frame #%d\n", __func__,  GetNODEID(curFrame));
                            bug("[animation.datatype/PLAY]: %s:      BitMap @ 0x%p\n", __func__, priv->pp_Data->ad_FrameBM);
                        )

                        priv->pp_PlayerFlags &= ~PRIVPROCF_BUSY;

                        if ((priv->pp_Data->ad_Window) && !(priv->pp_Data->ad_Flags & ANIMDF_LAYOUT))
                        {
                            if (priv->pp_Data->ad_Tapedeck)
                            {
                                // update the tapedeck gadget..
                                attrtags[0].ti_Tag = TDECK_CurrentFrame;
                                attrtags[0].ti_Data = GetNODEID(curFrame);
                                attrtags[1].ti_Tag = TAG_IGNORE;

                                SetAttrsA((Object *)priv->pp_Data->ad_Tapedeck, attrtags);
                            }

                            // tell the top level gadget to redraw...
                            gprMsg.MethodID   = GM_RENDER;
                            gprMsg.gpr_RPort  = priv->pp_Data->ad_Window->RPort;
                            gprMsg.gpr_GInfo  = NULL;
                            gprMsg.gpr_Redraw = GREDRAW_UPDATE;
                            DoGadgetMethodA((struct Gadget *)priv->pp_Object, priv->pp_Data->ad_Window, NULL, (Msg)&gprMsg);
                        }
                    }
                }
            }
            FreePlaybackSignals(priv);
        }
        priv->pp_PlayerFlags &= ~PRIVPROCF_RUNNING;
        priv->pp_Data->ad_PlayerProc = NULL;
    }

    D(bug("[animation.datatype/PLAY]: %s: exiting ...\n", __func__);)

    return;

    AROS_USERFUNC_EXIT
}
