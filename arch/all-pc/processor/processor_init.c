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

void Processor_QueryTask(struct ExecBase *SysBase)
{
    struct X86ProcessorInformation *procInfo;
    struct Task *thisTask;

    D(
        bug("[processor.x86] %s()\n", __func__);
        bug("[processor.x86] %s: SysBase @ 0x%p\n", __func__, SysBase);
    )

    thisTask = FindTask(NULL);
    procInfo = thisTask->tc_UserData;

    if (procInfo)
        ReadProcessorInformation(procInfo);
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

    sysprocs = AllocVec(ProcessorBase->cpucount * sizeof(APTR), MEMF_ANY | MEMF_CLEAR);
    if (sysprocs == NULL)
        return FALSE;

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
                ml->ml_NumEntries      = 1;

                ml->ml_ME[0].me_Length = 22;
                if ((ml->ml_ME[0].me_Addr = AllocMem(22, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
                {
                    processorQueryTaskName = (char *)ml->ml_ME[0].me_Addr;

                    RawDoFmt("Processor #%03u Query", (RAWARG)cpuNameArg, RAWFMTFUNC_STRING, processorQueryTaskName);

                    taskAffinity = KrnAllocCPUMask();
                    KrnGetCPUMask(cpuNo, taskAffinity);

                    processorQueryTask = NewCreateTask(TASKTAG_NAME   , processorQueryTaskName,
                                            TASKTAG_AFFINITY   , taskAffinity,
                                            TASKTAG_PRI        , 100,
                                            TASKTAG_PC         , Processor_QueryTask,
                                            TASKTAG_ARG1       , SysBase,
                                            TASKTAG_USERDATA, sysprocs[cpuNo],
                                            TAG_DONE);

                    if (processorQueryTask)
                    {
                        //AddTail(&processorQueryTask->tc_MemEntry, &ml->ml_Node);
                        continue;
                    }
                    FreeMem(ml->ml_ME[0].me_Addr, 22);
                    FreeMem(ml, sizeof(struct MemList));
                }
                else
                {
                    bug("[processor.x86] FATAL : Failed to allocate memory for processor query name");
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

    ProcessorBase->Private1 = sysprocs;

    return retval;
}

ADD2INITLIB(Processor_Init, 1);
