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
 * All time comes from timer.device: jiffies and ktime are the system
 * clock, delays are TR_ADDREQUEST on a per-caller port, and kernel timers
 * are requests answered on the timer process, which runs the callback.
 */

struct Device *TimerBase = NULL;
static struct MsgPort *timer_init_port = NULL;
static struct timerequest *timer_init_req = NULL;

static struct MsgPort *timer_proc_port = NULL;
static struct Process *timer_proc = NULL;
static struct Task *timer_parent = NULL;
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
    GetSysTime(&tv);
    return (ktime_t)tv.tv_secs * NSEC_PER_SEC + (ktime_t)tv.tv_micro * NSEC_PER_USEC;
}

/* microseconds since boot, for the hidd's own diagnostics */
unsigned long nouveau_compat_usecs(void)
{
    return (unsigned long)(ktime_get() / NSEC_PER_USEC);
}

u64 compat_jiffies64(void)
{
    struct timeval tv;

    if (!TimerBase)
        return 0;
    GetSysTime(&tv);
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

    /* not SIGF_SINGLE: that bit carries the sleep/wake protocol */
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
int nouveau_compat_atomic;

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
    if (nouveau_compat_atomic)
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
    if (min < 20 || nouveau_compat_atomic)
        spin_wait(min);
    else
        timed_wait(min);
}


/*
 * TEMPORARY DIAGNOSTIC: how long a short timer.device wait really takes
 * on this platform. Reported once at notice level.
 */
void nouveau_compat_time_check(void)
{
    static const unsigned long tests[] = { 50, 200, 1000, 5000 };
    char line[160];
    int n = 0, i;

    for (i = 0; i < 4; i++) {
        ktime_t t0 = ktime_get();
        timed_wait(tests[i]);
        n += snprintf(line + n, sizeof(line) - n, " %luus->%lldus", tests[i], (long long)((ktime_get() - t0) / 1000));
    }
    printk(KERN_NOTICE "[nouveau] timer check:%s\n", line);
}

/* --- kernel timers ---------------------------------------------------- */

/*
 * The timer process owns every request: arming and cancelling are
 * commands sent to it, so that AbortIO/WaitIO always run in the task
 * that owns the reply port and no request is ever re-queued while a
 * stale reply is still in flight.
 */
enum { TIMER_CMD_MOD, TIMER_CMD_DEL };

struct timer_cmd {
    struct Message msg;
    int op;
    struct timer_list *t;
    unsigned long expires;
    int result;
};

static void timer_do_disarm(struct timer_list *t)
{
    if (!t->pending)
        return;
    AbortIO((struct IORequest *)&t->req);
    WaitIO((struct IORequest *)&t->req);
    t->pending = FALSE;
}

static void timer_do_arm(struct timer_list *t, unsigned long expires)
{
    unsigned long now = jiffies;
    unsigned long delay = time_after(expires, now) ? expires - now : 0;
    unsigned long usecs = jiffies_to_usecs(delay);

    t->expires = expires;
    t->req = *timer_init_req;
    t->req.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    t->req.tr_node.io_Message.mn_ReplyPort = timer_proc_port;
    t->req.tr_node.io_Command = TR_ADDREQUEST;
    t->req.tr_time.tv_secs = usecs / 1000000;
    t->req.tr_time.tv_micro = usecs % 1000000;
    t->pending = TRUE;
    SendIO((struct IORequest *)&t->req);
}

static struct MsgPort *timer_cmd_port = NULL;

static void timer_proc_main(void)
{
    struct Message *msg;
    ULONG sigs;

    timer_proc_port = CreateMsgPort();
    timer_cmd_port = CreateMsgPort();
    Signal(timer_parent, SIGF_SINGLE);
    if (!timer_proc_port || !timer_cmd_port)
        return;

    for (;;) {
        sigs = Wait((1UL << timer_proc_port->mp_SigBit) | (1UL << timer_cmd_port->mp_SigBit));

        while ((msg = GetMsg(timer_cmd_port))) {
            struct timer_cmd *cmd = (struct timer_cmd *)msg;
            struct timer_list *t = cmd->t;

            cmd->result = t->pending;
            timer_do_disarm(t);
            if (cmd->op == TIMER_CMD_MOD)
                timer_do_arm(t, cmd->expires);
            ReplyMsg(msg);
        }

        while ((msg = GetMsg(timer_proc_port))) {
            struct timerequest *treq = (struct timerequest *)msg;
            struct timer_list *t = container_of(treq, struct timer_list, req);

            if (t->pending) {
                t->pending = FALSE;
                if (t->function)
                    t->function(t);
            }
        }
        (void)sigs;
    }
}

static BOOL timer_proc_start(void)
{
    if (timer_proc)
        return TRUE;

    ObtainSemaphore(&timer_lock);
    if (!timer_proc) {
        timer_parent = FindTask(NULL);
        timer_proc = CreateNewProcTags(
            NP_Name, (IPTR)"Nouveau Timers",
            NP_Priority, 21,     /* kernel timer callbacks must outrank waiters, like the workqueues */
            NP_Entry, (IPTR)timer_proc_main,
            NP_StackSize, 64 * 1024,
            TAG_DONE);
        if (timer_proc)
            Wait(SIGF_SINGLE);
    }
    ReleaseSemaphore(&timer_lock);
    return timer_proc && timer_cmd_port;
}

static int timer_send_cmd(int op, struct timer_list *t, unsigned long expires)
{
    struct timer_cmd cmd;
    struct MsgPort port;

    if (!timer_proc_start())
        return 0;

    /* commands from the timer process itself act directly */
    if (FindTask(NULL) == (struct Task *)timer_proc) {
        int was = t->pending;
        timer_do_disarm(t);
        if (op == TIMER_CMD_MOD)
            timer_do_arm(t, expires);
        return was;
    }

    memset(&port, 0, sizeof(port));
    port.mp_Node.ln_Type = NT_MSGPORT;
    port.mp_Flags = PA_SIGNAL;
    port.mp_SigBit = AllocSignal(-1);
    port.mp_SigTask = FindTask(NULL);
    NEWLIST(&port.mp_MsgList);
    if (port.mp_SigBit == (UBYTE)-1)
        return 0;

    memset(&cmd, 0, sizeof(cmd));
    cmd.msg.mn_Node.ln_Type = NT_MESSAGE;
    cmd.msg.mn_ReplyPort = &port;
    cmd.msg.mn_Length = sizeof(cmd);
    cmd.op = op;
    cmd.t = t;
    cmd.expires = expires;

    PutMsg(timer_cmd_port, &cmd.msg);
    WaitPort(&port);
    GetMsg(&port);
    FreeSignal(port.mp_SigBit);
    return cmd.result;
}

void timer_setup(struct timer_list *t, void (*func)(struct timer_list *), unsigned int flags)
{
    memset(t, 0, sizeof(*t));
    t->function = func;
}

int timer_pending(const struct timer_list *t)
{
    return t->pending;
}

int mod_timer(struct timer_list *t, unsigned long expires)
{
    return timer_send_cmd(TIMER_CMD_MOD, t, expires);
}

void add_timer(struct timer_list *t)
{
    mod_timer(t, t->expires);
}

int del_timer(struct timer_list *t)
{
    if (!t->pending)
        return 0;
    return timer_send_cmd(TIMER_CMD_DEL, t, 0);
}

int del_timer_sync(struct timer_list *t)
{
    return del_timer(t);
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
