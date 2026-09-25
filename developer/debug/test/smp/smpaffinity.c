/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for affinity inheritance and SetTaskAffinity().
*/

#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <resources/task.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>
#include <proto/task.h>

#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
APTR TaskResBase;
static int g_NumCPUs = 1;
static int pass, fail;

#define STACKSIZE   16384
#define WAIT_TICKS  250         /* 5s */
#define RACE_LOOPS  2000

struct Spinner
{
    volatile ULONG  sp_CPU;
    volatile ULONG  sp_Count;
    volatile ULONG  sp_Stop;
    volatile ULONG  sp_Done;
    struct Task    *sp_Parent;
    ULONG           sp_Signal;
};

struct Hammer
{
    struct Task    *hm_Target;
    APTR            hm_Mask;
    volatile ULONG  hm_Failed;
    volatile ULONG  hm_Done;
    struct Task    *hm_Parent;
    ULONG           hm_Signal;
};

static void check(BOOL ok, const char *what)
{
    if (ok)
    {
        bug("[smpaffinity] %s OK\n", what);
        pass++;
    }
    else
    {
        bug("[smpaffinity] *** FAIL *** %s\n", what);
        fail++;
    }
}

static APTR mask_of(ULONG bits)
{
    APTR m = KrnAllocCPUMask();
    int i;

    if (m)
    {
        KrnClearCPUMask(m);
        for (i = 0; i < g_NumCPUs; i++)
            if (bits & (1UL << i))
                KrnGetCPUMask(i, m);
    }
    return m;
}

static ULONG bits_of(APTR m)
{
    ULONG bits = 0;
    int i;

    for (i = 0; i < g_NumCPUs; i++)
        if (KrnCPUInMask(i, m))
            bits |= 1UL << i;
    return bits;
}

/* The task's affinity as a bit set, read through task.resource. */
static ULONG affinity_of(struct Task *t)
{
    APTR m = mask_of(0);
    struct TagItem tags[] =
    {
        { TaskTag_CPUAffinity, (IPTR)m },
        { TAG_DONE,            0       }
    };
    ULONG bits;

    if (!m)
        return 0;
    QueryTaskTagList(t, tags);
    bits = bits_of(m);
    KrnFreeCPUMask(m);
    return bits;
}

static void InheritChild(void)
{
    struct Spinner *s = (struct Spinner *)FindTask(NULL)->tc_UserData;

    s->sp_CPU = (ULONG)KrnGetCPUNumber();
    s->sp_Count = affinity_of(FindTask(NULL));
    s->sp_Done = 1;
    Signal(s->sp_Parent, 1UL << s->sp_Signal);
}

static void SpinChild(void)
{
    struct Spinner *s = (struct Spinner *)FindTask(NULL)->tc_UserData;

    while (!s->sp_Stop)
    {
        s->sp_CPU = (ULONG)KrnGetCPUNumber();
        s->sp_Count++;
    }
    s->sp_Done = 1;
    Signal(s->sp_Parent, 1UL << s->sp_Signal);
}

static void HammerChild(void)
{
    struct Hammer *h = (struct Hammer *)FindTask(NULL)->tc_UserData;
    int i;

    for (i = 0; i < RACE_LOOPS; i++)
        if (!SetTaskAffinity(h->hm_Target, h->hm_Mask))
            h->hm_Failed++;
    h->hm_Done = 1;
    Signal(h->hm_Parent, 1UL << h->hm_Signal);
}

static struct Process *start(void (*entry)(void), const char *name, APTR data, APTR affinity)
{
    return CreateNewProcTags(NP_Entry,     (IPTR)entry,
                             NP_Name,      (IPTR)name,
                             NP_StackSize, STACKSIZE,
                             NP_Priority,  -1,
                             NP_UserData,  (IPTR)data,
                             affinity ? NP_Affinity : TAG_IGNORE, (IPTR)affinity,
                             TAG_DONE);
}

static BOOL wait_flag(volatile ULONG *flag)
{
    int i;

    for (i = 0; i < WAIT_TICKS && !*flag; i++)
        Delay(1);
    return *flag != 0;
}

static void stop_spinner(struct Spinner *s)
{
    s->sp_Stop = 1;
    if (!wait_flag(&s->sp_Done))
        bug("[smpaffinity] spinner did not stop\n");
}

/* A child without NP_Affinity takes the parent's set, not the parent's core. */
static void test_inherit(BYTE sigbit)
{
    struct Spinner s;
    ULONG want = (g_NumCPUs > 2) ? 0x6 : 0x2;    /* cpus 1,2 or just 1 */
    APTR m = mask_of(want);

    bug("[smpaffinity] phase 1: inheritance, parent set 0x%lx\n", (unsigned long)want);
    memset(&s, 0, sizeof(s));
    s.sp_Parent = FindTask(NULL);
    s.sp_Signal = sigbit;

    if (!m || !SetTaskAffinity(NULL, m))
    {
        check(FALSE, "restricting the parent");
        KrnFreeCPUMask(m);
        return;
    }
    KrnFreeCPUMask(m);
    check(affinity_of(FindTask(NULL)) == want, "parent reads back its own set");

    if (!start(InheritChild, "smpaffinity.inherit", &s, NULL) || !wait_flag(&s.sp_Done))
    {
        check(FALSE, "child ran");
        return;
    }
    bug("[smpaffinity] child: set 0x%lx, ran on cpu %lu\n",
        (unsigned long)s.sp_Count, (unsigned long)s.sp_CPU);
    check(s.sp_Count == want, "child inherited the parent's set");
    check((want >> s.sp_CPU) & 1, "child ran inside that set");
}

/* SetTaskAffinity() moves a running task, rewriting its mask in place. */
static void test_migrate(BYTE sigbit)
{
    struct Spinner s;
    struct Process *p;
    ULONG all = (1UL << g_NumCPUs) - 1;
    APTR m;
    int k, i;
    char what[64];

    bug("[smpaffinity] phase 2: migrate a running task\n");
    memset(&s, 0, sizeof(s));
    s.sp_Parent = FindTask(NULL);
    s.sp_Signal = sigbit;

    p = start(SpinChild, "smpaffinity.spin", &s, NULL);
    if (!p)
    {
        check(FALSE, "spinner started");
        return;
    }

    for (k = 0; k < g_NumCPUs; k++)
    {
        m = mask_of(1UL << k);
        snprintf(what, sizeof(what), "SetTaskAffinity to cpu %d", k);
        check(m && SetTaskAffinity((struct Task *)p, m), what);
        KrnFreeCPUMask(m);

        for (i = 0; i < WAIT_TICKS && s.sp_CPU != (ULONG)k; i++)
            Delay(1);
        snprintf(what, sizeof(what), "spinner reached cpu %d (on %lu)", k, (unsigned long)s.sp_CPU);
        check(s.sp_CPU == (ULONG)k, what);
        check(affinity_of((struct Task *)p) == (1UL << k), "mask reads back");
    }

    /* Rejected requests leave the mask alone */
    m = mask_of(0);
    check(!SetTaskAffinity((struct Task *)p, NULL), "NULL mask rejected");
    check(m && !SetTaskAffinity((struct Task *)p, m), "empty mask rejected");
    KrnFreeCPUMask(m);
    check(affinity_of((struct Task *)p) == (1UL << (g_NumCPUs - 1)), "rejects left the mask unchanged");

    check(SetTaskAffinity((struct Task *)p, (APTR)TASKAFFINITY_ANY), "TASKAFFINITY_ANY accepted");
    check(affinity_of((struct Task *)p) == all, "ANY covers every cpu");

    stop_spinner(&s);
}

/* Two tasks retarget one that has no mask yet: both allocate, one loses. */
static void test_race(BYTE sigbit)
{
    struct Spinner s;
    struct Hammer h[2];
    struct Process *p;
    ULONG a = 0x1, b = (g_NumCPUs > 2) ? 0x6 : 0x2, got, before;
    APTR ma = mask_of(a), mb = mask_of(b);
    int i;

    bug("[smpaffinity] phase 3: concurrent SetTaskAffinity, %d calls each\n", RACE_LOOPS);
    memset(&s, 0, sizeof(s));
    memset(h, 0, sizeof(h));
    s.sp_Parent = FindTask(NULL);
    s.sp_Signal = sigbit;

    p = start(SpinChild, "smpaffinity.target", &s, (APTR)TASKAFFINITY_ANY);
    if (!p || !ma || !mb)
    {
        check(FALSE, "race setup");
        goto out;
    }

    for (i = 0; i < 2; i++)
    {
        h[i].hm_Target = (struct Task *)p;
        h[i].hm_Mask = i ? mb : ma;
        h[i].hm_Parent = FindTask(NULL);
        h[i].hm_Signal = sigbit;
        if (!start(HammerChild, i ? "smpaffinity.hammer1" : "smpaffinity.hammer0", &h[i], NULL))
            h[i].hm_Done = h[i].hm_Failed = 1;
    }
    for (i = 0; i < 2; i++)
        wait_flag(&h[i].hm_Done);

    check(h[0].hm_Done && h[1].hm_Done, "both hammers finished");
    check(!h[0].hm_Failed && !h[1].hm_Failed, "no SetTaskAffinity call failed");

    got = affinity_of((struct Task *)p);
    bug("[smpaffinity] final set 0x%lx\n", (unsigned long)got);
    check(got == a || got == b, "final mask is one of the two");

    before = s.sp_Count;
    Delay(5);
    check(s.sp_Count != before, "target still runs");

    stop_spinner(&s);
out:
    KrnFreeCPUMask(ma);
    KrnFreeCPUMask(mb);
}

int main(void)
{
    APTR saved;
    BYTE sigbit;

    KernelBase = OpenResource("kernel.resource");
    TaskResBase = OpenResource("task.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();
    if (g_NumCPUs > 32)
        g_NumCPUs = 32;

    bug("[smpaffinity] start: cpus=%d\n", g_NumCPUs);

    if (!KernelBase || !TaskResBase || g_NumCPUs < 2)
    {
        bug("[smpaffinity] INVALID (needs task.resource and at least 2 CPUs)\n");
        return RETURN_WARN;
    }

    sigbit = AllocSignal(-1);
    saved = mask_of(affinity_of(FindTask(NULL)));
    if (sigbit == -1 || !saved)
    {
        bug("[smpaffinity] INVALID (no signal bit or mask)\n");
        return RETURN_FAIL;
    }

    test_inherit(sigbit);
    test_migrate(sigbit);
    test_race(sigbit);

    SetTaskAffinity(NULL, saved);
    KrnFreeCPUMask(saved);
    FreeSignal(sigbit);

    bug("[smpaffinity] DONE: %d PASS, %d FAIL, 0 INVALID\n", pass, fail);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
