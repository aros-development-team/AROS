/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/config.h>

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/symbolsets.h>

#include <exec/tasks.h>
#include <exec/rawfmt.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

/* Private Data passed to each Processors Query Task */
struct PQData
{
    struct ProcessorBase *ProcessorBase;
    struct X86ProcessorInformation *pqd_ProcInfo;
};

static void Processor_QueryTask(struct ExecBase *SysBase)
{
    struct ProcessorBase *ProcessorBase;
    struct PQData *pqData;
    struct Task *thisTask;

    D(
        bug("[processor.x86] %s()\n", __func__);
        bug("[processor.x86] %s: SysBase @ 0x%p\n", __func__, SysBase);
    )

    thisTask = FindTask(NULL);
    pqData = thisTask->tc_UserData;
    ProcessorBase = pqData->ProcessorBase;

    if (pqData->pqd_ProcInfo)
        ReadProcessorInformation(ProcessorBase, pqData->pqd_ProcInfo);
    else
        bug("[processor.x86] %s: ERROR: procinfo missing!\n", __func__)

    D(bug("[processor.x86] %s: Finished!\n", __func__));
}

LONG Processor_Init(struct ProcessorBase * ProcessorBase)
{
    struct X86ProcessorInformation **sysprocs;
    struct Task *processorQueryTask;
    char *processorQueryTaskName;
    IPTR cpuNameArg[] = { 0 };
    struct MemList *ml;
    unsigned int cpuNo;
    BOOL retval = TRUE;
    void *taskAffinity;

    D(
        bug("[processor.x86] %s()\n", __func__);
        bug("[processor.x86] %s: SysBase @ 0x%p, ProcessorBase @ 0x%p\n", __func__, SysBase, ProcessorBase);
    )

    /*
     * Allocate cpu count + 1 slots
     * and embed UtilityBase in the last.
    */ 
    sysprocs = AllocVec((ProcessorBase->cpucount + 1) * sizeof(APTR), MEMF_ANY | MEMF_CLEAR);
    if (sysprocs == NULL)
        return FALSE;

    sysprocs[ProcessorBase->cpucount] =  TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!sysprocs[ProcessorBase->cpucount])
    {
        FreeVec(sysprocs);
        return FALSE;
    }

    ProcessorBase->Private1 = sysprocs;

    for (cpuNo = 0; cpuNo < ProcessorBase->cpucount; cpuNo++)
    {
    	sysprocs[cpuNo] = AllocMem(sizeof(struct X86ProcessorInformation), MEMF_CLEAR);
    	if (sysprocs[cpuNo])
        {
#if !defined(__AROSEXEC_SMP__)
            if (cpuNo > 0)
                continue;
#endif
            cpuNameArg[0] = (IPTR)cpuNo + 1;

            if ((ml = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
            {
                ml->ml_NumEntries      = 2;

                ml->ml_ME[1].me_Length = sizeof(struct PQData);
                if ((ml->ml_ME[1].me_Addr = AllocMem(ml->ml_ME[1].me_Length, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
                {
                    ml->ml_ME[0].me_Length = 22;
                    if ((ml->ml_ME[0].me_Addr = AllocMem(ml->ml_ME[0].me_Length, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
                    {
                        struct PQData *pqData = (struct PQData *)ml->ml_ME[1].me_Addr;
                        pqData->pqd_ProcInfo = sysprocs[cpuNo];
                        pqData->ProcessorBase = ProcessorBase;
                        processorQueryTaskName = (char *)ml->ml_ME[0].me_Addr;

                        RawDoFmt("Processor #%03u Query", (RAWARG)cpuNameArg, RAWFMTFUNC_STRING, processorQueryTaskName);

                        taskAffinity = KrnAllocCPUMask();
                        KrnGetCPUMask(cpuNo, taskAffinity);

                        processorQueryTask = NewCreateTask(TASKTAG_NAME   , processorQueryTaskName,
                                                TASKTAG_AFFINITY   , taskAffinity,
                                                TASKTAG_PRI        , 100,
                                                TASKTAG_PC         , Processor_QueryTask,
                                                TASKTAG_ARG1       , SysBase,
                                                TASKTAG_USERDATA, pqData,
                                                TAG_DONE);

                        if (processorQueryTask)
                        {
                            //AddTail(&processorQueryTask->tc_MemEntry, &ml->ml_Node);
                            continue;
                        }
                        FreeEntry(ml);
                    }
                    else
                    {
                        bug("[processor.x86] FATAL : Failed to allocate memory for ProcQuery Task name");
                        FreeMem(ml->ml_ME[1].me_Addr, ml->ml_ME[1].me_Length);
                        FreeMem(ml, sizeof(struct MemList));
                    }
                }
                else
                {
                    bug("[processor.x86] FATAL : Failed to allocate memory for ProcQuery Task params");
                    FreeMem(ml, sizeof(struct MemList));
                }
            }
            else
            {
                bug("[processor.x86] FATAL : Failed to allocate additional memory for processor query task");
            }
        }
        retval = FALSE;
    }

    return retval;
}

ADD2INITLIB(Processor_Init, 1);
