/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/macros.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/mbox.h>

#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#include "mbox_private.h"


static int mbox_init(struct MBoxBase *MBoxBase)
{
    int retval = TRUE;

    D(bug("[MBox] mbox_init()\n"));

    InitSemaphore(&MBoxBase->mbox_Sem);

    D(bug("[MBox] mbox_init: Initialised Semaphore @ 0x%p\n", &MBoxBase->mbox_Sem));

    return retval;
}

volatile unsigned int *mbox_call_locked(struct MBoxBase *MBoxBase, void *mb,
    unsigned int chan, void *msg)
{
    unsigned int try = 0x2000000;
    unsigned int reply;
    ULONG length;
    void *phys_addr;

    if ((((unsigned int)msg & VCMB_CHAN_MASK) != 0) || (chan > VCMB_CHAN_MAX))
        return (volatile unsigned int *)-1;

    /* Messages must own their cache lines; see <proto/mbox.h> for why. */
    if (((IPTR)msg & (MBOX_MSG_ALIGN - 1)) != 0)
        bug("[MBox] WARNING: message @ 0x%p not cache-line aligned — "
            "reply invalidate will clip neighbouring memory (caller %p)\n",
            msg, __builtin_return_address(0));

    length = AROS_LE2LONG(((ULONG *)msg)[0]);
    phys_addr = CachePreDMA(msg, &length, DMA_ReadFromRAM);

    ObtainSemaphore(&MBoxBase->mbox_Sem);

    {
        unsigned int wtry = 0x2000000;
        while ((MBoxStatus(mb) & VCMB_STATUS_WRITEREADY) != 0)
        {
            __asm__ __volatile__("dsb sy" ::: "memory");
            if (--wtry == 0)
            {
                ReleaseSemaphore(&MBoxBase->mbox_Sem);
                return (volatile unsigned int *)-1;
            }
        }
    }

    __asm__ __volatile__("dmb sy" ::: "memory");
    *((volatile unsigned int *)(mb + VCMB_WRITE)) =
        AROS_LONG2LE(((unsigned int)phys_addr | chan));

    /* Drain the inbox until either our own response shows up (matched by
     * channel AND by physical address — the address check distinguishes our
     * submission from any other caller queued on the same channel) or the
     * timeout budget is exhausted.*/
    while (try > 0)
    {
        if ((MBoxStatus(mb) & VCMB_STATUS_READREADY) != 0)
        {
            __asm__ __volatile__("dsb sy" ::: "memory");
            try--;
            continue;
        }

        __asm__ __volatile__("dmb sy" ::: "memory");
        reply = AROS_LE2LONG(*((volatile unsigned int *)(mb + VCMB_READ)));
        __asm__ __volatile__("dmb sy" ::: "memory");

        if ((reply & VCMB_CHAN_MASK) == chan)
        {
            uint32_t *addr = (uint32_t *)(reply & ~VCMB_CHAN_MASK);
            uint32_t len = AROS_LE2LONG(addr[0]);

            CacheClearE(addr, len, CACRF_InvalidateD);

            if ((void *)addr == phys_addr)
            {
                ReleaseSemaphore(&MBoxBase->mbox_Sem);
                return (volatile unsigned int *)msg;
            }
        }
        /* Wrong channel or wrong address — drop the message and keep
         * draining until we find ours, or 'try' reaches zero. */
    }

    ReleaseSemaphore(&MBoxBase->mbox_Sem);
    return (volatile unsigned int *)-1;
}

/*****************************************************************************

    NAME */
        AROS_LH1(unsigned int, MBoxStatus,

/*  SYNOPSIS */
                        AROS_LHA(void *, mb, A0),

/*  LOCATION */
                        struct MBoxBase *, MBoxBase, 1, Mbox)
/*  FUNCTION
        Read the mailbox status register and convert it to CPU byte order.

    INPUTS
        mb - Valid CPU-accessible base address of the mailbox registers.

    RESULT
        The raw status bits. Despite their names, a set
        VCMB_STATUS_READREADY bit (bit 30) means the read FIFO is empty;
        a set VCMB_STATUS_WRITEREADY bit (bit 31) means the write FIFO is
        full. The transfer routines wait while these bits are set.

    NOTES
        No semaphore is acquired, no message is consumed and no wait is
        performed. This is an instantaneous observation, not a reservation
        of FIFO space or proof that the next operation will not wait.

    SEE ALSO
        MBoxRead(), MBoxWrite(), MBoxCall()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[MBox] MBoxStatus(0x%p)\n", mb));

    return AROS_LE2LONG(*((volatile unsigned int *)(mb + VCMB_STATUS)));

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(volatile unsigned int *, MBoxRead,

/*  SYNOPSIS */
                        AROS_LHA(void *, mb, A0),
                        AROS_LHA( unsigned int, chan, D0),

/*  LOCATION */
                        struct MBoxBase *, MBoxBase, 2, Mbox)
/*  FUNCTION
        Read messages until a reply for the requested channel is found.
        The channel bits are removed from its address and the indicated
        message range is invalidated from the data cache before return.

    INPUTS
        mb - Valid CPU-accessible base address of the mailbox registers.
        chan - Mailbox channel from 0 through VCMB_CHAN_MAX (8).

    RESULT
        The address carried by the matching reply, or
        (volatile unsigned int *)-1 for an invalid channel or exhausted
        empty-FIFO polling budget. NULL is not the error sentinel.

    NOTES
        Message buffers must start at MBOX_MSG_ALIGN (64-byte) alignment
        and exclusively own every cache line covered by the message,
        including its final line. The first ULONG is the total message
        length in little-endian byte order. Prepare the remaining data in
        the format required by the selected firmware service. Memory must
        be contiguous and accessible to the firmware; the physical address
        must fit the mailbox's 32-bit address field. The implementation
        uses reply addresses directly as CPU pointers.

        The reply buffer is borrowed; this function allocates no message
        storage and does not transfer ownership. It does not verify the
        reply address against a particular request. Other-channel messages
        are consumed and discarded, not queued for their intended callers.
        Coordinate all users of the mailbox accordingly.

        Call from task context where waiting is permitted. The resource
        semaphore serializes FIFO access, but a separate MBoxWrite() and
        MBoxRead() pair is not one indivisible request/reply operation.
        Prefer MBoxCall() when matching a submitted message.

    BUGS
        The budget of 0x2000000 polls decreases only while the FIFO is
        empty. A stream of unmatched messages does not consume it, so the
        call has no guaranteed finite duration. Reply addresses and lengths
        are trusted and are not checked against caller-owned storage.

    SEE ALSO
        MBoxCall(), MBoxWrite(), MBoxStatus()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    unsigned int try = 0x2000000;
    unsigned int msg;

    D(bug("[MBox] MBoxRead(chan %d @ 0x%p)\n", chan, mb));

    if (chan > VCMB_CHAN_MAX)
        return (volatile unsigned int *)-1;

    ObtainSemaphore(&MBoxBase->mbox_Sem);

    /* Drain until we find a message for our channel or the timeout
     * budget runs out. */
    while (try > 0)
    {
        if ((MBoxStatus(mb) & VCMB_STATUS_READREADY) != 0)
        {
            asm volatile ("dsb sy" ::: "memory");
            try--;
            continue;
        }

        asm volatile ("dmb sy" ::: "memory");
        msg = AROS_LE2LONG(*((volatile unsigned int *)(mb + VCMB_READ)));
        asm volatile ("dmb sy" ::: "memory");

        if ((msg & VCMB_CHAN_MASK) == chan)
        {
            uint32_t *addr = (uint32_t *)(msg & ~VCMB_CHAN_MASK);
            uint32_t len = AROS_LE2LONG(addr[0]);

            ReleaseSemaphore(&MBoxBase->mbox_Sem);

            /* See mbox_call_locked: the invalidate clips whole lines. */
            if (((IPTR)addr & 63) != 0)
                bug("[MBox] WARNING: reply @ 0x%p not cache-line aligned — "
                    "invalidate will clip neighbouring memory\n", addr);

            CacheClearE(addr, len, CACRF_InvalidateD);

            return (volatile unsigned int *)(addr);
        }
        /* Wrong channel — drop and keep draining. */
    }

    ReleaseSemaphore(&MBoxBase->mbox_Sem);
    return (volatile unsigned int *)-1;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(void, MBoxWrite,

/*  SYNOPSIS */
                        AROS_LHA(void *, mb, A0),
                        AROS_LHA( unsigned int, chan, D0),
                        AROS_LHA(void *, msg, A1),

/*  LOCATION */
                        struct MBoxBase *, MBoxBase, 3, Mbox)
/*  FUNCTION
        Prepare a message for firmware access using CachePreDMA(), wait
        for write-FIFO space and submit its physical address with the
        channel number. This call does not wait for the reply.

    INPUTS
        mb - Valid CPU-accessible base address of the mailbox registers.
        chan - Mailbox channel from 0 through VCMB_CHAN_MAX (8).
        msg - Non-NULL pointer to a prepared message satisfying the buffer
              requirements below.

    RESULT
        None. An invalid channel or an address not aligned to 16 bytes
        silently causes no submission. Successful return does not report
        firmware completion or service-level success.

    NOTES
        Message buffers must start at MBOX_MSG_ALIGN (64-byte) alignment
        and exclusively own every cache line covered by the message,
        including its final line. The first ULONG is the total message
        length in little-endian byte order. Prepare the remaining data in
        the format required by the selected firmware service. Memory must
        be contiguous and accessible to the firmware; the physical address
        must fit the mailbox's 32-bit address field. The implementation
        uses reply addresses directly as CPU pointers.

        The caller retains ownership. Keep the message allocated and do not
        modify or reuse it until firmware has finished using it. Obtain the
        reply separately; this function does not perform reply cache
        invalidation. No allocation is performed by the resource itself.

        Call from task context where waiting is permitted. FIFO submission
        uses the resource semaphore. Independent write/read calls can be
        interleaved with other users; prefer MBoxCall() for a matched
        request/reply exchange.

    BUGS
        The write-FIFO polling loop has no timeout and can wait forever
        while holding the resource semaphore. NULL is not rejected before
        the message header is read. Cache-line ownership, message length,
        physical-address range and DMA contiguity are not validated.

    SEE ALSO
        MBoxRead(), MBoxCall(), MBoxStatus(), exec.library/CachePreDMA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[MBOX] MBoxWrite(chan %d @ 0x%p, msg @ 0x%p)\n", chan, mb, msg));

    if ((((unsigned int)msg & VCMB_CHAN_MASK) == 0) && (chan <= VCMB_CHAN_MAX))
    {
        ULONG length = AROS_LE2LONG(((ULONG *)msg)[0]);

        void *phys_addr = CachePreDMA(msg, &length, DMA_ReadFromRAM);

        ObtainSemaphore(&MBoxBase->mbox_Sem);

        while ((MBoxStatus(mb) & VCMB_STATUS_WRITEREADY) != 0)
        {
            /* Data synchronization barrier */
            asm volatile ("dsb sy" ::: "memory");
        }

        asm volatile ("dmb sy" ::: "memory");

        *((volatile unsigned int *)(mb + VCMB_WRITE)) = AROS_LONG2LE(((unsigned int)phys_addr | chan));

        ReleaseSemaphore(&MBoxBase->mbox_Sem);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(volatile unsigned int *, MBoxCall,

/*  SYNOPSIS */
                        AROS_LHA(void *, mb, A0),
                        AROS_LHA(unsigned int, chan, D0),
                        AROS_LHA(void *, msg, A1),

/*  LOCATION */
                        struct MBoxBase *, MBoxBase, 4, Mbox)
/*  FUNCTION
        Submit a prepared mailbox message and wait for a reply matching
        both its channel and physical address. The resource holds its
        semaphore across submission and reply selection. The reply's
        message range is invalidated from the data cache before return.

    INPUTS
        mb - Valid CPU-accessible base address of the mailbox registers.
        chan - Mailbox channel from 0 through VCMB_CHAN_MAX (8).
        msg - Pointer to a prepared message satisfying the buffer
              requirements below. NULL is rejected.

    RESULT
        The original msg pointer when a matching reply is found, or
        (volatile unsigned int *)-1 for NULL, an invalid channel, failure
        of 16-byte address alignment, or an exhausted polling budget.
        A matching reply does not establish firmware service-level success;
        inspect the response header and tag status for the requested service.

    NOTES
        Message buffers must start at MBOX_MSG_ALIGN (64-byte) alignment
        and exclusively own every cache line covered by the message,
        including its final line. The first ULONG is the total message
        length in little-endian byte order. Prepare the remaining data in
        the format required by the selected firmware service. Memory must
        be contiguous and accessible to the firmware; the physical address
        must fit the mailbox's 32-bit address field. The implementation
        uses reply addresses directly as CPU pointers.

        The caller retains ownership. No message allocation is performed.
        Other-channel and other-address replies are consumed and discarded.
        This can affect clients sharing the mailbox even though each call
        uses the resource semaphore. Coordinate users of all channels.

        Call from task context where waiting is permitted. The write wait
        and empty-read wait each have a 0x2000000-poll budget, measured in
        iterations rather than time. Semaphore acquisition is not bounded.
        An error after submission does not cancel firmware access; do not
        assume it is then safe to free or reuse the message.

    BUGS
        The read budget decreases only on empty-FIFO polls, not discarded
        messages. Unmatched traffic can therefore keep the call running
        indefinitely. For same-channel replies, the supplied address and
        length are dereferenced and invalidated before address matching.
        They are not validated. Alignment to 64 bytes is diagnosed but not
        enforced; message length, physical range and DMA contiguity are
        also not checked.

    SEE ALSO
        MBoxWrite(), MBoxRead(), MBoxStatus(), exec.library/CachePreDMA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[MBOX] MBoxCall(chan %d @ 0x%p, msg @ 0x%p)\n", chan, mb, msg));

    if (msg == NULL)
        return (volatile unsigned int *)-1;

    return mbox_call_locked(MBoxBase, mb, chan, msg);

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(mbox_init, 0)
