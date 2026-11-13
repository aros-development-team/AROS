/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A read spinlock taken in an interrupt handler while tasks take and
    release it in read mode. Exits with 0 after the given number of seconds
    (default 10); a kernel where the handler can spin on a reader it
    interrupted stops instead.
*/

#include <aros/asmcall.h>
#include <aros/types/spinlock_s.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>
#include <dos/dos.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <stdio.h>
#include <stdlib.h>

#define SPINNERS 8

APTR KernelBase;
static spinlock_t lock;
static volatile ULONG interrupts;
static volatile ULONG stop;
static volatile ULONG running;

static AROS_INTH1(handler, APTR, data)
{
    AROS_INTFUNC_INIT

    (void)data;
    KrnSpinLock(&lock, NULL, SPINLOCK_MODE_READ);
    interrupts++;
    KrnSpinUnLock(&lock);
    return 0;

    AROS_INTFUNC_EXIT
}

static struct Interrupt vblank =
{
    .is_Code = (APTR)handler
};

static void spinner(void)
{
    while (!stop)
    {
        KrnSpinLock(&lock, NULL, SPINLOCK_MODE_READ);
        KrnSpinUnLock(&lock);
    }
    Forbid();
    running--;
}

int main(int argc, char **argv)
{
    int seconds = argc > 1 ? atoi(argv[1]) : 10;
    int i;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return RETURN_FAIL;

    KrnSpinInit(&lock);

    for (i = 0; i < SPINNERS; i++)
    {
        Forbid();
        if (NewCreateTask(TASKTAG_PC, spinner, TASKTAG_NAME, "spinlock_irq", TASKTAG_PRI, -1, TAG_DONE))
            running++;
        Permit();
    }

    AddIntServer(INTB_VERTB, &vblank);
    Delay(seconds * 50);
    RemIntServer(INTB_VERTB, &vblank);

    stop = 1;
    while (running)
        Delay(1);

    printf("%u interrupts, %d tasks: %s\n", (unsigned)interrupts, i, lock.lock ? "lock left held" : "OK");
    return (interrupts && !lock.lock) ? RETURN_OK : RETURN_FAIL;
}
