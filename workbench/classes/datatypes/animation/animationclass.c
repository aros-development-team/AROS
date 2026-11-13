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
#include <proto/iffparse.h>
#include <proto/layers.h>
#include <proto/datatypes.h>
#include <proto/cybergraphics.h>
#include <proto/realtime.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <cybergraphx/cybergraphics.h>
#include <libraries/realtime.h>

#include <gadgets/tapedeck.h>

#include "animationclass.h"

#include <stdio.h>
#include <string.h>

extern AROS_UFP3(ULONG, playerHookFunc,
    AROS_UFPA(struct Hook *, hook, A0),
    AROS_UFPA(struct Player *, obj, A2),
    AROS_UFPA(struct pmTime *, msg, A1));

extern AROS_UFP3(void, bufferProc,
    AROS_UFPA(STRPTR, argPtr, A0),
    AROS_UFPA(ULONG, argSize, D0),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

extern AROS_UFP3(void, playerProc,
    AROS_UFPA(STRPTR, argPtr, A0),
    AROS_UFPA(ULONG, argSize, D0),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

extern void ReadENVPrefs(APTR, APTR);

ADD2LIBS("realtime.library", 0, struct Library *, RealTimeBase)
ADD2LIBS("gadgets/tapedeck.gadget", 0, struct Library *, TapeDeckBase)

const IPTR SupportedMethods[] =
{
    OM_NEW,
    OM_GET,
    OM_SET,
    OM_UPDATE,
    OM_DISPOSE,

    GM_LAYOUT,
    GM_HITTEST,
    GM_GOACTIVE,
    GM_GOINACTIVE,
    GM_HANDLEINPUT,
    GM_RENDER,

    DTM_FRAMEBOX,

    DTM_TRIGGER,

    DTM_CLEARSELECTED,
    DTM_COPY,
    DTM_PRINT,
    DTM_WRITE,

#if (0)
    ADTM_LOCATE,
    ADTM_PAUSE,
    ADTM_START,
    ADTM_STOP,
    ADTM_LOADFRAME,
    ADTM_UNLOADFRAME,
    ADTM_LOADNEWFORMATFRAME,
    ADTM_UNLOADNEWFORMATFRAME,
#endif

    (~0)
};

struct DTMethod SupportedTriggerMethods[] =
{
    { "Play",                   "PLAY",		        STM_PLAY        },
    { "Stop",                   "STOP",		        STM_STOP        },
    { "Pause",                  "PAUSE",	        STM_PAUSE       },

    { "Rewind",                 "REWIND",	        STM_REWIND      },
    { "Fast Forward",           "FF",		        STM_FASTFORWARD },

    { NULL,                     NULL,                   0               },
};

char *GenTaskName(char *basename, char *taskname)
{
    int namLen, id = 0;
    char *newName;

    namLen = strlen(basename);
    newName = AllocVec(namLen + strlen(taskname) + 2 + 5, MEMF_ANY);
    while (TRUE)
    {
        if (id == 0)
            sprintf(newName, "%s %s", basename, taskname);
        else
            sprintf(newName, "%s %s.%d", basename, taskname, id);
        if (FindTask(newName) == NULL)
            break;
    }
    return newName;
}

/*** PRIVATE METHODS ***/
IPTR DT_InitPlayer(struct IClass *cl, struct Gadget *g, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    struct TagItem playertags[] =
    {
        { PLAYER_Name,          (IPTR) "animation"      },
#if (0)
        { PLAYER_Conductor,     (IPTR) "playback"       },
#else
        { PLAYER_Conductor,     (IPTR) ~0               },
#endif
        { PLAYER_Priority,      0                       },
        { PLAYER_Hook,          0                       },
        { TAG_DONE,             0                       }
    };

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if (!animd->ad_Player)
    {
        /* create a realtime player */
        animd->ad_PlayerHook.h_Entry = (unsigned char *)playerHookFunc; 
        animd->ad_PlayerHook.h_Data = animd;
        playertags[3].ti_Data = (IPTR)&animd->ad_PlayerHook;
        animd->ad_Player = CreatePlayerA(playertags);
    }
    D(bug("[animation.datatype] %s: Realtime Player @ 0x%p\n", __func__, animd->ad_Player);)

    if (!animd->ad_ProcessData)
    {
        char *projName = NULL;

        animd->ad_ProcessData = AllocMem(sizeof(struct ProcessPrivate), MEMF_ANY);
        animd->ad_ProcessData->pp_Object = (Object *)g;
        animd->ad_ProcessData->pp_Data = animd;
        
        InitSemaphore(&animd->ad_FrameData.afd_AnimFramesLock);
        InitSemaphore(&animd->ad_ColorData.acd_PenLock);

        animd->ad_ProcessData->pp_PlayerFlags = 0;
        animd->ad_ProcessData->pp_BufferFlags = 0;

        animd->ad_ProcessData->pp_BufferFrames = animd->ad_BufferTime * animd->ad_TimerData.atd_FramesPerSec;

        animd->ad_ProcessData->pp_BufferEnable = -1;
        animd->ad_ProcessData->pp_BufferDisable = -1;
        animd->ad_ProcessData->pp_BufferFill = -1;
        animd->ad_ProcessData->pp_BufferPurge = -1;

        animd->ad_ProcessData->pp_PlaybackEnable = -1;
        animd->ad_ProcessData->pp_PlaybackDisable = -1;
        animd->ad_ProcessData->pp_PlaybackTick = -1;
        
        GetAttr(DTA_Name, (Object *)g, (IPTR *)&projName);
        if (projName)
        {
            animd->ad_BaseName = FilePart(projName);
            animd->ad_ProcessData->pp_BufferingName = GenTaskName(animd->ad_BaseName, "Buffering");
            animd->ad_ProcessData->pp_PlayBackName = GenTaskName(animd->ad_BaseName, "Playback");
        }
        else
        {
            animd->ad_ProcessData->pp_BufferingName = GenTaskName("Animation", "Buffering");
            animd->ad_ProcessData->pp_PlayBackName = GenTaskName("Animation", "Playback");
        }
    }

    if ((animd->ad_ProcessData) && !(animd->ad_BufferProc))
    {
        // buffer proc gets 2x as much stack as playback since that is where the work happens
        animd->ad_BufferProc = CreateNewProcTags(
                            NP_Entry,           (IPTR)bufferProc,
                            NP_Name,            (IPTR)animd->ad_ProcessData->pp_BufferingName,
                            NP_Priority,        (IPTR)((BYTE)-1),
                            NP_Synchronous,     FALSE,
                            NP_Input,           Input (),
                            NP_CloseInput,      FALSE,
                            NP_Output,          Output (),
                            NP_CloseOutput,     FALSE,
                            NP_UserData,        (IPTR)animd->ad_ProcessData,
                            NP_StackSize,       (animd->ad_ProcStack << 1),
                            TAG_DONE);
        while (!(animd->ad_ProcessData->pp_BufferFlags & PRIVPROCF_RUNNING))
        {
            Delay (1);
        }
    }
    D(bug("[animation.datatype] %s: Buffering Process @ 0x%p\n", __func__, animd->ad_BufferProc);)

    if ((animd->ad_ProcessData) && !(animd->ad_PlayerProc))
    {
        animd->ad_PlayerProc = CreateNewProcTags(
                            NP_Entry,           (IPTR)playerProc,
                            NP_Name,            (IPTR)animd->ad_ProcessData->pp_PlayBackName,
                            NP_Priority,        0,
                            NP_Synchronous,     FALSE,
                            NP_Input,           Input (),
                            NP_CloseInput,      FALSE,
                            NP_Output,          Output (),
                            NP_CloseOutput,     FALSE,
                            NP_UserData,        (IPTR)animd->ad_ProcessData,
                            NP_StackSize,       animd->ad_ProcStack,
                            TAG_DONE);
        while (!(animd->ad_ProcessData->pp_PlayerFlags & PRIVPROCF_RUNNING))
        {
            Delay (1);
        }
    }
    D(bug("[animation.datatype] %s: Playback Process @ 0x%p\n", __func__, animd->ad_PlayerProc);)

    return 1;
}

IPTR DT_FreePens(struct IClass *cl, struct Gadget *g, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    int i;

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if ((animd->ad_ColorData.acd_ColorMap) && (animd->ad_ColorData.acd_NumAlloc > 0))
    {
        D(
            bug("[animation.datatype] %s: attempting to free %d pens\n", __func__, animd->ad_ColorData.acd_NumAlloc);
            bug("[animation.datatype] %s: colormap @ 0x%p\n", __func__, animd->ad_ColorData.acd_ColorMap);
        )
        ObtainSemaphore(&animd->ad_ColorData.acd_PenLock);
        for (i = animd->ad_ColorData.acd_NumAlloc - 1; i >= 0; i--)
        {
            D(bug("[animation.datatype] %s: freeing pen %d\n", __func__, animd->ad_ColorData.acd_Allocated[i]);)
            ReleasePen(animd->ad_ColorData.acd_ColorMap, animd->ad_ColorData.acd_Allocated[i]);
        }
        animd->ad_ColorData.acd_NumAlloc = 0;
        animd->ad_Flags &= ~ANIMDF_REMAPPEDPENS;
        ReleaseSemaphore(&animd->ad_ColorData.acd_PenLock);
    }

    return 1;
}

IPTR DT_FreeColorTables(struct IClass *cl, struct Gadget *g, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    UWORD havecolors, needcolors;

    D(bug("[animation.datatype]: %s()\n", __func__);)

    havecolors = needcolors = animd->ad_ColorData.acd_NumColors;
    
    if (animd->ad_ModeID & EXTRAHALFBRITE_KEY)
    {
        if (needcolors < 64)
            needcolors = 64;
        if (havecolors > 32)
            havecolors = 32;
    }

    if (animd->ad_ColorData.acd_ColorRegs)
    {
        FreeMem(animd->ad_ColorData.acd_ColorRegs, (1 + havecolors) * sizeof (struct ColorRegister));
        animd->ad_ColorData.acd_ColorRegs = NULL;
    }
    if (animd->ad_ColorData.acd_ColorTable[0])
    {
        FreeMem(animd->ad_ColorData.acd_ColorTable[0], (1 + havecolors) * sizeof (UBYTE));
        animd->ad_ColorData.acd_ColorTable[0] = NULL;
    }
    if (animd->ad_ColorData.acd_ColorTable[1])
    {
        FreeMem(animd->ad_ColorData.acd_ColorTable[1], (1 + needcolors) * sizeof (UBYTE));
        animd->ad_ColorData.acd_ColorTable[1] = NULL;
    }
    if (animd->ad_ColorData.acd_Allocated)
    {
        DoMethod((Object *)g, PRIVATE_FREEPENS);
        FreeMem(animd->ad_ColorData.acd_Allocated, (1 + needcolors) * sizeof (UBYTE));
        animd->ad_ColorData.acd_Allocated = NULL;
    }
    if (animd->ad_ColorData.acd_CRegs)
    {
        FreeMem(animd->ad_ColorData.acd_CRegs, (1 + havecolors) * (sizeof (ULONG) * 3));
        animd->ad_ColorData.acd_CRegs = NULL;
    }
    if (animd->ad_ColorData.acd_GRegs)
    {
        FreeMem(animd->ad_ColorData.acd_GRegs, (1 + needcolors) * (sizeof (ULONG) * 3));
        animd->ad_ColorData.acd_GRegs = NULL;
    }

    return 1;
}

IPTR DT_AllocColorTables(struct IClass *cl, struct Gadget *g, struct privAllocColorTables *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if ((msg->NumColors != animd->ad_ColorData.acd_NumColors) && 
        (animd->ad_ColorData.acd_NumColors > 0))
    {
        DoMethod((Object *)g, PRIVATE_FREECOLORTABLES);
    }

    if (msg->NumColors > 0)
    {
        UWORD havecolors, needcolors;

        havecolors = needcolors = msg->NumColors;

        if (animd->ad_ModeID & EXTRAHALFBRITE_KEY)
        {
            if (needcolors < 64)
                needcolors = 64;
            if (havecolors > 32)
                havecolors = 32;
        }

        animd->ad_ColorData.acd_ColorRegs = AllocMem((1 + havecolors) * sizeof (struct ColorRegister), MEMF_CLEAR);
        D(bug("[animation.datatype] %s: ColorRegs @ 0x%p\n", __func__, animd->ad_ColorData.acd_ColorRegs);)
        animd->ad_ColorData.acd_ColorTable[0] = AllocMem((1 + havecolors) * sizeof (UBYTE), MEMF_CLEAR);                // shared pen table
        D(bug("[animation.datatype] %s: ColorTable @ 0x%p\n", __func__, animd->ad_ColorData.acd_ColorTable[0]);)
        animd->ad_ColorData.acd_ColorTable[1] = AllocMem((1 + needcolors) * sizeof (UBYTE), MEMF_CLEAR);
        D(bug("[animation.datatype] %s: ColorTable2 @ 0x%p\n", __func__, animd->ad_ColorData.acd_ColorTable[1]);)
        animd->ad_ColorData.acd_Allocated = AllocMem((1 + needcolors) * sizeof (UBYTE), MEMF_CLEAR);
        D(bug("[animation.datatype] %s: Allocated pens Array @ 0x%p\n", __func__, animd->ad_ColorData.acd_Allocated);)
        animd->ad_ColorData.acd_CRegs = AllocMem((1 + havecolors) * (sizeof (ULONG) * 3), MEMF_CLEAR);                  // RGB32 triples used with SetRGB32CM
        D(bug("[animation.datatype] %s: CRegs @ 0x%p\n", __func__, animd->ad_ColorData.acd_CRegs);)
        animd->ad_ColorData.acd_GRegs = AllocMem((1 + needcolors) * (sizeof (ULONG) * 3), MEMF_CLEAR);                  // remapped version of ad_ColorData.acd_CRegs
        D(bug("[animation.datatype] %s: GRegs @ 0x%p\n", __func__, animd->ad_ColorData.acd_GRegs);)
    }

    animd->ad_ColorData.acd_NumColors = msg->NumColors;

    return 1;
}


IPTR DT_MapPens(struct IClass *cl, struct Gadget *g, struct privMapFramePens *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    struct TagItem bestpenTags[] =
    {
        { OBP_Precision,        animd->ad_ColorData.acd_PenPrecison     },
        { TAG_DONE,             0                                       }
    };
    ULONG mappedpen, color;
    UBYTE buffdepth;

    D(bug("[animation.datatype]: %s()\n", __func__);)

    /* TODO: We need to merge the colors so we get a good average to use! */
    if (msg->Frame->af_Frame.alf_CMap)
    {
        D(bug("[animation.datatype]: %s: loading frame colormap\n", __func__);)

        if (animd->ad_Flags & ANIMDF_REMAPPEDPENS)
            DoMethod((Object *)g, PRIVATE_FREEPENS);

        GetRGB32(msg->Frame->af_Frame.alf_CMap, 0,
            (msg->Frame->af_Frame.alf_CMap->Count < animd->ad_ColorData.acd_NumColors) ? msg->Frame->af_Frame.alf_CMap->Count : animd->ad_ColorData.acd_NumColors,
            animd->ad_ColorData.acd_CRegs);
    }

    if ((animd->ad_ColorData.acd_NumColors > 0) && !(animd->ad_Flags & ANIMDF_REMAPPEDPENS))
    {
        UWORD havecolors, needcolors;

        havecolors = needcolors = animd->ad_ColorData.acd_NumColors;

        if (animd->ad_ModeID & EXTRAHALFBRITE_KEY)
        {
            if (needcolors < 64)
                needcolors = 64;
            if (havecolors > 32)
                havecolors = 32;
        }

        buffdepth = (UBYTE)GetBitMapAttr(animd->ad_CacheBM, BMA_DEPTH);

        ObtainSemaphore(&animd->ad_ColorData.acd_PenLock);

        if ((animd->ad_Window) && !(animd->ad_ColorData.acd_ColorMap))
            animd->ad_ColorData.acd_ColorMap = animd->ad_Window->WScreen->ViewPort.ColorMap;

        D(bug("[animation.datatype] %s: colormap @ 0x%p\n", __func__, animd->ad_ColorData.acd_ColorMap);)

        for (color = 0; color < havecolors; color++)
        {
            if ((buffdepth <= 8) && ((mappedpen = animd->ad_ColorData.acd_NumAlloc++) < needcolors))
            {
                if (animd->ad_ColorData.acd_ColorMap)
                {
                    animd->ad_Flags |= ANIMDF_REMAPPEDPENS;

                    animd->ad_ColorData.acd_Allocated[mappedpen] = ObtainBestPenA(animd->ad_ColorData.acd_ColorMap,
                        animd->ad_ColorData.acd_CRegs[color * 3], animd->ad_ColorData.acd_CRegs[color * 3 + 1], animd->ad_ColorData.acd_CRegs[color * 3 + 2],
                        bestpenTags);

                    // get the actual color components for the pen.
                    GetRGB32(animd->ad_ColorData.acd_ColorMap,
                        animd->ad_ColorData.acd_Allocated[mappedpen], 1, &animd->ad_ColorData.acd_GRegs[mappedpen * 3]);

                    D(bug("[animation.datatype] %s: bestpen #%d for %02x%02x%02x\n", __func__, animd->ad_ColorData.acd_Allocated[mappedpen], (animd->ad_ColorData.acd_CRegs[color * 3] & 0xFF), (animd->ad_ColorData.acd_CRegs[color * 3 + 1] & 0xFF), (animd->ad_ColorData.acd_CRegs[color * 3 + 2] & 0xFF));)

                    animd->ad_ColorData.acd_ColorTable[0][color] = animd->ad_ColorData.acd_Allocated[mappedpen];
                    animd->ad_ColorData.acd_ColorTable[1][color] = animd->ad_ColorData.acd_Allocated[mappedpen];
                    
                    if (animd->ad_ModeID & EXTRAHALFBRITE_KEY)
                    {
                        D(bug("[animation.datatype] %s: allocating halfbrite pen %d\n", __func__, color + 32);)
                        if ((mappedpen = animd->ad_ColorData.acd_NumAlloc++) < needcolors)
                        {
                            animd->ad_ColorData.acd_Allocated[mappedpen] = ObtainBestPenA(animd->ad_ColorData.acd_ColorMap,
                                animd->ad_ColorData.acd_CRegs[color * 3] >> 1, animd->ad_ColorData.acd_CRegs[color * 3 + 1] >> 1, animd->ad_ColorData.acd_CRegs[color * 3 + 2] >> 1,
                                bestpenTags);
                            // get the actual color components for the pen.
                            GetRGB32(animd->ad_ColorData.acd_ColorMap,
                                animd->ad_ColorData.acd_Allocated[mappedpen], 1, &animd->ad_ColorData.acd_GRegs[mappedpen * 3]);
                             animd->ad_ColorData.acd_ColorTable[1][color + 32] = animd->ad_ColorData.acd_Allocated[mappedpen];
                        }
                        else
                        {
                            bug("[animation.datatype] %s: ERROR: out of pen storage\n", __func__);
                        }
                    }
                }
                else
                {
                    bug("[animation.datatype] %s: ERROR: no colormap to remap against\n", __func__);
                }
            }
            else
            {
                if (animd->ad_ColorData.acd_NumAlloc >= needcolors)
                {
                    bug("[animation.datatype] %s: ERROR: out of pen storage\n", __func__);
                }
                else
                {
                    animd->ad_ColorData.acd_ColorTable[0][color] = color;
                    animd->ad_ColorData.acd_ColorTable[1][color] = color;
                    animd->ad_ColorData.acd_GRegs[color * 3] = animd->ad_ColorData.acd_CRegs[color * 3];
                    animd->ad_ColorData.acd_GRegs[color * 3 + 1] = animd->ad_ColorData.acd_CRegs[color * 3  +1];
                    animd->ad_ColorData.acd_GRegs[color * 3 + 2] = animd->ad_ColorData.acd_CRegs[color * 3 + 2];
                    if (animd->ad_ModeID & EXTRAHALFBRITE_KEY)
                    {
                        animd->ad_ColorData.acd_GRegs[(color + 32) * 3] = animd->ad_ColorData.acd_CRegs[color * 3] >> 1;
                        animd->ad_ColorData.acd_GRegs[(color + 32) * 3 + 1] = animd->ad_ColorData.acd_CRegs[color * 3  +1] >> 1;
                        animd->ad_ColorData.acd_GRegs[(color + 32) * 3 + 2] = animd->ad_ColorData.acd_CRegs[color * 3 + 2] >> 1;
                    }
                }
            }
        }
        ReleaseSemaphore(&animd->ad_ColorData.acd_PenLock);
    }
    return 0;
}

IPTR DT_AllocBuffer(struct IClass *cl, struct Gadget *g, struct privAllocBuffer *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if (animd->ad_CacheBM)
        FreeBitMap(animd->ad_CacheBM);

    animd->ad_CacheBM = AllocBitMap(animd->ad_BitMapHeader.bmh_Width, animd->ad_BitMapHeader.bmh_Height, msg->Depth,
                                  BMF_CLEAR, msg->Friend);

    D(
        bug("[animation.datatype] %s: Cache BM @ 0x%p\n", __func__, animd->ad_CacheBM);
        bug("[animation.datatype] %s:     %dx%dx%d\n", __func__, animd->ad_BitMapHeader.bmh_Width, animd->ad_BitMapHeader.bmh_Height, msg->Depth);
    )

    return (IPTR)animd->ad_CacheBM;
}

IPTR DT_RenderFrame(struct IClass *cl, struct Gadget *g, struct privRenderFrame *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if ((msg->Frame) && (msg->Target))
    {
        D(bug("[animation.datatype]: %s: frame @ 0x%p, target @ 0x%p\n", __func__, (msg->Frame), (msg->Target));)

        if ((animd->ad_ColorData.acd_NumColors > 0) && (animd->ad_Flags & ANIMDF_REMAP))
        {
            D(bug("[animation.datatype]: %s: remapping frame\n", __func__);)

            DoMethod((Object *)g, PRIVATE_REMAPFRAME, msg->Frame, msg->Target);
        }
        else
        {
            struct BitMap *sourceBM = (struct BitMap *)msg->Frame->af_CacheBM;

            D(bug("[animation.datatype]: %s: rendering frame (cachebm @ 0x%p)\n", __func__, sourceBM);)

            if (!sourceBM)
                sourceBM = msg->Frame->af_Frame.alf_BitMap;
            
            D(bug("[animation.datatype]: %s: source bitmap @ 0x%p\n", __func__, sourceBM);)

            BltBitMap(sourceBM, 0, 0, msg->Target, 0, 0, animd->ad_BitMapHeader.bmh_Width, animd->ad_BitMapHeader.bmh_Height, 0xC0, 0xFF, NULL);
        }
    }

    D(bug("[animation.datatype]: %s: done\n", __func__);)

    return (IPTR)1;
}

UBYTE HAMFlag(UBYTE depth, UBYTE pen)
{
    if (depth == 8)
        return  ((pen & 0xC0) >> 6);
    return ((pen & 0x30) >> 4);
}

UBYTE HAMComponent(UBYTE depth, UBYTE pen)
{
    if (depth == 8)
        return  (pen & 0x3F);
    return (pen & 0xF);
}

UBYTE HAMColor(UBYTE depth, UBYTE pen, UBYTE val)
{
    if (depth < 8)
        return  (HAMComponent(depth, pen) << 4);
    return (val & 0x3) | (HAMComponent(depth, pen) << 2);
}

IPTR DT_RemapFrame(struct IClass *cl, struct Gadget *g, struct privRenderFrame *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    struct RastPort *remapRP, *targetRP;
    UBYTE *tmpline, *outline;
    UBYTE buffdepth, srcdepth;
    ULONG curpen;
    int i, x;

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if (msg->Frame)
    {
        // remap the frame bitmap ..
        if (msg->Target && ((tmpline = AllocVec(animd->ad_BitMapHeader.bmh_Width, MEMF_ANY)) != NULL))
        {
            buffdepth = (UBYTE)GetBitMapAttr(msg->Target, BMA_DEPTH);
            srcdepth = (UBYTE)GetBitMapAttr(msg->Frame->af_Frame.alf_BitMap, BMA_DEPTH);

            if (((animd->ad_ModeID & HAM_KEY) && (srcdepth <= 8)) || (buffdepth > 8))
            {
                D(
                    if (animd->ad_ModeID & HAM_KEY)
                        bug("[animation.datatype] %s: remapping HAM%d\n", __func__, srcdepth);
                    else
                        bug("[animation.datatype] %s: remapping to %dbit\n", __func__, buffdepth);
                )
                outline = AllocVec((animd->ad_BitMapHeader.bmh_Width << 2), MEMF_ANY);
            }
            else
                outline = tmpline;

            if ((remapRP = CreateRastPort()) != NULL)
            {
                if ((targetRP = CreateRastPort()) != NULL)
                {
                    remapRP->BitMap = msg->Frame->af_Frame.alf_BitMap;
                    targetRP->BitMap = msg->Target;

                    for(i = 0; i < animd->ad_BitMapHeader.bmh_Height; i++)
                    {
                        if ((animd->ad_ModeID & HAM_KEY) && (srcdepth <= 8))
                        {
                            UBYTE hamr = 0, hamg = 0, hamb = 0;

                            for(x = 0; x < animd->ad_BitMapHeader.bmh_Width; x++)
                            {
                                BOOL compose = TRUE;
                                ULONG mask = 1 << (7 - (x & 7));
                                UBYTE p;

                                tmpline[x] = 0;
                                for (p = 0; p < srcdepth; p++)
                                {
                                    UBYTE *planedata = (UBYTE *)msg->Frame->af_Frame.alf_BitMap->Planes[p];
                                    ULONG offset = (i * animd->ad_BitMapHeader.bmh_Width) + x;

                                    if ((planedata) && (planedata[offset / 8 ] & mask))
                                        tmpline[x] |=  (1 << p);
                                }

                                curpen = tmpline[x];
                                if (HAMFlag(srcdepth, curpen) == 0)
                                {
                                    curpen = HAMComponent(srcdepth, curpen);
                                    if (buffdepth <= 8)
                                    {
                                        compose = FALSE;
                                        outline[x] = animd->ad_ColorData.acd_ColorTable[1][curpen];
                                    }
                                    hamr = (animd->ad_ColorData.acd_GRegs[curpen * 3] & 0xFF);
                                    hamg = (animd->ad_ColorData.acd_GRegs[curpen * 3 + 1] & 0xFF);
                                    hamb = (animd->ad_ColorData.acd_GRegs[curpen * 3 + 2] & 0xFF);
                                }
                                else if (HAMFlag(srcdepth, curpen) == 1)
                                {
                                    //modify blue..
                                    hamb = HAMColor(srcdepth, curpen, hamb);
                                }
                                else if (HAMFlag(srcdepth, curpen) == 2)
                                {
                                    // modify red
                                    hamr = HAMColor(srcdepth, curpen, hamr);
                                }
                                else if (HAMFlag(srcdepth, curpen) == 3)
                                {
                                    //modify green
                                    hamg = HAMColor(srcdepth, curpen, hamg);
                                }

                                if (compose)
                                {
                                    if (buffdepth <= 8)
                                    {
                                        // TODO: Map pixel color
                                        outline[x] = 0;
                                    }
                                    else
                                    {
                                        outline[x * 4] = 0;
                                        outline[x * 4 + 1] = hamr;
                                        outline[x * 4 + 2] = hamg;
                                        outline[x * 4 + 3] = hamb;
                                    }
                                }
                            }
                            if (buffdepth <= 8)
                            {
                                D(bug("[animation.datatype] %s: HAM->CM WritePixelLine8(0x%p)\n", __func__, outline);)
                                WritePixelLine8(targetRP,0,i,animd->ad_BitMapHeader.bmh_Width,outline,NULL);
                            }
                            else
                            {
                                D(bug("[animation.datatype] %s: HAM->TC WritePixelArray(0x%p, RECTFMT_ARGB)\n", __func__, outline);)
                                WritePixelArray(outline, 0, 0, animd->ad_BitMapHeader.bmh_Width, targetRP, 0, i, animd->ad_BitMapHeader.bmh_Width, 1, RECTFMT_ARGB);
                            }
                        }
                        else
                        {
                            ReadPixelLine8(remapRP,0,i,animd->ad_BitMapHeader.bmh_Width,tmpline,NULL);

                            for(x = 0; x < animd->ad_BitMapHeader.bmh_Width; x++)
                            {
                                curpen = tmpline[x];
                                if (buffdepth <= 8)
                                    outline[x] = animd->ad_ColorData.acd_ColorTable[1][curpen];
                                else
                                {
                                    outline[x * 4] = 0;
                                    outline[x * 4 + 1] = (animd->ad_ColorData.acd_GRegs[curpen * 3] & 0xFF);
                                    outline[x * 4 + 2] = (animd->ad_ColorData.acd_GRegs[curpen * 3 + 1] & 0xFF);
                                    outline[x * 4 + 3] = (animd->ad_ColorData.acd_GRegs[curpen * 3 + 2] & 0xFF);
                                }
                            }

                            if (buffdepth <= 8)
                            {
                                D(bug("[animation.datatype] %s: ->CM WritePixelLine8(0x%p)\n", __func__, outline);)
                                WritePixelLine8(targetRP,0,i,animd->ad_BitMapHeader.bmh_Width,outline,NULL);
                            }
                            else
                            {
                                D(bug("[animation.datatype] %s: ->TC WritePixelArray(0x%p, RECTFMT_ARGB)\n", __func__, outline);)
                                WritePixelArray(outline, 0, 0, animd->ad_BitMapHeader.bmh_Width, targetRP, 0, i, animd->ad_BitMapHeader.bmh_Width, 1, RECTFMT_ARGB);
                            }
                        }
                    }
                    targetRP->BitMap = NULL;
                    FreeRastPort(targetRP);
                }
                remapRP->BitMap = NULL;
                FreeRastPort(remapRP);
            }

            if (outline != tmpline)
                FreeVec(outline);
            FreeVec(tmpline);
        }

        msg->Frame->af_Flags = AFFLAGF_READY;
    }

    return 1;
}

/*** PUBLIC METHODS ***/
IPTR DT_GetMethod(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    switch(msg->opg_AttrID)
    {
    case ADTA_KeyFrame:
        D(bug("[animation.datatype] %s: ADTA_KeyFrame\n", __func__);)
        *msg->opg_Storage = (IPTR) NULL;
        if (animd->ad_KeyFrame)
            *msg->opg_Storage = (IPTR) animd->ad_KeyFrame->af_Frame.alf_BitMap;
        break;

    case ADTA_ModeID:
        D(bug("[animation.datatype] %s: ADTA_ModeID\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ModeID;
        break;

    case ADTA_Width:
        D(bug("[animation.datatype] %s: ADTA_Width\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_BitMapHeader.bmh_Width;
        D(bug("[animation.datatype] %s:     = %d\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_Height:
        D(bug("[animation.datatype] %s: ADTA_Height\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_BitMapHeader.bmh_Height;
        D(bug("[animation.datatype] %s:     = %d\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_Depth:
        D(bug("[animation.datatype] %s: ADTA_Depth\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_BitMapHeader.bmh_Depth;
        D(bug("[animation.datatype] %s:     = %d\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_Frames:
        D(bug("[animation.datatype] %s: ADTA_Frames\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_FrameData.afd_Frames;
        break;

    case ADTA_Frame:
        D(bug("[animation.datatype] %s: ADTA_Frame\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_FrameData.afd_FrameCurrent;
        break;

    case ADTA_TicksPerFrame:
        D(bug("[animation.datatype] %s: ADTA_TicksPerFrame\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_TimerData.atd_TicksPerFrame;
        break;

    case ADTA_FramesPerSecond:
        D(bug("[animation.datatype] %s: ADTA_FramesPerSecond\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_TimerData.atd_FramesPerSec;
        break;

    case ADTA_FrameIncrement:
        D(bug("[animation.datatype] %s: ADTA_FrameIncrement\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_FrameData.afd_FramesStep;
        break;

    case ADTA_NumPrefetchFrames:
        *msg->opg_Storage = (IPTR) animd->ad_ProcessData->pp_BufferFrames;
        break;

    case ADTA_Sample:
        D(bug("[animation.datatype] %s: ADTA_Sample\n", __func__);)
        break;
    case ADTA_SampleLength:
        D(bug("[animation.datatype] %s: ADTA_SampleLength\n", __func__);)
        break;

    case ADTA_Period:
        D(bug("[animation.datatype] %s: ADTA_Period\n", __func__);)
        *msg->opg_Storage = (IPTR) 360UL;
        break;

    case ADTA_Volume:
        D(bug("[animation.datatype] %s: ADTA_Volume\n", __func__);)
        *msg->opg_Storage = (IPTR) 64UL;
        break;

    case ADTA_Cycles:
        D(bug("[animation.datatype] %s: ADTA_Cycles\n", __func__);)
        *msg->opg_Storage = (IPTR) 1UL;
        break;

    case ADTA_NumColors:
        D(bug("[animation.datatype] %s: ADTA_NumColors\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_NumColors;
        D(bug("[animation.datatype] %s:     = %d\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_NumAlloc:
        D(bug("[animation.datatype] %s: ADTA_NumAlloc\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_NumAlloc;
        D(bug("[animation.datatype] %s:     = %d\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_ColorRegisters:
        D(bug("[animation.datatype] %s: ADTA_ColorRegisters\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_ColorRegs;
        D(bug("[animation.datatype] %s:     = %p\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_ColorTable:
        D(bug("[animation.datatype] %s: ADTA_ColorTable\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_ColorTable[0];
        D(bug("[animation.datatype] %s:     = %p\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_ColorTable2:
        D(bug("[animation.datatype] %s: ADTA_ColorTable2\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_ColorTable[1];
        D(bug("[animation.datatype] %s:     = %p\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_Allocated:
        D(bug("[animation.datatype] %s: ADTA_Allocated\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_Allocated;
        D(bug("[animation.datatype] %s:     = %p\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_CRegs:
        D(bug("[animation.datatype] %s: ADTA_CRegs\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_CRegs;
        D(bug("[animation.datatype] %s:     = %p\n", __func__, *msg->opg_Storage);)
        break;

    case ADTA_GRegs:
        D(bug("[animation.datatype] %s: ADTA_GRegs\n", __func__);)
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_GRegs;
        D(bug("[animation.datatype] %s:     = %p\n", __func__, *msg->opg_Storage);)
        break;

    case PDTA_BitMapHeader:
        D(bug("[animation.datatype] %s: PDTA_BitMapHeader\n", __func__);)
        *msg->opg_Storage = (IPTR) &animd->ad_BitMapHeader;
        break;

    case DTA_TriggerMethods:
        D(bug("[animation.datatype] %s: DTA_TriggerMethods\n", __func__);)
        *msg->opg_Storage = (IPTR) SupportedTriggerMethods;
        break;

    case DTA_Methods:
        D(bug("[animation.datatype] %s: DTA_Methods\n", __func__);)
        *msg->opg_Storage = (IPTR) SupportedMethods;
        break;

    case DTA_ControlPanel:
        D(bug("[animation.datatype] %s: DTA_ControlPanel\n", __func__);)
        if ((animd->ad_Tapedeck) && (animd->ad_Flags & ANIMDF_CONTROLPANEL))
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case DTA_Immediate:
        D(bug("[animation.datatype] %s: DTA_Immediate\n", __func__);)
        if (animd->ad_Flags & ANIMDF_IMMEDIATE)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case DTA_Repeat:
        D(bug("[animation.datatype] %s: DTA_Repeat\n", __func__);)
        if (animd->ad_Flags & ANIMDF_REPEAT)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case ADTA_Remap:
        D(bug("[animation.datatype] %s: ADTA_Remap\n", __func__);)
        if (animd->ad_Flags & ANIMDF_REMAP)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case ADTA_AdaptiveFPS:
        D(bug("[animation.datatype] %s: ADTA_AdaptiveFPS\n", __func__);)
        if (animd->ad_Flags & ANIMDF_ADAPTFPS)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case ADTA_SmartSkip:
        D(bug("[animation.datatype] %s: ADTA_SmartSkip\n", __func__);)
        if (animd->ad_Flags & ANIMDF_SMARTSKIP)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case ADTA_OvertakeScreen:
        D(bug("[animation.datatype] %s: ADTA_OvertakeScreen\n", __func__);)
        if (animd->ad_Flags & ANIMDF_ADJUSTPALETTE)
            *msg->opg_Storage = (IPTR) TRUE;
        else
            *msg->opg_Storage = (IPTR) FALSE;
        break;

    case OBP_Precision:
        *msg->opg_Storage = (IPTR) animd->ad_ColorData.acd_PenPrecison;
        break;

    default:
        return DoSuperMethodA (cl, g, (Msg) msg);
    }

    return (IPTR)TRUE;
}

IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, g);
    struct TagItem *tstate = msg->ops_AttrList;
    struct TagItem attrtags[] =
    {
        { TAG_IGNORE,   0},
        { TAG_DONE,     0}
    };
    struct TagItem *tag;
    IPTR allocpens = 0;

    D(bug("[animation.datatype]: %s()\n", __func__);)

    while((tag = NextTagItem(&tstate)) != NULL)
    {
        switch(tag->ti_Tag)
        {
        case ADTA_ModeID:
            D(bug("[animation.datatype] %s: ADTA_ModeID (%08x)\n", __func__, tag->ti_Data);)
            animd->ad_ModeID = (IPTR) tag->ti_Data;
            D(
                if (animd->ad_ModeID & HAM_KEY)
                    D(bug("[animation.datatype] %s: HAM mode!\n", __func__);)
                else if (animd->ad_ModeID & EXTRAHALFBRITE_KEY)
                    D(bug("[animation.datatype] %s: EHB mode!\n", __func__);)
            )
            break;

        case ADTA_Width:
            D(bug("[animation.datatype] %s: ADTA_Width (%d)\n", __func__, tag->ti_Data);)
            animd->ad_BitMapHeader.bmh_Width = (UWORD) tag->ti_Data;
            attrtags[0].ti_Tag = DTA_NominalHoriz;
            attrtags[0].ti_Data = tag->ti_Data;
            SetAttrsA((Object *)g, attrtags);
            break;

        case ADTA_Height:
            D(bug("[animation.datatype] %s: ADTA_Height (%d)\n", __func__, tag->ti_Data);)
            animd->ad_BitMapHeader.bmh_Height = (UWORD) tag->ti_Data;
            attrtags[0].ti_Tag = DTA_NominalVert;
            if ((animd->ad_Tapedeck) && (animd->ad_Flags & ANIMDF_CONTROLPANEL))
                GetAttr(GA_Height, (Object *)animd->ad_Tapedeck, (IPTR *)&attrtags[0].ti_Data);
            else
                attrtags[0].ti_Data = 0;
            attrtags[0].ti_Data += tag->ti_Data;
            SetAttrsA((Object *)g, attrtags);
            break;

        case ADTA_Depth:
            D(bug("[animation.datatype] %s: ADTA_Depth (%d)\n", __func__, tag->ti_Data);)
            animd->ad_BitMapHeader.bmh_Depth = (UBYTE) tag->ti_Data;
            break;

        case ADTA_Frames:
            D(bug("[animation.datatype] %s: ADTA_Frames (%d)\n", __func__, tag->ti_Data);)
            animd->ad_FrameData.afd_Frames = (UWORD) tag->ti_Data;
            if (animd->ad_Tapedeck)
            {
                attrtags[0].ti_Tag = TDECK_Frames;
                attrtags[0].ti_Data = tag->ti_Data;
                SetAttrsA((Object *)animd->ad_Tapedeck, attrtags);
            }
            break;

        case ADTA_KeyFrame:
            D(bug("[animation.datatype] %s: ADTA_KeyFrame (0x%p)\n", __func__, tag->ti_Data);)
            if (!(animd->ad_KeyFrame))
                animd->ad_KeyFrame = AllocMem(sizeof(struct AnimFrame), MEMF_ANY|MEMF_CLEAR);
            animd->ad_KeyFrame->af_Frame.alf_BitMap = (struct BitMap *) tag->ti_Data;
            break;

        case ADTA_TicksPerFrame:
            D(bug("[animation.datatype] %s: ADTA_TicksPerFrame (%d)\n", __func__, tag->ti_Data);)
            animd->ad_TimerData.atd_TicksPerFrame = tag->ti_Data;
            break;

        case ADTA_FramesPerSecond:
            D(bug("[animation.datatype] %s: ADTA_FramesPerSecond (%d)\n", __func__, tag->ti_Data));
            animd->ad_TimerData.atd_FramesPerSec = (UWORD) tag->ti_Data;
            if (animd->ad_TimerData.atd_FramesPerSec == 0)
                animd->ad_TimerData.atd_FramesPerSec = 1;
            else if (animd->ad_TimerData.atd_FramesPerSec > 60)
                animd->ad_TimerData.atd_FramesPerSec = 60;
            D(bug("[animation.datatype] %s: = %d\n", __func__, animd->ad_TimerData.atd_FramesPerSec));
            animd->ad_TimerData.atd_TicksPerFrame = (ANIMPLAYER_TICKFREQ / animd->ad_TimerData.atd_FramesPerSec);
            break;

        case ADTA_FrameIncrement:
            D(bug("[animation.datatype] %s: ADTA_FrameIncrement (%d)\n", __func__, tag->ti_Data);)
            animd->ad_FrameData.afd_FramesStep = tag->ti_Data;
            break;

        case ADTA_NumPrefetchFrames:
            D(bug("[animation.datatype] %s: ADTA_NumPrefetchFrames (%d)\n", __func__, tag->ti_Data);)
            animd->ad_ProcessData->pp_BufferFrames = tag->ti_Data;
            break;

        case ADTA_NumColors:
            D(bug("[animation.datatype] %s: ADTA_NumColors (%d)\n", __func__, tag->ti_Data);)
            allocpens = tag->ti_Data;
            break;

        case ADTA_NumSparse:
            D(bug("[animation.datatype] %s: ADTA_NumSparse\n", __func__);)
            break;

        case ADTA_SparseTable:
            D(bug("[animation.datatype] %s: ADTA_SparseTable\n", __func__);)
            break;

        case ADTA_Screen:
            D(bug("[animation.datatype] %s: ADTA_Screen @ 0x%p\n", __func__, tag->ti_Data);)
            if (tag->ti_Data)
                animd->ad_ColorData.acd_ColorMap = ((struct Screen *)tag->ti_Data)->ViewPort.ColorMap;
            break;

        case OBP_Precision:
            D(bug("[animation.datatype] %s: OBP_Precision %08x\n", __func__, tag->ti_Data);)
            animd->ad_ColorData.acd_PenPrecison = (ULONG)tag->ti_Data;
            break;

        case SDTA_Sample:
            D(bug("[animation.datatype] %s: SDTA_Sample\n", __func__);)
            break;
        case SDTA_SampleLength:
            D(bug("[animation.datatype] %s: SDTA_SampleLength (%d)\n", __func__, tag->ti_Data);)
            break;
        case SDTA_Period:
            D(bug("[animation.datatype] %s: SDTA_Period (%d)\n", __func__, tag->ti_Data);)
            break;
        case SDTA_Volume:
            D(bug("[animation.datatype] %s: SDTA_Volume (%d)\n", __func__, tag->ti_Data);)
            break;

        case DTA_TopHoriz:
            D(bug("[animation.datatype] %s: DTA_TopHoriz (%d)\n", __func__, tag->ti_Data);)
            animd->ad_HorizTop = (UWORD) tag->ti_Data;
            break;

        case DTA_TotalHoriz:
            D(bug("[animation.datatype] %s: DTA_TotalHoriz (%d)\n", __func__, tag->ti_Data);)
            animd->ad_HorizTotal = (UWORD) tag->ti_Data;
            break;

        case DTA_VisibleHoriz:
            D(bug("[animation.datatype] %s: DTA_VisibleHoriz (%d)\n", __func__, tag->ti_Data);)
            animd->ad_HorizVis = (UWORD) tag->ti_Data;
            break;

        case DTA_TopVert:
            D(bug("[animation.datatype] %s: DTA_TopVert (%d)\n", __func__, tag->ti_Data);)
            animd->ad_VertTop = (UWORD) tag->ti_Data;
            break;

        case DTA_TotalVert:
            D(bug("[animation.datatype] %s: DTA_TotalVert (%d)\n", __func__, tag->ti_Data);)
            animd->ad_VertTotal = (UWORD) tag->ti_Data;
            break;

        case DTA_VisibleVert:
            D(bug("[animation.datatype] %s: DTA_VisibleVert (%d)\n", __func__, tag->ti_Data);)
            animd->ad_VertVis = (UWORD) tag->ti_Data;
            break;

        case DTA_ControlPanel:
            D(bug("[animation.datatype] %s: DTA_ControlPanel\n", __func__);)
            if ((animd->ad_Tapedeck) && (tag->ti_Data))
                animd->ad_Flags |= ANIMDF_CONTROLPANEL;
            else
                animd->ad_Flags &= ~(ANIMDF_CONTROLPANEL);
            break;

        case DTA_Immediate:
            D(bug("[animation.datatype] %s: DTA_Immediate\n", __func__);)
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_IMMEDIATE;
            else
                animd->ad_Flags &= ~(ANIMDF_IMMEDIATE);
            break;

        case DTA_Repeat:
            D(bug("[animation.datatype] %s: DTA_Repeat\n", __func__);)
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_REPEAT;
            else
                animd->ad_Flags &= ~(ANIMDF_REPEAT);
            break;

        case ADTA_Remap:
            D(bug("[animation.datatype] %s: ADTA_Remap\n", __func__);)
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_REMAP;
            else
                animd->ad_Flags &= ~(ANIMDF_REMAP);
            break;

        case ADTA_AdaptiveFPS:
            D(bug("[animation.datatype] %s: ADTA_AdaptiveFPS\n", __func__);)
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_ADAPTFPS;
            else
                animd->ad_Flags &= ~(ANIMDF_ADAPTFPS);
            break;

        case ADTA_SmartSkip:
            D(bug("[animation.datatype] %s: ADTA_SmartSkip\n", __func__);)
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_SMARTSKIP;
            else
                animd->ad_Flags &= ~(ANIMDF_SMARTSKIP);
            break;

        case ADTA_OvertakeScreen:
            D(bug("[animation.datatype] %s: ADTA_OvertakeScreen\n", __func__);)
            if (tag->ti_Data)
                animd->ad_Flags |= ANIMDF_ADJUSTPALETTE;
            else
                animd->ad_Flags &= ~(ANIMDF_ADJUSTPALETTE);
            break;
        }
    }

    if (allocpens > 0)
    {
        /*
         * we allocate pens now since the ModeID may
         * have been changed after the number of colors
         */
        DoMethod((Object *)g, PRIVATE_ALLOCCOLORTABLES, allocpens);
    }

    return (DoSuperMethodA (cl, g, (Msg) msg));
}

IPTR DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct Gadget *g;
    struct Animation_Data *animd;
    struct TagItem tdtags[] =
    {
        { GA_RelVerify, TRUE},
        { GA_Width, 200},
        { GA_Height, 15},
        { TAG_DONE, 0}
    };

    D(bug("[animation.datatype]: %s()\n", __func__);)

    g = (struct Gadget *) DoSuperMethodA(cl, o, (Msg) msg);
    if (g)
    {
        D(
            bug("[animation.datatype] %s: Created object 0x%p\n", __func__, g);
            bug("[animation.datatype] %s: for '%s'\n", __func__, OCLASS((Object *)g)->cl_ID);
        )
        animd = (struct Animation_Data *) INST_DATA(cl, g);
        memset(animd, 0 , sizeof(struct Animation_Data));

        NewList(&animd->ad_FrameData.afd_AnimFrames);
        D(bug("[animation.datatype] %s: FrameList @ 0x%p\n", __func__, &animd->ad_FrameData.afd_AnimFrames);)

        animd->ad_Flags = ANIMDF_CONTROLPANEL|ANIMDF_REMAP|ANIMDF_FRAMESKIP;
#if (1)
        animd->ad_Flags |= (ANIMDF_REPEAT|ANIMDF_IMMEDIATE);
#endif
        animd->ad_TimerData.atd_FramesPerSec = 60;
        animd->ad_TimerData.atd_TicksPerFrame = ANIMPLAYER_TICKFREQ / animd->ad_TimerData.atd_FramesPerSec;
        animd->ad_FrameData.afd_FramesStep= 10;
        animd->ad_ColorData.acd_PenPrecison = PRECISION_IMAGE;
        animd->ad_ProcStack = 8192;
        animd->ad_BufferStep = 4; // Try to load 4 frames at a time
        animd->ad_BufferTime = 3;

        if (msg->ops_AttrList)
        {
            D(bug("[animation.datatype] %s: Setting attributes.. \n", __func__);)
            DT_SetMethod(cl, g, msg);
        }

        ReadENVPrefs(g, animd);

        D(bug("[animation.datatype] %s: Prepare controls.. \n", __func__);)

        /* create a tapedeck gadget */
        if ((animd->ad_Tapedeck = NewObjectA(NULL, "tapedeck.gadget", tdtags)) != NULL)
        {
            D(bug("[animation.datatype] %s: Tapedeck @ 0x%p\n", __func__, animd->ad_Tapedeck);)
        }

        DT_InitPlayer(cl, g, (Msg) msg);
    }

    D(bug("[animation.datatype] %s: returning %p\n", __func__, g);)

    return (IPTR)g;
}

IPTR DT_RemoveDTObject(struct IClass *cl, Object *o, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, o);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    DoMethod(o, ADTM_STOP);

    if (animd->ad_ProcessData)
    {
        // disable our subprocesses ..
        Signal((struct Task *)animd->ad_BufferProc, (1 << animd->ad_ProcessData->pp_BufferDisable));
        Signal((struct Task *)animd->ad_PlayerProc, (1 << animd->ad_ProcessData->pp_PlaybackDisable));

        // wait for them to finish ...
        while (animd->ad_ProcessData->pp_BufferFlags & PRIVPROCF_ACTIVE)
        {
            Delay (1);
        }
        while (animd->ad_ProcessData->pp_PlayerFlags & PRIVPROCF_ACTIVE)
        {
            Delay (1);
        }
        Delay (2);
    }

    return DoSuperMethodA(cl, o, msg);
}

IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
    struct Animation_Data *animd = INST_DATA (cl, o);
    struct AnimFrame *curFrame = NULL, *lastFrame = NULL;

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if (animd->ad_Player)
        DeletePlayer(animd->ad_Player);

    if (animd->ad_PlayerProc)
    {
        Signal((struct Task *)animd->ad_PlayerProc, SIGBREAKF_CTRL_C);

        while (animd->ad_ProcessData->pp_PlayerFlags & PRIVPROCF_RUNNING)
        {
            Delay (1);
        }
    }

    if (animd->ad_BufferProc)
    {
        Signal((struct Task *)animd->ad_BufferProc, SIGBREAKF_CTRL_C);

        while (animd->ad_ProcessData->pp_BufferFlags & PRIVPROCF_RUNNING)
        {
            Delay (1);
        }
    }


    if (animd->ad_ProcessData)
    {
        FreeVec(animd->ad_ProcessData->pp_BufferingName);
        FreeVec(animd->ad_ProcessData->pp_PlayBackName);
        FreeMem(animd->ad_ProcessData, sizeof(struct ProcessPrivate));
    }

    ForeachNodeSafe(&animd->ad_FrameData.afd_AnimFrames, curFrame, lastFrame)
    {
        D(bug("[animation.datatype] %s: disposing of frame @ 0x%p\n", __func__, curFrame);)
        Remove(&curFrame->af_Node);
        FreeMem(curFrame, sizeof(struct AnimFrame));
    }

    DoMethod(o, PRIVATE_FREECOLORTABLES);

    if (animd->ad_CacheBM)
        FreeBitMap(animd->ad_CacheBM);

    if (animd->ad_Tapedeck)
        DisposeObject (animd->ad_Tapedeck);

    return DoSuperMethodA(cl, o, msg);
}

IPTR DT_GoInActiveMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return DoSuperMethodA(cl, (Object *)g, (Msg)msg);
}

IPTR DT_HandleInputMethod(struct IClass *cl, struct Gadget *g, struct gpInput *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);
    struct InputEvent *ie = msg->gpi_IEvent;
    struct RastPort     *rport;
    IPTR retval = GMR_MEACTIVE;
    IPTR tdHeight = 0;
    BOOL redraw = FALSE;
    ULONG condstate = 0;

   D(bug("[animation.datatype]: %s(%d,%d)\n", __func__, msg->gpi_Mouse.X, msg->gpi_Mouse.Y);)

    if (ie->ie_Code == SELECTDOWN)
    {
        D(bug("[animation.datatype]: %s: SELECTDOWN\n", __func__);)
        g->Flags |= GFLG_SELECTED;
    }

    if (animd->ad_Tapedeck)
        GetAttr(GA_Height, (Object *)animd->ad_Tapedeck, &tdHeight);

    if ((animd->ad_Tapedeck) &&
        ((((struct Gadget *)animd->ad_Tapedeck)->Flags & GFLG_SELECTED) ||
        (msg->gpi_Mouse.Y > animd->ad_RenderHeight)))
    {
        IPTR tdMode = 0;

        D(bug("[animation.datatype]: %s: input event is for the tapedeck ...\n", __func__);)

        if ((!(((struct Gadget *)animd->ad_Tapedeck)->Flags & GFLG_SELECTED)) &&
            ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == SELECTDOWN)))
        {
            if ((animd->ad_PlayerSourceLastState = animd->ad_Player->pl_Source->cdt_State) != CONDSTATE_STOPPED)
                SetConductorState (animd->ad_Player, CONDSTATE_PAUSED, animd->ad_FrameData.afd_FrameCurrent * animd->ad_TimerData.atd_TicksPerFrame);
        }

        /* pass it to the tapedeck gadget .. */
        msg->gpi_Mouse.Y -= animd->ad_RenderHeight;
        retval = DoMethodA((Object *)animd->ad_Tapedeck, (Msg)msg);
        msg->gpi_Mouse.Y += animd->ad_RenderHeight;

        GetAttr(TDECK_Mode, (Object *)animd->ad_Tapedeck, &tdMode);
        if ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == SELECTUP))
        {
            if ((tdMode == BUT_PLAY) && (animd->ad_PlayerSourceLastState != CONDSTATE_RUNNING))
                animd->ad_PlayerSourceLastState = CONDSTATE_RUNNING;
            if ((tdMode == BUT_PAUSE) && (animd->ad_PlayerSourceLastState != CONDSTATE_PAUSED))
                animd->ad_PlayerSourceLastState = CONDSTATE_PAUSED;
        }

        if (((tdMode == BUT_FRAME) || (tdMode == BUT_REWIND) || (tdMode == BUT_FORWARD)) &&
            ((ie->ie_Class == IECLASS_TIMER) ||
            ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == SELECTDOWN))))
        {
            IPTR tdFrame = 0;

            GetAttr(TDECK_CurrentFrame, (Object *)animd->ad_Tapedeck, &tdFrame);

            if (tdMode == BUT_REWIND)
                tdFrame -= animd->ad_FrameData.afd_FramesStep;
            else if (tdMode == BUT_FORWARD)
                tdFrame += animd->ad_FrameData.afd_FramesStep;

            if (tdFrame < 0)
                tdFrame = 0;
            if (tdFrame >= animd->ad_FrameData.afd_Frames)
                tdFrame -= animd->ad_FrameData.afd_Frames;

            if (tdFrame != animd->ad_FrameData.afd_FrameCurrent)
            {
                condstate = CONDSTATE_SHUTTLE;
                animd->ad_FrameData.afd_FrameCurrent = tdFrame;
                redraw = TRUE;
            }
        }
        else if (((tdMode == BUT_FRAME) || (tdMode == BUT_REWIND) || (tdMode == BUT_FORWARD)) &&
            ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == SELECTUP)))
        {
                redraw = TRUE;
        }
    }

    if ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == SELECTUP))
    {
        D(bug("[animation.datatype]: %s: SELECTUP\n", __func__);)

        g->Flags &= ~GFLG_SELECTED;
        retval = GMR_NOREUSE;
        condstate = animd->ad_PlayerSourceLastState;
    }

    if (condstate)
    {
        D(bug("[animation.datatype]: %s: updating conductor...\n", __func__);)
        if ((condstate == CONDSTATE_RUNNING) ||
            (condstate == CONDSTATE_SHUTTLE))
        {
            Signal((struct Task *)animd->ad_BufferProc, (1 << animd->ad_ProcessData->pp_BufferEnable));
            Signal((struct Task *)animd->ad_PlayerProc, (1 << animd->ad_ProcessData->pp_PlaybackEnable));
        }
        SetConductorState(animd->ad_Player, condstate, animd->ad_FrameData.afd_FrameCurrent * animd->ad_TimerData.atd_TicksPerFrame);
    }

    if ((redraw) &&
        (rport = ObtainGIRPort(msg->gpi_GInfo)))
    {
        struct gpRender gpr;

        gpr.MethodID   = GM_RENDER;
        gpr.gpr_GInfo  = msg->gpi_GInfo;
        gpr.gpr_RPort  = rport;
        gpr.gpr_Redraw = GREDRAW_REDRAW;
        DoMethodA((Object *)g, &gpr);

        ReleaseGIRPort(rport);
    }

    return retval;
}

IPTR DT_HelpTestMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)

    return (IPTR)NULL;
}

IPTR DT_HitTestMethod(struct IClass *cl, struct Gadget *g, struct gpHitTest *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)

    return GMR_GADGETHIT;
}

IPTR DT_Layout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);
    struct IBox *gadBox = NULL;
    IPTR RetVal, totalheight;

    D(bug("[animation.datatype]: %s()\n", __func__);)

    animd->ad_Flags |= ANIMDF_LAYOUT;

    // cache the window pointer
    animd->ad_Window = msg->gpl_GInfo->gi_Window;

    RetVal = DoSuperMethodA(cl, (Object *)g, (Msg)msg);

    GetAttr(DTA_Domain, (Object *)g, (IPTR *)&gadBox);

    animd->ad_RenderLeft = gadBox->Left;
    animd->ad_RenderTop = gadBox->Top;
    if (gadBox->Width > animd->ad_BitMapHeader.bmh_Width)
        animd->ad_RenderWidth = animd->ad_BitMapHeader.bmh_Width;
    else
        animd->ad_RenderWidth = gadBox->Width;
        
    if (gadBox->Height > animd->ad_BitMapHeader.bmh_Height)
        animd->ad_RenderHeight = animd->ad_BitMapHeader.bmh_Height;
    else
        animd->ad_RenderHeight = gadBox->Height;

    totalheight = animd->ad_BitMapHeader.bmh_Height;

    // propogate our known dimensions to the tapedeck ..
    if (animd->ad_Tapedeck)
    {
        struct TagItem tdAttrs[] =
        {
            { GA_Left,          animd->ad_RenderLeft                            },
            { GA_Top,           0                                               },
            { GA_Width,         animd->ad_BitMapHeader.bmh_Width                },
            { GA_Height,        0                                               },
            { TAG_DONE,         0                                               }
        };

        GetAttr(GA_Height, (Object *)animd->ad_Tapedeck, (IPTR *)&tdAttrs[3].ti_Data);
        totalheight += tdAttrs[3].ti_Data;
        D(bug("[animation.datatype]: %s: tapedeck height = %d\n", __func__, tdAttrs[3].ti_Data);)

        animd->ad_Flags |= ANIMDF_SHOWPANEL;

        // try to adjust to accomodate it ..
        if (gadBox->Height >= totalheight)
            tdAttrs[1].ti_Data = animd->ad_RenderTop + animd->ad_BitMapHeader.bmh_Height;
        else
        {
            if (gadBox->Height > tdAttrs[3].ti_Data)
            {
                animd->ad_RenderHeight = gadBox->Height - tdAttrs[3].ti_Data;
                tdAttrs[1].ti_Data = animd->ad_RenderTop + animd->ad_RenderHeight;
            }
            else
            {
                D(bug("[animation.datatype]: %s: tapedeck too big for visible space!\n", __func__);)
                animd->ad_Flags &= ~ANIMDF_SHOWPANEL;
            }
        }

        if (animd->ad_RenderWidth > animd->ad_BitMapHeader.bmh_Width)
            tdAttrs[2].ti_Data = animd->ad_BitMapHeader.bmh_Width;
        else
            tdAttrs[2].ti_Data = animd->ad_RenderWidth;

        SetAttrsA((Object *)animd->ad_Tapedeck, tdAttrs);

        // .. and ask it to layout
        // NB - Do not use async layout or it crashes .. =(
        DoMethod((Object *)animd->ad_Tapedeck,
            GM_LAYOUT, (IPTR)msg->gpl_GInfo, (IPTR)msg->gpl_Initial);
    }

    animd->ad_Flags &= ~ANIMDF_LAYOUT;

    {
        struct TagItem notifyAttrs[] =
        {
            { GA_ID,            g->GadgetID                             },
            { DTA_Busy,         FALSE                                   },
            { DTA_TotalHoriz,   animd->ad_BitMapHeader.bmh_Width        },
            { DTA_VisibleHoriz, animd->ad_RenderWidth                           },
            { DTA_TotalVert,    totalheight                             },
            { DTA_VisibleVert,  gadBox->Height                          },
            { TAG_DONE,         0                                       }
        };

        DoMethod((Object *) g, OM_NOTIFY, notifyAttrs, (IPTR) msg->gpl_GInfo, 0);
    }

    if ((msg->gpl_Initial) && (animd->ad_Flags & ANIMDF_IMMEDIATE))
    {
        DoMethod((Object *)g, ADTM_START, 0);
    }

    return RetVal;
}

IPTR DT_Render(struct IClass *cl, struct Gadget *g, struct gpRender *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if (!animd->ad_CacheBM)
    {
        IPTR bmdepth;

        bmdepth = GetBitMapAttr(msg->gpr_RPort->BitMap, BMA_DEPTH);
        if (bmdepth < animd->ad_BitMapHeader.bmh_Depth)
            bmdepth = animd->ad_BitMapHeader.bmh_Depth;

        DoMethod((Object *)g, PRIVATE_ALLOCBUFFER, msg->gpr_RPort->BitMap, bmdepth);

        if (animd->ad_KeyFrame)
        {
            D(bug("[animation.datatype] %s: rendering keyframe\n", __func__);)
            DoMethod((Object *)g, PRIVATE_MAPFRAMEPENS, animd->ad_KeyFrame);
            DoMethod((Object *)g, PRIVATE_RENDERFRAME, animd->ad_KeyFrame, animd->ad_CacheBM);
#if (0)
            animd->ad_FrameBM =  animd->ad_KeyFrame->af_Frame.alf_BitMap;
#else
            animd->ad_FrameBM =  animd->ad_CacheBM;
#endif
        }
    }

    if (animd->ad_ProcessData && (animd->ad_ProcessData->pp_PlayerFlags & PRIVPROCF_RUNNING))
    {
        struct BitMap * renderBM = animd->ad_FrameBM;

        if ((!renderBM) && (animd->ad_KeyFrame))
        {
            renderBM =  animd->ad_KeyFrame->af_Frame.alf_BitMap;
        }
        LockLayer(0, msg->gpr_GInfo->gi_Window->WLayer);

        if (renderBM)
        {
            D(bug("[animation.datatype] %s: rendering frame bitmap @ 0x%p\n", __func__, renderBM);)

            BltBitMapRastPort(renderBM,
                animd->ad_HorizTop, animd->ad_VertTop,
                msg->gpr_RPort,
                animd->ad_RenderLeft, animd->ad_RenderTop, animd->ad_RenderWidth, animd->ad_RenderHeight, 0xC0);
        }
        else
        {
            // for now fill the animations area
            D(bug("[animation.datatype] %s: clearing display area\n", __func__);)

            SetRPAttrs(msg->gpr_RPort, RPTAG_FgColor, 0xEE8888, TAG_DONE );
            RectFill(msg->gpr_RPort,
                animd->ad_RenderLeft, animd->ad_RenderTop,
                animd->ad_RenderLeft + animd->ad_RenderWidth - 1, animd->ad_RenderTop + animd->ad_RenderHeight - 1);
        }

        if (animd->ad_Flags & ANIMDF_SHOWPANEL)
        {
            DoMethodA((Object *)animd->ad_Tapedeck, (Msg)msg);
        }

        UnlockLayer(msg->gpr_GInfo->gi_Window->WLayer);
    }
    return (IPTR)TRUE;
}

IPTR DT_FrameBox(struct IClass *cl, struct Gadget *g, struct dtFrameBox *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return (IPTR)NULL;
}

IPTR DT_ProcLayout(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return (IPTR)NULL;
}

IPTR DT_Print(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return (IPTR)NULL;
}

IPTR DT_Copy(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return (IPTR)NULL;
}

IPTR DT_Trigger(struct IClass *cl, struct Gadget *g, struct dtTrigger *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)

    return DoSuperMethodA (cl, (Object *)g, (Msg)msg);
}

IPTR DT_Write(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return (IPTR)NULL;
}

IPTR DT_Locate(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return (IPTR)NULL;
}

IPTR DT_Pause(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if (animd->ad_Tapedeck)
    {
        struct TagItem tdAttrs[] =
        {
            { TDECK_Mode,          BUT_STOP                                    },
            { TAG_DONE,         0                                               }
        };
        SetAttrsA((Object *)animd->ad_Tapedeck, tdAttrs);
    }
    SetConductorState (animd->ad_Player, CONDSTATE_PAUSED, animd->ad_FrameData.afd_FrameCurrent * animd->ad_TimerData.atd_TicksPerFrame);

    return (IPTR)NULL;
}

IPTR DT_Start(struct IClass *cl, struct Gadget *g, struct adtStart *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if (animd->ad_Tapedeck)
    {
        struct TagItem tdAttrs[] =
        {
            { TDECK_Mode,          BUT_PLAY                                    },
            { TAG_DONE,         0                                               }
        };
        SetAttrsA((Object *)animd->ad_Tapedeck, tdAttrs);
    }
    SetConductorState(animd->ad_Player, CONDSTATE_RUNNING, msg->asa_Frame * animd->ad_TimerData.atd_TicksPerFrame);
    if (animd->ad_ProcessData)
    {
        // enable our subprocesses ..
        Signal((struct Task *)animd->ad_BufferProc, (1 << animd->ad_ProcessData->pp_BufferEnable));
        Signal((struct Task *)animd->ad_PlayerProc, (1 << animd->ad_ProcessData->pp_PlaybackEnable));
    }

    return (IPTR)NULL;
}

IPTR DT_Stop(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    struct Animation_Data *animd = INST_DATA (cl, (Object *)g);

    D(bug("[animation.datatype]: %s()\n", __func__);)

    if (animd->ad_Tapedeck)
    {
        struct TagItem tdAttrs[] =
        {
            { TDECK_Mode,          BUT_STOP                                    },
            { TAG_DONE,         0                                               }
        };
        SetAttrsA((Object *)animd->ad_Tapedeck, tdAttrs);
    }
    SetConductorState(animd->ad_Player, CONDSTATE_STOPPED, 0);

    return (IPTR)NULL;
}

IPTR DT_LoadNewFormatFrame(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return (IPTR)NULL;
}

IPTR DT_UnLoadNewFormatFrame(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    D(bug("[animation.datatype]: %s()\n", __func__);)
    return (IPTR)NULL;
}
