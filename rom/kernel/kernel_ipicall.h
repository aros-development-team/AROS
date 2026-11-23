#ifndef KERNEL_IPICALL_H_
#define KERNEL_IPICALL_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Requests from one CPU to another, shared by every SMP port.

    Two things go from CPU to CPU, and both work the same way on every port:

    A request is a bit in the target CPU's pending word. Setting a bit that
    is set already changes nothing and sends nothing: the target is
    interrupted once, takes the whole word and acts on every bit in it. A
    thousand "reschedule" requests for a CPU that has not got round to the
    first one are one interrupt.

    A call is a hook that the target CPU runs in its interrupt handler. The
    calls for a CPU are kept in a queue that any CPU adds to without taking a
    lock; the target takes the whole queue at once and runs it in the order
    it was filled. That there are calls is itself a request bit, so the
    target is interrupted when its queue stops being empty and not once per
    call.

    The port supplies, in its kernel_ipiport.h (rom/kernel has an empty one
    for ports with one CPU):

        KRN_IPICALL                     defined, to build this code
        KRN_IPICALL_MAXCPUS             the most CPUs the port can have
        KRN_IPICALL_STATIC              optional: keep the queues in the kernel's
                                        own data, for a port that sets them up
                                        before exec's allocator can be used
        KRN_IPIF_CALL                   the pending bit that means "calls"
        ULONG krnIPIPortThisCPU(void)   the number of the calling CPU
        void krnIPIPortRaise(ULONG cpu, ULONG bits)
                                        interrupt that CPU; bits are the
                                        ones that have just become pending
        ULONG krnIPIPortIntrDisable(void), void krnIPIPortIntrRestore(ULONG)
                                        mask every interrupt of this CPU,
                                        the IPI included, and put it back
        void krnIPIPortPause(void)      between two attempts of a busy wait
        BOOL krnIPIPortCanServe(void)   TRUE if this CPU cannot take its IPI
                                        now, so that a sender who has to
                                        wait must run its own calls itself
        void krnIPIPortServe(void)      run krnIPICallDrain() from a sender,
                                        in whatever context a hook needs
*/

#include <exec/types.h>
#include <utility/hooks.h>

#define IPI_CALL_HOOK_MAX_ARGS  5

/* What a call's hook is given as its first argument */
struct IPIHook
{
    struct Hook ih_Hook;
    IPTR        ih_Args[IPI_CALL_HOOK_MAX_ARGS];
};

#include <kernel_ipiport.h>

#if defined(KRN_IPICALL)

/* Set up the queues of the first 'cpus' CPUs. Needs exec's allocator; can be called again for more. */
void krnIPICallInit(ULONG cpus);

/*
 * Make 'bits' pending for a CPU and interrupt it for those that were not
 * pending before. Can be called from any context.
 */
void krnIPIPost(ULONG cpu, ULONG bits);

/* The calling CPU's pending bits out of 'bits'; they are cleared. For its IPI handler. */
ULONG krnIPIAck(ULONG bits);

/*
 * Have every CPU in the mask (one bit per CPU, 'words' ULONGs, online CPUs
 * only) run the hook. With async = 0 the call returns when all of them
 * have. The calling CPU can be in the mask: it is interrupted like any
 * other. A CPU that has no queue yet is left out. Returns the number of
 * CPUs the call went to.
 */
ULONG krnIPICall(struct Hook *hook, const ULONG *mask, ULONG words, BOOL async, ULONG nargs, IPTR *args);

/* Run the calls queued for the calling CPU. Interrupts are off. For its IPI handler. */
void krnIPICallDrain(void);

/*
 * Drop the asynchronous calls of this hook entry whose second argument is
 * arg1 and that no CPU has started on, and wait for one that is running.
 */
void krnIPICallCancel(APTR hookEntry, IPTR arg1);

#endif /* KRN_IPICALL */

#endif /* KERNEL_IPICALL_H_ */
