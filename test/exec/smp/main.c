/*
    Copyright 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/processor.h>
#include <proto/kernel.h>

#include <proto/intuition.h>

#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <exec/rawfmt.h>
#include <resources/processor.h>

#include "work.h"

CONST_STRPTR version = "$VER: SMP-Test 1.0 (20.02.2017) ©2017 The AROS Development Team";

APTR KernelBase;

int main()
{
    struct SMPMaster workMaster;

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
        int count = 0;
        BOOL complete = FALSE;

        /* Create a port that workers/masters will signal us using .. */
        if ((workMaster.smpm_MasterPort = CreateMsgPort()) == NULL)
            return 0;

        NEWLIST(&workMaster.smpm_Workers);

        D(bug("[SMP-Test] %s: work Master MsgPort @ 0x%p\n", __func__, workMaster.smpm_MasterPort);)
        D(bug("[SMP-Test] %s: SigTask = 0x%p\n", __func__, workMaster.smpm_MasterPort->mp_SigTask);)

        for (core = 0; core < coreCount; core++)
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
                        D(bug("[SMP-Test] %s: Worker Task Name '%s'\n", __func__, coreML->ml_ME[1].me_Addr);)

                        coreWorker->smpw_MasterPort = workMaster.smpm_MasterPort;
                        coreWorker->smpw_Node.ln_Type = 0;
                        coreWorker->smpw_SyncTask = FindTask(NULL);
                        coreWorker->smpw_Task = NewCreateTask(TASKTAG_NAME   , coreML->ml_ME[1].me_Addr,
                                                    TASKTAG_AFFINITY   , coreAffinity,
                                                    TASKTAG_PRI        , 10,
                                                    TASKTAG_PC         , SMPTestWorker,
                                                    TASKTAG_ARG1       , SysBase,
                                                    TASKTAG_USERDATA   , coreWorker,
                                                    TAG_DONE);

                        if (coreWorker->smpw_Task)
                        {
                            count++;
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

        rawArgs[0] = count;
        rawArgs[1] = coreCount;
        RawDoFmt("SMP Test Output (%d workers on %d cores)", (RAWARG)rawArgs, RAWFMTFUNC_STRING, buffer);

        complete = FALSE;
        pubScreen = LockPubScreen(0);
        if ((displayWin = OpenWindowTags(0,
                                     WA_PubScreen, (IPTR)pubScreen,
                                     WA_Left, 0,
                                     WA_Top, (pubScreen) ? pubScreen->BarHeight : 10,
                                     WA_Width, (pubScreen) ? pubScreen->Width : 320,
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
            BOOL working = TRUE;

            if (pubScreen)
                UnlockPubScreen(0, pubScreen);

            workMaster.smpm_Master = NewCreateTask(TASKTAG_NAME   , "SMP-Test Master",
                                                        TASKTAG_AFFINITY, TASKAFFINITY_ANY,
                                                        TASKTAG_PRI        , 0,
                                                        TASKTAG_PC         , SMPTestMaster,
                                                        TASKTAG_ARG1       , SysBase,
                                                        TASKTAG_USERDATA   , &workMaster,
                                                        TAG_DONE);

            D(bug("[SMP-Test] %s: Waiting for the work to be done ...\n", __func__);)

            /* Wait for the workers to finish processing the data ... */
            
            while ((working) && ((signals = Wait(SIGBREAKF_CTRL_D | displayWin->UserPort->mp_SigBit)) != 0))
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

                    if (complete)
                    {
                        working = FALSE;
                        break;
                    }
                }
                else if (signals & displayWin->UserPort->mp_SigBit)
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
#if (0)
                                BeginRefresh(win);
                                EndRefresh(win,TRUE);
#endif
                                break;
                        }
                        ReplyMsg((struct Message *)msg);
                    }
                }

                /* update the output... */
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
            CloseWindow(displayWin);
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
