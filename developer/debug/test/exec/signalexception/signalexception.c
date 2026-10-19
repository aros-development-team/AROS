/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A dispatched signal exception must preserve the caller's interrupt nesting.
*/
#include <aros/debug.h>
#include <aros/asmcall.h>
#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#if defined(__AROSEXEC_SMP__)

/* SMP keeps the live Disable() count in private per-CPU scheduler state;
 * ExecBase has no IDNestCnt there, so this check cannot be made. */
int main(void)
{
    bug("SIGNAL EXCEPTION TEST: skipped (SMP nesting count is private)\n");
    return 0;
}

#else

struct Context
{
    volatile ULONG calls;
    ULONG flags;
    BYTE during;
};

AROS_UFH3(ULONG, handler,
    AROS_UFHA(struct Context *, ctx, A1),
    AROS_UFHA(ULONG, flags, D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    ctx->during = SysBase->IDNestCnt;
    ctx->flags = flags;
    ctx->calls++;
    return flags;
    AROS_USERFUNC_EXIT
}

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("SIGNAL EXCEPTION FAIL: %s\n", what);
        failures++;
    }
}

int main(void)
{
    struct Task *task = FindTask(NULL);
    struct Context ctx = {0};
    APTR oldcode = task->tc_ExceptCode;
    APTR olddata = task->tc_ExceptData;
    BYTE initial = SysBase->IDNestCnt;
    BYTE bit = AllocSignal(-1);
    ULONG mask, oldmask, round;

    if (bit < 0)
    {
        check(FALSE, "allocate exception signal");
        goto end;
    }
    mask = 1UL << bit;
    Forbid();
    task->tc_ExceptCode = (APTR)handler;
    task->tc_ExceptData = &ctx;
    oldmask = SetExcept(mask, mask);
    Permit();

    for (round = 0; round < 3; round++)
    {
        ULONG previous = ctx.calls;
        ULONG spins;

        Forbid();
        Signal(task, mask);
        Permit();
        for (spins = 0; spins < 1000000 && ctx.calls == previous; spins++)
            ;
        check(ctx.calls == previous + 1 && ctx.flags == mask,
              "dispatcher delivers the exception once");
        check(ctx.during < 0, "handler runs with interrupts enabled");
        check(SysBase->IDNestCnt == initial,
              "exception return restores interrupt nesting");

        /* Restore the known starting state on a broken ROM so diagnostics
         * can finish without leaving subsequent tests with interrupts off. */
        while (SysBase->IDNestCnt > initial)
            Enable();
    }
    Forbid();
    SetExcept(oldmask, mask);
    SetSignal(0, mask);
    task->tc_ExceptCode = oldcode;
    task->tc_ExceptData = olddata;
    Permit();
    FreeSignal(bit);
end:
    bug("SIGNAL EXCEPTION TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}

#endif /* !__AROSEXEC_SMP__ */
