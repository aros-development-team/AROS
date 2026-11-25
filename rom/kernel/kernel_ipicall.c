/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Requests from one CPU to another, shared by every SMP port.
          See kernel_ipicall.h.
*/

#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_ipicall.h>

#if defined(KRN_IPICALL)

/* Calls a CPU can have on their way to other CPUs at one time */
#define KRN_IPICALL_POOL    64

/*
 * A call. It belongs to the pool of the CPU that sent it and goes back
 * there when the target has run it. The memory is never freed, so a CPU may
 * look at any entry at any time; kic_State says whether what it sees is
 * current.
 *
 * kic_State is a status in the two low bits and a count above them that
 * goes up every time the entry is used again. Every change of status is a
 * compare and exchange on the whole word, so a CPU that read the state of
 * one use of the entry cannot act on the next one by mistake.
 */
#define KIC_FREE        0
#define KIC_QUEUED      1
#define KIC_RUNNING     2
#define KIC_CANCELLED   3
#define KIC_STATUS(s)   ((s) & 3)
#define KIC_NEXTUSE(s)  ((((s) >> 2) + 1) << 2)     /* status KIC_FREE */

struct KrnIPICall
{
    struct KrnIPICall  *kic_Next;       /* in a queue, a returned list or a free list   */
    ULONG               kic_State;
    ULONG               kic_Owner;      /* the CPU whose pool it is from                */
    ULONG              *kic_Done;       /* synchronous call: CPUs that still have to run it */
    struct IPIHook      kic_Hook;
};

struct KrnIPICPU
{
    ULONG               kcp_Pending;    /* request bits for this CPU                    */
    struct KrnIPICall  *kcp_Queue;      /* calls for this CPU; any CPU adds, it takes all */
    struct KrnIPICall  *kcp_Returned;   /* its own entries coming back; the same        */
    struct KrnIPICall  *kcp_Free;       /* its own free entries; this CPU only, interrupts off */
    BOOL                kcp_Draining;   /* this CPU only                                */
    struct KrnIPICall   kcp_Pool[KRN_IPICALL_POOL];
};

static struct KrnIPICPU *krnIPICPUs[KRN_IPICALL_MAXCPUS];

#if defined(KRN_IPICALL_STATIC)
/* For a port that sets its queues up before exec's allocator can be used */
static struct KrnIPICPU krnIPIStatic[KRN_IPICALL_MAXCPUS];
#endif

static inline struct KrnIPICPU *krnIPIData(ULONG cpu)
{
    if (cpu >= KRN_IPICALL_MAXCPUS)
        return NULL;
    return __atomic_load_n(&krnIPICPUs[cpu], __ATOMIC_ACQUIRE);
}

/*
 * Add to a list that any CPU adds to and one CPU empties. The CPU that
 * empties it takes the whole list in one exchange, so an entry that is
 * being added is either on it or not: there is nothing to lock.
 * TRUE if the list was empty.
 */
static inline BOOL krnIPIPush(struct KrnIPICall **list, struct KrnIPICall *call)
{
    struct KrnIPICall *old = __atomic_load_n(list, __ATOMIC_RELAXED);

    do
    {
        call->kic_Next = old;
    } while (!__atomic_compare_exchange_n(list, &old, call, TRUE, __ATOMIC_RELEASE, __ATOMIC_RELAXED));

    return (old == NULL);
}

void krnIPICallInit(ULONG cpus)
{
    ULONG cpu, i;

    if (cpus > KRN_IPICALL_MAXCPUS)
        cpus = KRN_IPICALL_MAXCPUS;

    for (cpu = 0; cpu < cpus; cpu++)
    {
        struct KrnIPICPU *kcp;

        if (krnIPIData(cpu))
            continue;

#if defined(KRN_IPICALL_STATIC)
        kcp = &krnIPIStatic[cpu];
#else
        kcp = AllocMem(sizeof(struct KrnIPICPU), MEMF_PUBLIC | MEMF_CLEAR);
        if (!kcp)
            break;
#endif

        for (i = 0; i < KRN_IPICALL_POOL; i++)
        {
            kcp->kcp_Pool[i].kic_Owner = cpu;
            kcp->kcp_Pool[i].kic_Next = kcp->kcp_Free;
            kcp->kcp_Free = &kcp->kcp_Pool[i];
        }

        __atomic_store_n(&krnIPICPUs[cpu], kcp, __ATOMIC_RELEASE);
    }
}

void krnIPIPost(ULONG cpu, ULONG bits)
{
    struct KrnIPICPU *kcp = krnIPIData(cpu);
    ULONG old;

    if (!kcp)
    {
        /* No pending word to keep the request in: every one is sent */
        krnIPIPortRaise(cpu, bits);
        return;
    }

    old = __atomic_fetch_or(&kcp->kcp_Pending, bits, __ATOMIC_SEQ_CST);

    /* Bits that were pending already are on their way, or waiting for the CPU to take them */
    if (bits & ~old)
        krnIPIPortRaise(cpu, bits & ~old);
}

ULONG krnIPIAck(ULONG bits)
{
    struct KrnIPICPU *kcp = krnIPIData(krnIPIPortThisCPU());

    if (!kcp)
        return bits;

    return __atomic_fetch_and(&kcp->kcp_Pending, ~bits, __ATOMIC_SEQ_CST) & bits;
}

/*
 * An entry from the calling CPU's pool. When all of them are on their way
 * the sender waits for one to come back; a CPU that cannot take its own
 * IPI meanwhile runs its own calls, or two such CPUs that call each other
 * would wait for each other for ever.
 */
static struct KrnIPICall *krnIPICallAlloc(void)
{
    for (;;)
    {
        ULONG flags = krnIPIPortIntrDisable();
        struct KrnIPICPU *kcp = krnIPIData(krnIPIPortThisCPU());
        struct KrnIPICall *call = NULL;

        if (kcp)
        {
            if (!kcp->kcp_Free)
                kcp->kcp_Free = __atomic_exchange_n(&kcp->kcp_Returned, NULL, __ATOMIC_ACQUIRE);

            call = kcp->kcp_Free;
            if (call)
                kcp->kcp_Free = call->kic_Next;
        }

        krnIPIPortIntrRestore(flags);

        if (call || !kcp)
            return call;

        if (krnIPIPortCanServe())
            krnIPIPortServe();
        krnIPIPortPause();
    }
}

ULONG krnIPICall(struct Hook *hook, const ULONG *mask, ULONG words, BOOL async, ULONG nargs, IPTR *args)
{
    ULONG done = 0;
    ULONG cpu, i, sent = 0;

    if (!hook || !mask || (nargs > IPI_CALL_HOOK_MAX_ARGS))
        return 0;

    for (cpu = 0; (cpu < words * 32) && (cpu < KRN_IPICALL_MAXCPUS); cpu++)
    {
        struct KrnIPICPU *target;
        struct KrnIPICall *call;
        ULONG state;

        if (!(mask[cpu / 32] & (1UL << (cpu % 32))))
            continue;
        if (!(target = krnIPIData(cpu)))
            continue;
        if (!(call = krnIPICallAlloc()))
            continue;

        call->kic_Hook.ih_Hook.h_Entry = hook->h_Entry;
        call->kic_Hook.ih_Hook.h_SubEntry = hook->h_SubEntry;
        call->kic_Hook.ih_Hook.h_Data = hook->h_Data;
        for (i = 0; i < nargs; i++)
            call->kic_Hook.ih_Args[i] = args[i];

        if (async)
            call->kic_Done = NULL;
        else
        {
            /* Before it is queued: the target may run it straight away */
            __atomic_fetch_add(&done, 1, __ATOMIC_RELAXED);
            call->kic_Done = &done;
        }

        /* From here the entry is a call, for the target and for a cancel */
        state = __atomic_load_n(&call->kic_State, __ATOMIC_RELAXED);
        __atomic_store_n(&call->kic_State, (state & ~3UL) | KIC_QUEUED, __ATOMIC_RELEASE);

        krnIPIPush(&target->kcp_Queue, call);
        krnIPIPost(cpu, KRN_IPIF_CALL);
        sent++;
    }

    /* Keep our own queue moving while we wait, if nothing else can */
    while (__atomic_load_n(&done, __ATOMIC_ACQUIRE))
    {
        if (krnIPIPortCanServe())
            krnIPIPortServe();
        krnIPIPortPause();
    }

    return sent;
}

/* Give an entry back to the pool it is from. Interrupts are off. */
static inline void krnIPICallRelease(struct KrnIPICPU *me, ULONG mycpu, struct KrnIPICall *call)
{
    ULONG state = __atomic_load_n(&call->kic_State, __ATOMIC_RELAXED);

    /* A cancel that waited for this call goes on from here */
    __atomic_store_n(&call->kic_State, KIC_NEXTUSE(state), __ATOMIC_RELEASE);

    if (call->kic_Owner == mycpu)
    {
        call->kic_Next = me->kcp_Free;
        me->kcp_Free = call;
    }
    else
    {
        struct KrnIPICPU *owner = krnIPIData(call->kic_Owner);

        krnIPIPush(&owner->kcp_Returned, call);
    }
}

void krnIPICallDrain(void)
{
    ULONG mycpu = krnIPIPortThisCPU();
    struct KrnIPICPU *me = krnIPIData(mycpu);

    if (!me)
        return;

    /*
     * Not from inside one of this drain's own hooks. A hook that sends a
     * call on and has to wait for an entry waits without running more
     * calls; the drain it was called from carries on with the queue when
     * the hook returns.
     */
    if (me->kcp_Draining)
        return;
    me->kcp_Draining = TRUE;

    for (;;)
    {
        struct KrnIPICall *list = __atomic_exchange_n(&me->kcp_Queue, NULL, __ATOMIC_ACQUIRE);
        struct KrnIPICall *call, *next, *fifo = NULL;

        if (!list)
            break;

        /* The list has the newest call first; run them oldest first */
        for (call = list; call; call = next)
        {
            next = call->kic_Next;
            call->kic_Next = fifo;
            fifo = call;
        }

        for (call = fifo; call; call = next)
        {
            ULONG state = __atomic_load_n(&call->kic_State, __ATOMIC_ACQUIRE);
            ULONG *done;

            next = call->kic_Next;

            /* Ours to run unless a cancel got there first */
            if ((KIC_STATUS(state) == KIC_QUEUED) &&
                __atomic_compare_exchange_n(&call->kic_State, &state, (state & ~3UL) | KIC_RUNNING,
                                            FALSE, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
            {
                CALLHOOKPKT(&call->kic_Hook.ih_Hook, NULL, 0);
            }

            done = call->kic_Done;
            krnIPICallRelease(me, mycpu, call);

            /* The sender's counter is gone the moment it reads zero */
            if (done)
                __atomic_fetch_sub(done, 1, __ATOMIC_RELEASE);
        }
    }

    me->kcp_Draining = FALSE;
}

void krnIPICallCancel(APTR hookEntry, IPTR arg1)
{
    ULONG cpu, i;
    BOOL again;

    /*
     * A call that is running can queue another for the same argument (a
     * forwarded Signal() forwards again if its task has moved), and the
     * new one may be in a pool this pass has been through already. So the
     * pools are gone through until a pass meets no call for the argument,
     * queued or running. The caller has seen to it that no new chain
     * starts: for Signal() the task is in a terminal state by now.
     */
    do
    {
    again = FALSE;

    for (cpu = 0; cpu < KRN_IPICALL_MAXCPUS; cpu++)
    {
        struct KrnIPICPU *kcp = krnIPIData(cpu);

        if (!kcp)
            continue;

        for (i = 0; i < KRN_IPICALL_POOL; i++)
        {
            struct KrnIPICall *call = &kcp->kcp_Pool[i];

            for (;;)
            {
                ULONG state = __atomic_load_n(&call->kic_State, __ATOMIC_ACQUIRE);
                ULONG status = KIC_STATUS(state);

                if ((status != KIC_QUEUED) && (status != KIC_RUNNING))
                    break;

                /*
                 * What the entry holds belongs to the use the state was
                 * read for; if the entry has moved on, the exchange below
                 * fails or the state read in the wait differs, and what
                 * was read here is not acted on.
                 */
                if (((APTR)call->kic_Hook.ih_Hook.h_Entry != hookEntry) ||
                    (call->kic_Hook.ih_Args[1] != arg1) || (call->kic_Done != NULL))
                {
                    if (__atomic_load_n(&call->kic_State, __ATOMIC_ACQUIRE) == state)
                        break;
                    continue;
                }

                if (status == KIC_QUEUED)
                {
                    /* The target finds it cancelled and gives it back unrun */
                    if (__atomic_compare_exchange_n(&call->kic_State, &state, (state & ~3UL) | KIC_CANCELLED,
                                                    FALSE, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
                    {
                        again = TRUE;
                        break;
                    }
                    continue;
                }

                /* Running on its target now: let it finish */
                while (__atomic_load_n(&call->kic_State, __ATOMIC_ACQUIRE) == state)
                {
                    if (krnIPIPortCanServe())
                        krnIPIPortServe();
                    krnIPIPortPause();
                }
                again = TRUE;
                break;
            }
        }
    }

    } while (again);
}

#endif /* KRN_IPICALL */
