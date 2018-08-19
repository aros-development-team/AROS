/*
    Copyright 2017-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
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

#include "work.h"

CONST_STRPTR version = "$VER: SMP-Test 1.0 (03.03.2017) ©2017 The AROS Development Team";

//#define SMPTEST_DIEWHENFINISHED

APTR KernelBase;

#define ARG_TEMPLATE "MAXCPU/N,MAXITER/N,OVERSAMPLE/N,BUDDHA/S"
#define ARG_MAXCPU 0
#define ARG_MAXITER 1
#define ARG_OVERSAMPLE 2
#define ARG_BUDDHA 3

int main()
{
    struct SMPMaster workMaster;
    IPTR args[4] = { 0, 0, 0, 0 };
    int max_cpus = 0;

    APTR ProcessorBase;
    IPTR coreCount = 1, core;
    struct TagItem tags [] = 
    {
        { GCIT_NumberOfProcessors,      (IPTR)&coreCount },
        { 0,                            (IPTR)NULL }
    };
    ULONG signals;
    struct Screen *pubScreen;
    struct Window *displayWin;
    struct BitMap *outputBMap = NULL;

    ProcessorBase = OpenResource(PROCESSORNAME);
    if (!ProcessorBase)
        return 0;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return 0;

    GetCPUInfo(tags);

    if (coreCount > 1)
    {
        struct MemList *coreML;
        struct SMPWorker *coreWorker;
        struct SMPWorkMessage *workMsg;
        IPTR rawArgs[2];
        char buffer[100];
        BOOL complete = FALSE, buddha = FALSE;
        ULONG maxWork = 0;
        ULONG oversample = 0;
        struct RDArgs *rda;

        max_cpus = coreCount;

        rda = ReadArgs(ARG_TEMPLATE, args, NULL);
        if (rda != NULL)
        {
            LONG *ptr = (LONG *)args[ARG_MAXCPU];
            if (ptr)
                max_cpus = *ptr;

            ptr = (LONG *)args[ARG_MAXITER];
            if (ptr)
            {
                maxWork = *ptr;
                if (maxWork < 2)
                    maxWork = 2;
                else if (maxWork > 10000000)
                    maxWork = 10000000;
            }

            ptr = (LONG *)args[ARG_OVERSAMPLE];
            if (ptr)
            {
                oversample = *ptr;
                if (oversample < 1)
                    oversample = 1;
                else if (oversample > 10)
                    oversample = 10;
            }

            if (args[ARG_BUDDHA])
                buddha = TRUE;

            if (max_cpus > coreCount)
                max_cpus = coreCount;
        }

        /* Create a port that workers/masters will signal us using .. */
        if ((workMaster.smpm_MasterPort = CreateMsgPort()) == NULL)
            return 0;

        NEWLIST(&workMaster.smpm_Workers);

        D(bug("[SMP-Test] %s: work Master MsgPort @ 0x%p\n", __func__, workMaster.smpm_MasterPort);)
        D(bug("[SMP-Test] %s: SigTask = 0x%p\n", __func__, workMaster.smpm_MasterPort->mp_SigTask);)

        workMaster.smpm_WorkerCount = 0;
        if (buddha)
        {
            workMaster.smpm_MaxWork = 4000;
            workMaster.smpm_Oversample = 4;
            workMaster.smpm_Buddha = TRUE;
        }
        else
        {
            workMaster.smpm_MaxWork = 1000;
            workMaster.smpm_Oversample = 1;
            workMaster.smpm_Buddha = FALSE;
        }
        if (maxWork)
            workMaster.smpm_MaxWork = maxWork;
        if (oversample)
            workMaster.smpm_Oversample = oversample;

        KrnSpinInit(&workMaster.smpm_Lock);

        for (core = 0; core < max_cpus; core++)
        {
            void *coreAffinity = KrnAllocCPUMask();
            KrnGetCPUMask(core, coreAffinity);

            D(bug("[SMP-Test] %s: CPU #%03u affinity = %08X\n", __func__, core, coreAffinity);)

            if ((coreML = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
            {
                coreML->ml_ME[0].me_Length = sizeof(struct SMPWorker);
                if ((coreML->ml_ME[0].me_Addr = AllocMem(sizeof(struct SMPWorker), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
                {
                    coreWorker = coreML->ml_ME[0].me_Addr;
                    D(bug("[SMP-Test] %s: CPU Worker Node allocated @ 0x%p\n", __func__, coreWorker);)

                    coreML->ml_ME[1].me_Length = 22;
                    if ((coreML->ml_ME[1].me_Addr = AllocMem(22, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
                    {
                        coreML->ml_NumEntries = 2;

                        rawArgs[0] = core;
                        RawDoFmt("SMP-Test Worker.#%03u", (RAWARG)rawArgs, RAWFMTFUNC_STRING, coreML->ml_ME[1].me_Addr);
                        D(
                            bug("[SMP-Test] %s: Worker Task Name '%s'\n", __func__, coreML->ml_ME[1].me_Addr);
                            bug("[SMP-Test] %s: Worker Stack Size = %d bytes\n", __func__, ((workMaster.smpm_MaxWork / 50000) + 1) * AROS_STACKSIZE);
                        )

                        coreWorker->smpw_MasterPort = workMaster.smpm_MasterPort;
                        coreWorker->smpw_Node.ln_Type = 0;
                        coreWorker->smpw_SyncTask = FindTask(NULL);
                        coreWorker->smpw_Lock = &workMaster.smpm_Lock;
                        coreWorker->smpw_MaxWork = workMaster.smpm_MaxWork;
                        coreWorker->smpw_Oversample = workMaster.smpm_Oversample;
                        coreWorker->smpw_Task = NewCreateTask(TASKTAG_NAME   , coreML->ml_ME[1].me_Addr,
                                                    TASKTAG_AFFINITY   , coreAffinity,
                                                    TASKTAG_PRI        , 0,
                                                    TASKTAG_PC         , SMPTestWorker,
                                                    TASKTAG_ARG1       , SysBase,
                                                    TASKTAG_STACKSIZE,      ((workMaster.smpm_MaxWork / 50000) + 1) * AROS_STACKSIZE,
                                                    TASKTAG_USERDATA   , coreWorker,
                                                    TAG_DONE);

                        if (coreWorker->smpw_Task)
                        {
                            workMaster.smpm_WorkerCount++;
                            Wait(SIGBREAKF_CTRL_C);
                            AddTail(&workMaster.smpm_Workers, &coreWorker->smpw_Node);
                            AddHead(&coreWorker->smpw_Task->tc_MemEntry, &coreML->ml_Node);
                        }
                    }
                    else
                    {
                        FreeMem(coreML->ml_ME[0].me_Addr, sizeof(struct SMPWorker));
                        FreeMem(coreML, sizeof(struct MemList) + sizeof(struct MemEntry));
                    }
                }
                else
                {
                    FreeMem(coreML, sizeof(struct MemList) + sizeof(struct MemEntry));
                }
            }
        }

        D(bug("[SMP-Test] %s: Waiting for workers to become ready ...\n", __func__);)

        do {
            complete = TRUE;
            ForeachNode(&workMaster.smpm_Workers, coreWorker)
            {
                if (coreWorker->smpw_Node.ln_Type != 1)
                {
                    complete = FALSE;
                    Reschedule();
                    break;
                }
            }
        } while (!complete);

        /*
         * We now have our workers all launched,
         * and a node for each on our list.
         * lets get them to do some work ...
         */
        D(bug("[SMP-Test] %s: Sending out work to do ...\n", __func__);)

        rawArgs[0] = workMaster.smpm_WorkerCount;
        rawArgs[1] = coreCount;
        RawDoFmt("SMP Test Output (%id workers on %id cores)", (RAWARG)rawArgs, RAWFMTFUNC_STRING, buffer);

        complete = FALSE;
        pubScreen = LockPubScreen(0);
        if ((displayWin = OpenWindowTags(0,
                                     WA_PubScreen, (IPTR)pubScreen,
                                     WA_Left, 0,
                                     WA_Top, (pubScreen) ? pubScreen->BarHeight : 10,
                                     WA_Width, (pubScreen) ? (pubScreen->Height - pubScreen->BarHeight ) : 320,
                                     WA_Height, (pubScreen) ? (pubScreen->Height - pubScreen->BarHeight ) : 200,
                                     WA_Title, (IPTR)buffer,
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
            struct RastPort *outBMRastPort;
            BOOL working = TRUE;
            UWORD width, height;

            SetWindowPointer( displayWin, WA_BusyPointer, TRUE, TAG_DONE );

            if (pubScreen)
                UnlockPubScreen(0, pubScreen);

            width = workMaster.smpm_Width  = (displayWin->Width - displayWin->BorderLeft - displayWin->BorderRight);
            height = workMaster.smpm_Height = (displayWin->Height - displayWin->BorderTop - displayWin->BorderBottom);

            outputBMap = AllocBitMap(
                                        workMaster.smpm_Width,
                                        workMaster.smpm_Height,
                                        GetBitMapAttr(displayWin->WScreen->RastPort.BitMap, BMA_DEPTH),
                                        BMF_DISPLAYABLE, displayWin->WScreen->RastPort.BitMap);

            workMaster.smpm_WorkBuffer = AllocMem(workMaster.smpm_Width * workMaster.smpm_Height * sizeof(ULONG), MEMF_ANY|MEMF_CLEAR);

            D(
                bug("[SMP-Test] %s: Target BitMap @ 0x%p\n", __func__, outputBMap);
                bug("[SMP-Test] %s:     %dx%dx%d\n", __func__, workMaster.smpm_Width, workMaster.smpm_Height, GetBitMapAttr(outputBMap, BMA_DEPTH));
                bug("[SMP-Test] %s: Buffer @ 0x%p\n", __func__, workMaster.smpm_WorkBuffer);
            )

            outBMRastPort = CreateRastPort();
            outBMRastPort->BitMap = outputBMap;

            D(bug("[SMP-Test] %s: Target BitMap RastPort @ 0x%p\n", __func__, outBMRastPort);)

            workMaster.smpm_Master = NewCreateTask(TASKTAG_NAME   , "SMP-Test Master",
                                                        TASKTAG_AFFINITY, TASKAFFINITY_ANY,
                                                        TASKTAG_PRI        , -1,
                                                        TASKTAG_PC         , SMPTestMaster,
                                                        TASKTAG_ARG1       , SysBase,
                                                        TASKTAG_USERDATA   , &workMaster,
                                                        TAG_DONE);

            D(bug("[SMP-Test] %s: Waiting for the work to be done ...\n", __func__);)

            /* Wait for the workers to finish processing the data ... */
            
            while ((working) && ((signals = Wait(SIGBREAKF_CTRL_D | (1 << displayWin->UserPort->mp_SigBit))) != 0))
            {
                if ((signals & SIGBREAKF_CTRL_D) && (!complete))
                {
                    complete = TRUE;

                    /* Is work still being issued? */
                    if (workMaster.smpm_Master)
                        complete = FALSE;

                    /* Are workers still working ? */
                    ForeachNode(&workMaster.smpm_Workers, coreWorker)
                    {
                        if (!IsListEmpty(&coreWorker->smpw_MsgPort->mp_MsgList))
                            complete = FALSE;
                    }

                    D(bug("[SMP-Test] %s: Updating work output ...\n", __func__);)
                    WritePixelArray(workMaster.smpm_WorkBuffer,
                                            0, 0, workMaster.smpm_Width * sizeof(ULONG),
                                            outBMRastPort,
                                            0, 0,
                                            workMaster.smpm_Width, workMaster.smpm_Height,
                                            RECTFMT_ARGB);

                    if (complete)
                    {
                        SetWindowPointer( displayWin, WA_BusyPointer, FALSE, TAG_DONE );

                        rawArgs[0] = coreCount;
                        RawDoFmt("SMP Test Output (0 workers on %id cores) - Finished", (RAWARG)rawArgs, RAWFMTFUNC_STRING, buffer);
                        SetWindowTitles( displayWin, buffer, NULL);
#if defined(SMPTEST_DIEWHENFINISHED)
                        working = FALSE;
                        break;
#endif
                    }
                }
                else if (signals & (1 << displayWin->UserPort->mp_SigBit))
                {
                    struct IntuiMessage *msg;
                    while ((msg = (struct IntuiMessage *)GetMsg(displayWin->UserPort)))
                    {
                        switch(msg->Class)
                        {
                            case IDCMP_CLOSEWINDOW:
                                working = FALSE;
                                break;

                            case IDCMP_REFRESHWINDOW:
                                D(bug("[SMP-Test] %s: Displaying output BitMap (REFRESHWINDOW)\n", __func__);)
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
                D(bug("[SMP-Test] %s: Displaying output BitMap...\n", __func__);)
                BltBitMapRastPort (outputBMap, 0, 0,
                    displayWin->RPort, displayWin->BorderLeft, displayWin->BorderTop,
                    width, height, 0xC0); 
            }

            D(bug("[SMP-Test] %s: Letting workers know we are finished ...\n", __func__);)

            ForeachNode(&workMaster.smpm_Workers, coreWorker)
            {
                /* Tell the workers they are finished ... */
                if ((workMsg = (struct SMPWorkMessage *)AllocMem(sizeof(struct SMPWorkMessage), MEMF_CLEAR)) != NULL)
                {
                    workMsg->smpwm_Type = SPMWORKTYPE_FINISHED;
                    PutMsg(coreWorker->smpw_MsgPort, (struct Message *)workMsg);
                }
            }
            FreeMem(workMaster.smpm_WorkBuffer, workMaster.smpm_Width * workMaster.smpm_Height * sizeof(ULONG));
            CloseWindow(displayWin);
            outBMRastPort->BitMap = NULL;
            FreeRastPort(outBMRastPort);
            FreeBitMap(outputBMap);
        }
        if (pubScreen) UnlockPubScreen(0, pubScreen);
    }

    D(bug("[SMP-Test] %s: Waiting for workers to finish ...\n", __func__);)

    /* wait for the workers to finish up before we exit ... */
    if (workMaster.smpm_MasterPort)
    {
        while ((signals = Wait(SIGBREAKF_CTRL_C)) != 0)
        {
            if ((signals & SIGBREAKF_CTRL_C) && IsListEmpty(&workMaster.smpm_Workers))
                break;
        }
        DeleteMsgPort(workMaster.smpm_MasterPort);
    }

    D(bug("[SMP-Test] %s: Done ...\n", __func__);)

    return 0;
}
