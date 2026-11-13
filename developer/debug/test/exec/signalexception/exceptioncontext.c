/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Asynchronous signal exceptions must preserve ordinary C comparisons.
*/
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <dos/dostags.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>

static struct Task *parent;
static ULONG exception_mask, done_mask;
static volatile ULONG calls;
static volatile ULONG zero;

AROS_UFH3(ULONG, handler,
    AROS_UFHA(APTR, data, A1),
    AROS_UFHA(ULONG, flags, D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    (void)data;
    calls++;
    return flags;
    AROS_USERFUNC_EXIT
}

static void sender(void)
{
    ULONG i;
    for (i = 0; i < 100; i++)
    {
        Delay(1);
        Signal(parent, exception_mask);
    }
    Signal(parent, done_mask);
}

int main(void)
{
    struct Process *child;
    BYTE exception_bit = AllocSignal(-1), done_bit = AllocSignal(-1);
    ULONG oldmask, failures = 0, comparisons = 0;
    APTR oldcode;

    if (exception_bit < 0 || done_bit < 0)
        return 20;
    parent = FindTask(NULL);
    exception_mask = 1UL << exception_bit;
    done_mask = 1UL << done_bit;
    oldcode = parent->tc_ExceptCode;
    parent->tc_ExceptCode = (APTR)handler;
    oldmask = SetExcept(exception_mask, exception_mask);
    child = CreateNewProcTags(NP_Entry, (IPTR)sender,
        NP_Name, (IPTR)"Exception context sender", NP_Priority, 1,
        TAG_DONE);
    if (!child)
        failures++;
    else
    {
        /* zero is never modified. Its comparison must remain true even
         * when an exception interrupts between the compare and branch. */
        while (calls < 100 && comparisons < 50000000)
        {
            if (zero != 0)
                failures++;
            comparisons++;
        }
        Wait(done_mask);
        if (calls != 100)
            failures++;
    }
    SetExcept(oldmask, exception_mask);
    parent->tc_ExceptCode = oldcode;
    SetSignal(0, exception_mask | done_mask);
    /* The child has signalled completion; let its process exit finish
     * before this program's code and data are unloaded. */
    if (child)
        Delay(5);
    FreeSignal(exception_bit);
    FreeSignal(done_bit);
    bug("EXCEPTION CONTEXT TEST: %lu failures, %lu callbacks, %lu comparisons\n",
        (unsigned long)failures, (unsigned long)calls,
        (unsigned long)comparisons);
    return failures ? 20 : 0;
}
