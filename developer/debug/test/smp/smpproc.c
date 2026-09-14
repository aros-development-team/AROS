/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for NP_Affinity. A process created with an affinity
          mask must actually start on a CPU from that mask - without the
          tag a new process silently inherits the CPU of whoever created
          it, which pins every command a shell starts to the shell's core.
*/

#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
static int g_NumCPUs = 1;

#define STACKSIZE   16384
#define WAIT_TICKS  250         /* 5s */

struct ProcResult
{
    volatile ULONG  pr_Wanted;      /* CPU we asked for                 */
    volatile ULONG  pr_Ran;         /* CPU it actually started on       */
    volatile ULONG  pr_Done;
    struct Task    *pr_Parent;
    ULONG           pr_Signal;
};

static void AffinityChild(void)
{
    struct ProcResult *r = (struct ProcResult *)FindTask(NULL)->tc_UserData;

    r->pr_Ran = (ULONG)KrnGetCPUNumber();
    r->pr_Done = 1;
    Signal(r->pr_Parent, 1UL << r->pr_Signal);
}

int main(void)
{
    struct ProcResult res[4];
    struct Process *proc[4];
    char names[4][32];
    BYTE sigbit;
    int i, n, started = 0, pass = 0, fail = 0;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();
    n = g_NumCPUs < 4 ? g_NumCPUs : 4;

    bug("[smpproc] start: cpus=%d\n", g_NumCPUs);

    if (!KernelBase || n < 2)
    {
        bug("[smpproc] INVALID (needs at least 2 CPUs)\n");
        return RETURN_WARN;
    }

    sigbit = AllocSignal(-1);
    if (sigbit == -1)
    {
        bug("[smpproc] INVALID (no signal bit)\n");
        return RETURN_FAIL;
    }

    memset(res, 0, sizeof(res));

    for (i = 0; i < n; i++)
    {
        cpumask_t *mask = KrnAllocCPUMask();

        if (!mask)
        {
            bug("[smpproc] INVALID (KrnAllocCPUMask failed)\n");
            break;
        }
        KrnClearCPUMask(mask);
        KrnGetCPUMask(i, mask);

        snprintf(names[i], sizeof(names[i]), "smpproc.%d", i);
        res[i].pr_Wanted = (ULONG)i;
        res[i].pr_Ran = (ULONG)~0;
        res[i].pr_Parent = FindTask(NULL);
        res[i].pr_Signal = (ULONG)sigbit;

        proc[i] = CreateNewProcTags(NP_Entry,     (IPTR)AffinityChild,
                                    NP_Name,      (IPTR)names[i],
                                    NP_StackSize, STACKSIZE,
                                    NP_UserData,  (IPTR)&res[i],
                                    NP_Affinity,  (IPTR)mask,
                                    TAG_DONE);
        if (!proc[i])
        {
            bug("[smpproc] INVALID (CreateNewProc failed for cpu %d)\n", i);
            KrnFreeCPUMask(mask);
            break;
        }
        started++;
    }

    /* The children only touch res[] and signal us. */
    for (i = 0; i < WAIT_TICKS; i++)
    {
        int done = 0;
        int k;

        for (k = 0; k < started; k++)
            if (res[k].pr_Done)
                done++;
        if (done == started)
            break;
        Delay(1);
    }

    for (i = 0; i < started; i++)
    {
        if (!res[i].pr_Done)
        {
            bug("[smpproc] *** FAIL *** process %d never ran\n", i);
            fail++;
        }
        else if (res[i].pr_Ran != res[i].pr_Wanted)
        {
            bug("[smpproc] *** FAIL *** process %d asked for cpu %lu, ran on cpu %lu\n",
                i, (unsigned long)res[i].pr_Wanted, (unsigned long)res[i].pr_Ran);
            fail++;
        }
        else
        {
            bug("[smpproc] process %d: asked cpu %lu, ran cpu %lu OK\n",
                i, (unsigned long)res[i].pr_Wanted, (unsigned long)res[i].pr_Ran);
            pass++;
        }
    }

    FreeSignal(sigbit);

    bug("[smpproc] DONE: %d PASS, %d FAIL, 0 INVALID\n", pass, fail);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
