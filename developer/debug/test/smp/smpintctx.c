/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Interrupt context verification.

          Runs an INTB_VERTB server and checks from inside it that exec
          leaves the interrupt mask alone:
            - KrnIsSuper() reports supervisor mode,
            - the server is entered with interrupts masked,
            - a Disable()/Enable() pair does not unmask them,
            - Signal() does not unmask them (on SMP it does its own
              Disable()/Enable()).

          Where one of these fails a handler can be interrupted by
          itself, and a spinlock it holds deadlocks the CPU.

          From the task it then checks SuperState()/UserState():
            - SuperState() in supervisor mode returns NULL,
            - UserState() returns to user mode,
            - UserState() sets the interrupt mask from the Disable()
              nesting, not unconditionally.
*/

#include <aros/config.h>
#include <aros/asmcall.h>

#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;

#define RUN_TICKS   100             /* sampling window (~2s at 50Hz) */

/* Interrupt mask access. Only used inside the handler. */
#if defined(__aarch64__)
#define IRQMASK_ALL 0xc0            /* DAIF: I and F */

static inline IPTR irqmask_get(void)
{
    IPTR daif;

    asm volatile("mrs %0, daif" : "=r"(daif));
    return daif;
}

static inline void irqmask_set(IPTR daif)
{
    asm volatile("msr daif, %0" :: "r"(daif));
}
#else
/* No reader for this CPU: only KrnIsSuper() is checked */
#define IRQMASK_ALL 0

static inline IPTR irqmask_get(void)
{
    return 0;
}

static inline void irqmask_set(IPTR mask)
{
}
#endif

#define UNMASKED(m) (((m) & IRQMASK_ALL) != IRQMASK_ALL)

struct IntCtx
{
    struct Task    *ic_Task;
    ULONG           ic_SigMask;
    volatile ULONG  ic_Samples;
    volatile ULONG  ic_NotSuper;    /* KrnIsSuper() said user mode */
    volatile ULONG  ic_Entered;     /* entered with interrupts unmasked */
    volatile ULONG  ic_Enable;      /* unmasked by Disable()/Enable() */
    volatile ULONG  ic_Signal;      /* unmasked by Signal() */
};

static AROS_INTH1(IntCtxServer, struct IntCtx *, ic)
{
    AROS_INTFUNC_INIT

    IPTR entry, masked;

    if (!KrnIsSuper())
        ic->ic_NotSuper++;

    entry = irqmask_get();
    if (UNMASKED(entry))
        ic->ic_Entered++;

    /* Each check starts from a masked state, so one failure can not hide the next */
    masked = entry | IRQMASK_ALL;
    irqmask_set(masked);

    Disable();
    Enable();
    if (UNMASKED(irqmask_get()))
        ic->ic_Enable++;
    irqmask_set(masked);

    Signal(ic->ic_Task, ic->ic_SigMask);
    if (UNMASKED(irqmask_get()))
        ic->ic_Signal++;

    /* Leave as found */
    irqmask_set(entry);

    ic->ic_Samples++;

    return 0;

    AROS_INTFUNC_EXIT
}

/* Returns a bit per failed check. No output while in supervisor mode. */
static int CheckSuperState(void)
{
    APTR ssp;
    int fail = 0;

    ssp = SuperState();
    if (!KrnIsSuper())
        fail |= 1;
    if (SuperState() != NULL)
        fail |= 2;
    UserState(ssp);
    if (KrnIsSuper())
        fail |= 4;

    /* Entered under Disable(): must come back masked */
    Disable();
    ssp = SuperState();
    UserState(ssp);
    if (UNMASKED(irqmask_get()))
        fail |= 8;
    Enable();

    /* Disable()/Enable() inside: must come back unmasked */
    ssp = SuperState();
    Disable();
    Enable();
    UserState(ssp);
    if (irqmask_get() & IRQMASK_ALL)
        fail |= 16;

    return fail;
}

int main(void)
{
    struct IntCtx ic = { 0 };
    struct Interrupt server = { { 0 } };
    BYTE sigbit;
    int fail, super;

    KernelBase = OpenResource("kernel.resource");
    sigbit = AllocSignal(-1);
    if (!KernelBase || sigbit == -1)
    {
        bug("[smpintctx] INVALID: no kernel.resource or no free signal\n");
        return RETURN_FAIL;
    }

    ic.ic_Task    = FindTask(NULL);
    ic.ic_SigMask = 1UL << sigbit;

    server.is_Node.ln_Type = NT_INTERRUPT;
    server.is_Node.ln_Name = "smpintctx";
    server.is_Code         = (VOID_FUNC)IntCtxServer;
    server.is_Data         = &ic;

    bug("[smpintctx] start: sampling a VBlank server for %d ticks\n", RUN_TICKS);
    if (!IRQMASK_ALL)
        bug("[smpintctx] no interrupt mask reader for this CPU - only KrnIsSuper() is checked\n");

    AddIntServer(INTB_VERTB, &server);
    Delay(RUN_TICKS);
    RemIntServer(INTB_VERTB, &server);
    /* The chain is walked unlocked: let a pass in flight on another CPU finish */
    Delay(1);
    FreeSignal(sigbit);

    bug("[smpintctx] %lu samples: KrnIsSuper()==0 %lu, entered unmasked %lu, "
        "unmasked by Disable()/Enable() %lu, by Signal() %lu\n",
        (unsigned long)ic.ic_Samples, (unsigned long)ic.ic_NotSuper,
        (unsigned long)ic.ic_Entered, (unsigned long)ic.ic_Enable,
        (unsigned long)ic.ic_Signal);

    if (ic.ic_Samples == 0)
    {
        bug("[smpintctx] INVALID: the VBlank server never ran\n");
        return RETURN_FAIL;
    }

    super = CheckSuperState();
    bug("[smpintctx] SuperState(): not supervisor %d, nested not NULL %d, "
        "UserState(): still supervisor %d, unmasked under Disable() %d, left masked %d\n",
        !!(super & 1), !!(super & 2), !!(super & 4), !!(super & 8), !!(super & 16));

    fail = ic.ic_NotSuper || ic.ic_Entered || ic.ic_Enable || ic.ic_Signal || super;
    bug("[smpintctx] DONE: %s\n", fail ? "FAIL" : "PASS");

    return fail ? RETURN_FAIL : RETURN_OK;
}
