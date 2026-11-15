/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dostags.h>

#include <errno.h>
#include <stdlib.h>

#include "test.h"

static struct Task *parent;
static BYTE sigbit = -1;

static volatile int child_first = -1;
static volatile int child_last = -1;
static volatile int child_overlaps = -1;

void cleanup(void)
{
    if (sigbit != -1)
    {
        FreeSignal(sigbit);
        sigbit = -1;
    }
}

/*
 * Exec frees every entry of every MemList in tc_MemEntry and then the
 * MemList itself when the task is gone. An entry that lies inside its own
 * MemList is therefore freed twice. Such a list is counted and taken off
 * the task, so that a failing run leaks it and ends with a report.
 */
static int drop_overlapping_memlists(struct Task *task)
{
    struct MemList *ml, *next;
    int found = 0;

    Forbid();
    for (ml = (struct MemList *)task->tc_MemEntry.lh_Head;
         (next = (struct MemList *)ml->ml_Node.ln_Succ) != NULL;
         ml = next)
    {
        UBYTE *start = (UBYTE *)ml;
        UBYTE *end = (UBYTE *)&ml->ml_ME[ml->ml_NumEntries];
        ULONG i;

        for (i = 0; i < ml->ml_NumEntries; i++)
        {
            UBYTE *addr = ml->ml_ME[i].me_Addr;

            if (addr && addr < end && addr + ml->ml_ME[i].me_Length > start)
            {
                Remove(&ml->ml_Node);
                found++;
                break;
            }
        }
    }
    Permit();

    return found;
}

/* A task of the program that is not a pthread and opened no C library */
static void child(void)
{
    child_first = errno;
    strtol("99999999999999999999999999999999", NULL, 10);
    child_last = errno;

    child_overlaps = drop_overlapping_memlists(FindTask(NULL));

    Signal(parent, 1UL << sigbit);
}

int main(void)
{
    APTR mem;

    parent = FindTask(NULL);
    sigbit = AllocSignal(-1);
    TEST(sigbit != -1);

    errno = EDOM;

    TEST(CreateNewProcTags(
        NP_Entry, (IPTR)child,
        NP_Name, (IPTR)"errno_task child",
        TAG_DONE) != NULL);
    Wait(1UL << sigbit);

    /* Let the task end and exec release its memory */
    Delay(50);

    /* Each task has an errno of its own */
    TEST(child_first == 0);
    TEST(child_last == ERANGE);
    TEST(errno == EDOM);

    /* The errno of the task must be released once when the task ends */
    TEST(child_overlaps == 0);

    /* The system still allocates memory after the task was released */
    mem = AllocMem(4096, MEMF_ANY);
    TEST(mem != NULL);
    FreeMem(mem, 4096);

    cleanup();

    return OK;
}
