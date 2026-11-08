/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/dos.h>
#include <exec/ports.h>
#include <devices/timer.h>

#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/random.h>

/*
 * All time comes from timer.device: jiffies and ktime both count from
 * boot (GetUpTime), so setting the clock moves neither; delays are TR_ADDREQUEST on a per-caller port, and kernel timers
 * are requests answered on the timer process, which runs the callback.
 */

struct Device *TimerBase = NULL;
static struct MsgPort *timer_init_port = NULL;
static struct timerequest *timer_init_req = NULL;

static struct Process *timer_proc = NULL;
static struct Task *timer_parent = NULL;
static ULONG timer_parent_ack;
static struct SignalSemaphore timer_lock;

static int init_timerbase(void)
{
    timer_init_port = CreateMsgPort();
    if (!timer_init_port)
        return FALSE;
    timer_init_req = CreateIORequest(timer_init_port, sizeof(struct timerequest));
    if (!timer_init_req) {
        DeleteMsgPort(timer_init_port);
        return FALSE;
    }
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)timer_init_req, 0) != 0) {
        DeleteIORequest(timer_init_req);
        DeleteMsgPort(timer_init_port);
        return FALSE;
    }
    TimerBase = timer_init_req->tr_node.io_Device;
    InitSemaphore(&timer_lock);
    return TRUE;
}

static int exit_timerbase(void)
{
    if (timer_init_req) {
        CloseDevice((struct IORequest *)timer_init_req);
        DeleteIORequest(timer_init_req);
        DeleteMsgPort(timer_init_port);
        timer_init_req = NULL;
        TimerBase = NULL;
    }
    return TRUE;
}

ADD2INIT(init_timerbase, 5);
ADD2EXIT(exit_timerbase, 5);

/* an opened request other users copy to build their own */
struct timerequest *compat_timer_template(void)
{
    return timer_init_req;
}

/* --- clocks -------------------------------------------------------- */

ktime_t ktime_get(void)
{
    struct timeval tv;

    if (!TimerBase)
        return 0;
    GetUpTime(&tv);
    return (ktime_t)tv.tv_secs * NSEC_PER_SEC + (ktime_t)tv.tv_micro * NSEC_PER_USEC;
}

/* microseconds since boot, for the hidd's own diagnostics */
unsigned long amdgpu_compat_usecs(void)
{
    return (unsigned long)(ktime_get() / NSEC_PER_USEC);
}

u64 compat_jiffies64(void)
{
    struct timeval tv;

    if (!TimerBase)
        return 0;
    GetUpTime(&tv);
    return (u64)tv.tv_secs * 1000ULL + tv.tv_micro / 1000;
}

unsigned long compat_jiffies(void)
{
    return (unsigned long)compat_jiffies64();
}

u32 get_random_u32(void)
{
    static u32 seed = 0x2545f491;
    struct timeval tv;

    if (TimerBase) {
        GetSysTime(&tv);
        seed ^= tv.tv_micro;
    }
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

/* --- delays -------------------------------------------------------- */

/*
 * A delay is a timer request on a port that lives on the caller's stack.
 * Very short waits spin on the clock instead: a request round trip
 * costs more than the wait itself.
 */
static void timed_wait(unsigned long usecs)
{
    struct timerequest req;
    struct MsgPort port;

    if (!TimerBase)
        return;

    memset(&port, 0, sizeof(port));
    port.mp_Node.ln_Type = NT_MSGPORT;
    port.mp_Flags = PA_SIGNAL;
    port.mp_SigBit = AllocSignal(-1);
    port.mp_SigTask = FindTask(NULL);
    NEWLIST(&port.mp_MsgList);
    if (port.mp_SigBit == (UBYTE)-1) {
        ktime_t end = ktime_get() + (ktime_t)usecs * NSEC_PER_USEC;
        while (ktime_get() < end)
            ;
        return;
    }

    req = *timer_init_req;
    req.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    req.tr_node.io_Message.mn_ReplyPort = &port;
    req.tr_node.io_Command = TR_ADDREQUEST;
    req.tr_time.tv_secs = usecs / 1000000;
    req.tr_time.tv_micro = usecs % 1000000;

    DoIO((struct IORequest *)&req);
    FreeSignal(port.mp_SigBit);
}

/*
 * Set while the shutdown reset callback unloads the card: a wait that
 * yields there would let the callback chain advance to the platform
 * reset in the middle of the unload, so every wait spins instead.
 */
int amdgpu_compat_atomic;

/* Linux busy-waits these, and callers rely on that (they hold spinlocks) */
static void spin_wait(unsigned long usecs)
{
    ktime_t end = ktime_get() + (ktime_t)usecs * NSEC_PER_USEC;

    if (!TimerBase)
        return;
    while (ktime_get() < end)
        ;
}

void udelay(unsigned long usecs)
{
    spin_wait(usecs);
}

void ndelay(unsigned long nsecs)
{
    spin_wait((nsecs + 999) / 1000);
}

void mdelay(unsigned long msecs)
{
    spin_wait(msecs * 1000);
}

/* these may sleep */
void msleep(unsigned int msecs)
{
    if (amdgpu_compat_atomic)
        spin_wait((unsigned long)msecs * 1000);
    else
        timed_wait((unsigned long)msecs * 1000);
}

unsigned long msleep_interruptible(unsigned int msecs)
{
    msleep(msecs);
    return 0;
}

void usleep_range(unsigned long min, unsigned long max)
{
    if (min < 20 || amdgpu_compat_atomic)
        spin_wait(min);
    else
        timed_wait(min);
}


/* --- kernel timers ---------------------------------------------------- */

/*
 * Armed timers are one list, edited in place under a lock that disables
 * interrupts: arming and cancelling are atomic for the caller, as in
 * Linux, so they are legal under a spinlock with interrupts off, from an
 * interrupt, and a timer may be freed right after del_timer(). The timer
 * process only sleeps until the earliest deadline, woken by Signal() when
 * the list changes, and runs the callbacks.
 */
static struct MinList timer_armed;
static volatile LONG timer_armed_lock;
static struct timer_list * volatile timer_running;
static ULONG timer_wake_mask;

static inline void timer_list_lock(void)
{
    Disable();
    while (__atomic_exchange_n(&timer_armed_lock, 1, __ATOMIC_ACQUIRE))
        cpu_relax();
    if (!timer_armed.mlh_Head)
        NEWLIST((struct List *)&timer_armed);
}

static inline void timer_list_unlock(void)
{
    __atomic_store_n(&timer_armed_lock, 0, __ATOMIC_RELEASE);
    Enable();
}

static inline BOOL timer_can_sleep(void)
{
    return !irqs_disabled();
}

/* under timer_list_lock */
static int timer_unlink(struct timer_list *t)
{
    if (!t->pending)
        return 0;
    Remove((struct Node *)&t->node);
    t->pending = 0;
    return 1;
}

/* under timer_list_lock */
static void timer_link(struct timer_list *t, unsigned long expires)
{
    t->expires = expires;
    AddTail((struct List *)&timer_armed, (struct Node *)&t->node);
    t->pending = 1;
}

static void timer_wake(void)
{
    if (timer_proc)
        Signal((struct Task *)timer_proc, timer_wake_mask);
}

static void timer_proc_main(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct timerequest *req = port ? CreateIORequest(port, sizeof(*req)) : NULL;
    BYTE wake = AllocSignal(-1);
    ULONG portsig;

    if (req) {
        *req = *timer_init_req;
        req->tr_node.io_Message.mn_ReplyPort = port;
    }
    if (wake >= 0)
        timer_wake_mask = 1UL << wake;
    Signal(timer_parent, timer_parent_ack);
    if (!req || wake < 0)
        return;
    portsig = 1UL << port->mp_SigBit;

    for (;;) {
        struct timer_list *t, *first;
        struct MinNode *n;
        unsigned long now = jiffies, expires = 0;
        BOOL fire;

        /* the earliest deadline, and whether it has passed */
        timer_list_lock();
        first = NULL;
        for (n = timer_armed.mlh_Head; n->mln_Succ; n = n->mln_Succ) {
            t = container_of(n, struct timer_list, node);
            if (!first || time_before(t->expires, first->expires))
                first = t;
        }
        if (first)
            expires = first->expires;
        fire = first && !time_after(expires, now);
        if (fire) {
            timer_unlink(first);
            timer_running = first;
        }
        timer_list_unlock();

        if (fire) {
            if (first->function)
                first->function(first);
            timer_list_lock();
            timer_running = NULL;
            timer_list_unlock();
            continue;
        }

        if (!first) {
            Wait(timer_wake_mask);
            continue;
        }

        {
            unsigned long usecs = jiffies_to_usecs(expires - now);
            ULONG sigs;

            req->tr_node.io_Command = TR_ADDREQUEST;
            req->tr_time.tv_secs = usecs / 1000000;
            req->tr_time.tv_micro = usecs % 1000000;
            SendIO((struct IORequest *)req);
            sigs = Wait(timer_wake_mask | portsig);
            if (!(sigs & portsig))
                AbortIO((struct IORequest *)req);
            WaitIO((struct IORequest *)req);
            SetSignal(0, portsig);
        }
    }
}

static BOOL timer_proc_start(void)
{
    BYTE ack;

    if (timer_proc)
        return TRUE;

    ObtainSemaphore(&timer_lock);
    if (!timer_proc && (ack = AllocSignal(-1)) >= 0) {
        timer_parent = FindTask(NULL);
        timer_parent_ack = 1UL << ack;
        timer_proc = CreateNewProcTags(
            NP_Name, (IPTR)"Amdgpu Timers",
            NP_Priority, 21,     /* kernel timer callbacks must outrank waiters, like the workqueues */
            NP_Entry, (IPTR)timer_proc_main,
            NP_StackSize, 64 * 1024,
            TAG_DONE);
        if (timer_proc)
            Wait(timer_parent_ack);
        FreeSignal(ack);
    }
    ReleaseSemaphore(&timer_lock);
    return timer_proc != NULL;
}

static int init_timerproc(void)
{
    timer_proc_start();
    return TRUE;
}

ADD2INIT(init_timerproc, 6);

void timer_setup(struct timer_list *t, void (*func)(struct timer_list *), unsigned int flags)
{
    memset(t, 0, sizeof(*t));
    t->function = func;
}

int timer_pending(const struct timer_list *t)
{
    return __atomic_load_n(&t->pending, __ATOMIC_ACQUIRE);
}

int mod_timer(struct timer_list *t, unsigned long expires)
{
    int was;

    timer_list_lock();
    was = timer_unlink(t);
    timer_link(t, expires);
    timer_list_unlock();
    timer_wake();
    return was;
}

void add_timer(struct timer_list *t)
{
    mod_timer(t, t->expires);
}

int del_timer(struct timer_list *t)
{
    int was;

    if (!timer_pending(t))
        return 0;
    timer_list_lock();
    was = timer_unlink(t);
    timer_list_unlock();
    return was;
}

/*
 * Returns with the timer neither queued nor running: a callback that is
 * running may re-arm its timer, so the check for a running callback and
 * the unlink are one step under the lock, repeated until it is not
 * running. From the timer process itself, or where it cannot sleep, it
 * only unlinks, as waiting there could never end.
 */
int del_timer_sync(struct timer_list *t)
{
    BOOL wait = FindTask(NULL) != (struct Task *)timer_proc && timer_can_sleep() && !amdgpu_compat_atomic;
    int was;

    for (;;) {
        timer_list_lock();
        if (!wait || timer_running != t) {
            was = timer_unlink(t);
            timer_list_unlock();
            return was;
        }
        timer_list_unlock();
        Delay(1);
    }
}

/* --- hrtimers, at timer granularity ------------------------------------ */

static void hrtimer_fire(struct timer_list *tl)
{
    struct hrtimer *timer = container_of(tl, struct hrtimer, timer);

    if (timer->function && timer->function(timer) == HRTIMER_RESTART)
        hrtimer_start(timer, timer->expires, HRTIMER_MODE_ABS);
}

void hrtimer_init(struct hrtimer *timer, int which_clock, enum hrtimer_mode mode)
{
    memset(timer, 0, sizeof(*timer));
    timer_setup(&timer->timer, hrtimer_fire, 0);
}

void hrtimer_setup(struct hrtimer *timer, enum hrtimer_restart (*function)(struct hrtimer *), int clock, enum hrtimer_mode mode)
{
    hrtimer_init(timer, clock, mode);
    timer->function = function;
}

void hrtimer_start(struct hrtimer *timer, ktime_t tim, const enum hrtimer_mode mode)
{
    ktime_t now = ktime_get();
    ktime_t expires = (mode & HRTIMER_MODE_REL) ? now + tim : tim;
    s64 delta_ns = expires > now ? expires - now : 0;

    timer->expires = expires;
    mod_timer(&timer->timer, jiffies + nsecs_to_jiffies(delta_ns));
}

int hrtimer_cancel(struct hrtimer *timer)
{
    return del_timer_sync(&timer->timer);
}

int hrtimer_try_to_cancel(struct hrtimer *timer)
{
    return del_timer(&timer->timer);
}
