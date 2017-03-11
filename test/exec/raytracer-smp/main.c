/*
    Copyright 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/processor.h>
#include <proto/kernel.h>

#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>

#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <exec/rawfmt.h>
#include <resources/processor.h>
#include <cybergraphx/cybergraphics.h>

#include "renderer.h"

CONST_STRPTR version = "$VER: SMP-Smallpt 1.0 (03.03.2017) ©2017 The AROS Development Team";

APTR KernelBase;

struct Window * createMainWindow()
{
    struct Screen *pubScreen;
    struct Window *displayWin = NULL;
    int width, height;

    pubScreen = LockPubScreen(0);

    if (pubScreen)
    {
        width = ((pubScreen->Width * 4) / 5) & ~0x1f;
        height = (width * 3 / 4) & ~0x1f;

        if (height >= (pubScreen->Height * 4) / 5)
        {
            height = ((pubScreen->Height * 4) / 5) & ~0x1f;
            width = (height * 4 / 3) & ~0x1f;
        }
    }
    else
    {
        width = 320;
        height = 240;
    }

    if ((displayWin = OpenWindowTags(0,
                                     WA_PubScreen, (IPTR)pubScreen,
                                     WA_Left, 0,
                                     WA_Top, (pubScreen) ? pubScreen->BarHeight : 10,
                                     WA_InnerWidth, width,
                                     WA_InnerHeight, height,
                                     WA_Title, (IPTR) "SMP-Smallpt renderer",
                                     WA_SimpleRefresh, TRUE,
                                     WA_CloseGadget, TRUE,
                                     WA_DepthGadget, TRUE,
                                     WA_DragBar, TRUE,
                                     WA_SizeGadget, FALSE,
                                     WA_SizeBBottom, FALSE,
                                     WA_SizeBRight, FALSE,
                                     WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW,
                                     TAG_DONE)) != NULL)
    {
        if (pubScreen)
            UnlockPubScreen(0, pubScreen);

        SetWindowPointer(displayWin, WA_BusyPointer, TRUE, TAG_DONE);
    }

    return displayWin;
}

int main()
{
    APTR ProcessorBase;
    //char buffer[200];

    struct Window *displayWin;
    struct BitMap *outputBMap = NULL;

    IPTR coreCount = 1;
    struct TagItem tags[] =
        {
            {GCIT_NumberOfProcessors, (IPTR)&coreCount},
            {0, (IPTR)NULL}};

    ProcessorBase = OpenResource(PROCESSORNAME);
    if (!ProcessorBase)
        return 0;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return 0;

    GetCPUInfo(tags);

    D(bug("[SMP-Smallpt] %s: detected %d CPU cores\n", __func__, coreCount);)

//    NewRawDoFmt("Hello %s", RAWFMTFUNC_STRING, buffer, "world!");

    displayWin = createMainWindow();
    if (displayWin)
    {
        int width, height;
        struct RastPort *outBMRastPort;
        ULONG *workBuffer;
        int windowClosing = FALSE;
        struct MsgPort *mainPort = CreateMsgPort();
        struct MsgPort *rendererPort = NULL;
        ULONG signals;
        struct Task *renderer;
        struct Message *msg;
        struct MyMessage cmd;

        width = (displayWin->Width - displayWin->BorderLeft - displayWin->BorderRight);
        height = (displayWin->Height - displayWin->BorderTop - displayWin->BorderBottom);

        D(bug("[SMP-Smallpt] %s: Created window with inner size of %dx%d\n", __func__, width, height);)
        D(bug("[SMP-Smallpt] %s: Tiles amount %dx%d\n", __func__, width / 32, height / 32);)

        outputBMap = AllocBitMap(
                        width,
                        height,
                        GetBitMapAttr(displayWin->WScreen->RastPort.BitMap, BMA_DEPTH),
                        BMF_DISPLAYABLE, displayWin->WScreen->RastPort.BitMap);

        outBMRastPort = CreateRastPort();
        outBMRastPort->BitMap = outputBMap;

        workBuffer = AllocMem(width * height * sizeof(ULONG), MEMF_ANY|MEMF_CLEAR);

        WritePixelArray(workBuffer,
                        0, 0, width * sizeof(ULONG),
                        outBMRastPort,
                        0, 0,
                        width, height,
                        RECTFMT_ARGB);

        BltBitMapRastPort (outputBMap, 0, 0,
            displayWin->RPort, displayWin->BorderLeft, displayWin->BorderTop,
            width, height, 0xC0); 

        D(bug("[SMP-Smallpt] %s: Creating renderer task\n", __func__);)

        renderer = NewCreateTask(TASKTAG_NAME,      "SMP-Smallpt Master",
                                TASKTAG_AFFINITY,   TASKAFFINITY_ANY,
                                TASKTAG_PRI,        -1,
                                TASKTAG_PC,         Renderer,
                                TASKTAG_ARG1,       SysBase,
                                TASKTAG_ARG2,       mainPort,
                                //TASKTAG_STACKSIZE,  10000000,
                                //TASKTAG_USERDATA   , &workMaster,
                                TAG_DONE);
        (void)renderer;

        D(bug("[SMP-Smallpt] %s: waiting for welcome message form renderer\n", __func__);)
        WaitPort(mainPort);
        msg = GetMsg(mainPort);
        rendererPort = msg->mn_ReplyPort;
        ReplyMsg(msg);

        cmd.mm_Type = MSG_STARTUP;
        cmd.mm_Message.mn_Length = sizeof(cmd);
        cmd.mm_Message.mn_ReplyPort = mainPort;
        cmd.mm_Body.Startup.ChunkyBM = workBuffer;
        cmd.mm_Body.Startup.Width = width;
        cmd.mm_Body.Startup.Height = height;
        cmd.mm_Body.Startup.coreCount = coreCount;

        D(bug("[SMP-Smallpt] %s: renderer alive. sending startup message\n", __func__);)
        
        PutMsg(rendererPort, &cmd.mm_Message);
        WaitPort(mainPort);
        GetMsg(mainPort);

        D(bug("[SMP-Smallpt] %s: enter main loop\n", __func__);)

        while ((!windowClosing) && ((signals = Wait(SIGBREAKF_CTRL_D | (1 << displayWin->UserPort->mp_SigBit) | (1 << mainPort->mp_SigBit))) != 0))
        {
            // CTRL_D is redraw signal
            if (signals & SIGBREAKF_CTRL_D)
            {
                WritePixelArray(workBuffer,
                                            0, 0, width * sizeof(ULONG),
                                            outBMRastPort,
                                            0, 0,
                                            width, height,
                                            RECTFMT_ARGB);

                BltBitMapRastPort (outputBMap, 0, 0,
                    displayWin->RPort, displayWin->BorderLeft, displayWin->BorderTop,
                    width, height, 0xC0); 
            }
            if (signals & (1 << displayWin->UserPort->mp_SigBit))
            {
                struct IntuiMessage *msg;
                while ((msg = (struct IntuiMessage *)GetMsg(displayWin->UserPort)))
                {
                    switch(msg->Class)
                    {
                        case IDCMP_CLOSEWINDOW:
                                D(bug("[SMP-Smallpt] %s: window closing pressed\n", __func__);)

                            windowClosing = TRUE;
                            break;

                        case IDCMP_REFRESHWINDOW:
                            D(bug("[SMP-Smallpt] %s: Displaying output BitMap (REFRESHWINDOW)\n", __func__);)
                            BeginRefresh(msg->IDCMPWindow);
                            BltBitMapRastPort (outputBMap, 0, 0,
                                msg->IDCMPWindow->RPort, msg->IDCMPWindow->BorderLeft, msg->IDCMPWindow->BorderTop,
                                width, height, 0xC0);
                            EndRefresh(msg->IDCMPWindow, TRUE);
                            break;
                    }
                    ReplyMsg((struct Message *)msg);
                }
            }
            if (signals & (1 << mainPort->mp_SigBit))
            {
                struct MyMessage *msg;

                while ((msg = (struct MyMessage *)GetMsg(mainPort)))
                {
                    if (msg->mm_Message.mn_Length == sizeof(struct MyMessage))
                    {
                        switch (msg->mm_Type)
                        {
                            case MSG_REDRAWTILE:
                                WritePixelArray(workBuffer,
                                            msg->mm_Body.RedrawTile.TileX * TILE_SIZE, 
                                            msg->mm_Body.RedrawTile.TileY * TILE_SIZE, width * sizeof(ULONG),
                                            outBMRastPort,
                                            msg->mm_Body.RedrawTile.TileX * TILE_SIZE,
                                            msg->mm_Body.RedrawTile.TileY * TILE_SIZE,
                                            TILE_SIZE, TILE_SIZE, RECTFMT_ARGB);

                                BltBitMapRastPort (outputBMap, 
                                            msg->mm_Body.RedrawTile.TileX * TILE_SIZE, 
                                            msg->mm_Body.RedrawTile.TileY * TILE_SIZE,
                                            displayWin->RPort, 
                                            displayWin->BorderLeft + msg->mm_Body.RedrawTile.TileX * TILE_SIZE, displayWin->BorderTop + msg->mm_Body.RedrawTile.TileY * TILE_SIZE,
                                            TILE_SIZE, TILE_SIZE, 0xC0); 
                                break;
                            
                            default:
                                D(bug("[SMP-Smallpt] %s: unhandled message (%d) arrived\n", __func__, msg->mm_Type);)
                        }
                        ReplyMsg(&msg->mm_Message);
                    }
                }
            }
        }

        D(bug("[SMP-Smallpt] %s: Send DIE msg to renderer\n", __func__);)

        struct MyMessage quitmsg;
        quitmsg.mm_Message.mn_ReplyPort = mainPort;
        quitmsg.mm_Message.mn_Length = sizeof(quitmsg);
        quitmsg.mm_Type = MSG_DIE;

        PutMsg(rendererPort, &quitmsg.mm_Message);

        Wait(SIGBREAKF_CTRL_C);
        
        DeleteMsgPort(mainPort);

        CloseWindow(displayWin);
        FreeRastPort(outBMRastPort);
        FreeBitMap(outputBMap);
        FreeMem(workBuffer, width * height * sizeof(ULONG));

D(bug("[SMP-Smallpt] %s: goodbye\n", __func__);)
#if 0
        for (int ty = 0; ty < height/32; ty++)
        {
            for (int tx = 0; tx < width/32; tx++)
            {
                D(bug("[SMP-Smallpt] %s: Rendering tile %d,%d\n", __func__, tx, ty));

                render_tile(width, height, 1024, tx, ty, workBuffer);

                D(bug("[SMP-Smallpt] %s: Redrawing\n", __func__));

                WritePixelArray(workBuffer,
                                            tx*32, ty*32, width * sizeof(ULONG),
                                            outBMRastPort,
                                            tx*32, ty*32,
                                            32, 32,
                                            RECTFMT_ARGB);

                BltBitMapRastPort (outputBMap, tx*32, ty*32,
                    displayWin->RPort, displayWin->BorderLeft + tx*32, displayWin->BorderTop + ty*32,
                    32, 32, 0xC0); 
            }
        }
#endif
    }

    return 0;
}