/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/synch.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>	/* for protocol timeouts */
#include <net/if.h>		/* for if timeout */
#include <sys/kernel.h>		/* hz, tick */
#include <net/netmodule.h>	/* netperiodic_add() */

#include <kern/amiga_includes.h>
#include <kern/amiga_time.h>
#include <kern/amiga_netdb.h>           /* netdb_defer_timer() */
#include <kern/netdiag.h>

/*
 * include prototypes for timeout functions
 */
#include <net/if_protos.h>              /* if_slowtimo() */
#include <kern/uipc_domain_protos.h>    /* pfslowtimo(), pffasttimo() */

/*
 * Global timer base pointer used throughout the code.
 * Commodore says that same base should not be used by different task,
 * so API users should have their own base pointer.
 */
#ifdef __MORPHOS__
struct Library    *TimerBase = NULL;
#else
struct Device     *TimerBase = NULL;
#endif

static struct MsgPort     *timerport = NULL;	 /* Timer message reply port */
static struct timerequest *timerIORequest = NULL; /* template IORequest */

/*
 * timeoutrequest pointers for all the timeouts
 */
static struct timeoutRequest *ifTimer = NULL,
                                   *deferTimer = NULL,
                                    *protoSlowTimer = NULL,
                                     *protoFastTimer = NULL;

static BOOL can_send_timeouts = FALSE;

/*
 * The periodic calls of the modules (netperiodic_add(), net/netmodule.h):
 * a protocol's own timeouts - address resolution ageing and the like - are
 * registered by the module that has them, not created here by name. One
 * request each, sent with the others by timer_send() or, for a module
 * started later, at once.
 */
#define PERIODIC_MAX 8
static struct timeoutRequest *periodicTimers[PERIODIC_MAX];
static BOOL timers_running = FALSE;     /* timer_send() has sent the requests */

/* netdiag.tm_last[] has a slot for each of them (kern/netdiag.h) */
typedef char netdiag_timers_fit[(NETDIAG_TIMERS == NDT_PERIODIC + PERIODIC_MAX) ? 1 : -1];

/*
 * Every request goes out through here and comes back through timer_poll()
 * or an AbortIO()/WaitIO(), so that netdiag can tell a request that went
 * out twice (tm_sent - tm_replied - tm_collected above tm_reqs) or never
 * came back (its tm_last[] frozen).
 */
static inline void
timer_sendreq(struct timeoutRequest *tr)
{
    netdiag.tm_sent++;
    sendTimeoutRequest(tr);
}

/* abort a request that may be out and take it back */
static inline void
timer_collect(struct timeoutRequest *tr)
{
    AbortIO((struct IORequest *)tr);
    WaitIO((struct IORequest *)tr);
    netdiag.tm_collected++;
}

/*
 * The core's coarse clock (net/netmodule.h): one more with every slow
 * timeout of the protocols.
 */
unsigned long net_slowticks = 0;

static void
proto_slowtimo(void)
{
    net_slowticks++;
    pfslowtimo();
}

/*
 * The TCP retransmission deadlines (t_rexmt_at: RTO, loss probe) on a
 * request of their own, so that they do not wait for the next 50 ms tick:
 * it waits for the earliest of all connections'. tcp_timer_want() asks for
 * it to be brought forward when a deadline comes before it; when it fires,
 * the due ones are run and the next deadline is given, which it is sent
 * again for (timer_poll()), or half a second when none is set.
 *
 * Only the task that owns the timer port - the stack's - ever aborts,
 * collects or sends the request. exec lets nobody else: WaitIO() takes a
 * reply off the owner's port and waits on the caller's own signals, the
 * reply signals the owner, and whether it was right depended on where the
 * owner happened to be in timer_poll(). Another task records the deadline
 * and signals the owner (tcp_timer_rearm), which does the rest at its next
 * timer_poll().
 *
 * The core does not name TCP for this: the module that has the deadlines
 * registers how its clock is read and how the due ones are run (struct
 * netdeadline, net/netmodule.h: TCP's tcp_ms() and tcp_rexmt_due()), and
 * `deadline' is that registration. While there is none - no transports in
 * the running stack, or not started yet - the request goes round every
 * half second and does nothing.
 */
static const struct netdeadline *deadline = NULL;
static struct timeoutRequest *tcpTimer = NULL;
static u_int32_t tcp_timer_at;          /* when it is due, or is wanted to be
                                           (on nd_now's clock) */
static BOOL tcp_timer_out = FALSE;      /* sent and not back yet */
static BOOL tcp_timer_rearm = FALSE;    /* another task wants it at tcp_timer_at,
                                           and the owner has not seen to it yet */

/* only with a registration */
static void
tcp_timer_set(u_int32_t at)
{
    long d = (long)(at - (*deadline->nd_now)());

    if(d < 1)
        d = 1;
    tcpTimer->timeout_timeval.tv_sec = d / 1000;
    tcpTimer->timeout_timeval.tv_usec = (d % 1000) * 1000;
    tcp_timer_at = at;
}

/*
 * Half a second from now: when no deadline is set, and when nobody is
 * registered (the interval alone is set then - there is no clock to say
 * when that is, and nobody who could ask: see tcp_timer_want()).
 */
static void
tcp_timer_idle(void)
{
    if(deadline != NULL)
        tcp_timer_set((*deadline->nd_now)() + 500);
    else {
        tcpTimer->timeout_timeval.tv_sec = 0;
        tcpTimer->timeout_timeval.tv_usec = 500000;
    }
}

/* timer_poll() calls it, then sends the request again */
static void
tcp_timer_fire(void)
{
    u_int32_t next;
    int any = 0;

    netdiag.dl_fire++;
    tcp_timer_out = FALSE;
    tcp_timer_rearm = FALSE;            /* every deadline is looked at below */
    if(deadline != NULL)
        next = (*deadline->nd_due)(&any);
    if(any)
        tcp_timer_set(next);
    else
        tcp_timer_idle();
    tcp_timer_out = TRUE;
}

/*
 * The module that has the deadlines makes itself known, and takes itself
 * back. With the stack fenced, as modules are started and stopped; at
 * splsoftclock the request is pending or replied and still on the port,
 * never half-way through timer_poll().
 *
 * Registered while the request is out (a module started after the timers):
 * the request is left as it is - it comes back within half a second - and
 * tcp_timer_at is set to no earlier than that, so that a deadline before
 * it brings the request forward (tcp_timer_want()) and one after it finds
 * the request back in time.
 */
int
netdeadline_register(const struct netdeadline *nd)
{
    spl_t s;

    if(nd == NULL || nd->nd_now == NULL || nd->nd_due == NULL)
        return (EINVAL);
    if(deadline != NULL)
        return (deadline == nd ? 0 : EEXIST);
    s = splsoftclock();
    deadline = nd;
    tcp_timer_at = (*nd->nd_now)() + 500;
    splx(s);
    return (0);
}

int
netdeadline_unregister(const struct netdeadline *nd)
{
    spl_t s;

    if(nd == NULL || deadline != nd)
        return (ENOENT);
    /* the request, if it is out, comes back by itself and then idles */
    s = splsoftclock();
    deadline = NULL;
    tcp_timer_rearm = FALSE;
    splx(s);
    return (0);
}

/*
 * Send the request again for tcp_timer_at, which is before the time it is
 * out for. ONLY in the task that owns the timer port, at splsoftclock or
 * above, with the request out (tcp_timer_out) and a registration.
 *
 * A request that is back already is left on the port: timer_poll() takes
 * it next and tcp_timer_fire() looks at every deadline. One that comes back
 * between the test and the AbortIO() is collected by the WaitIO() and sent
 * again for tcp_timer_at, a deadline that has not been run yet.
 */
static void
tcp_timer_bring_forward(void)
{
    if(CheckIO((struct IORequest *)tcpTimer) != NULL) {
        netdiag.dl_back++;
        return;
    }
    netdiag.dl_forward++;
    timer_collect(tcpTimer);
    tcp_timer_set(tcp_timer_at);
    timer_sendreq(tcpTimer);
}

/*
 * A retransmission deadline was set: have the request brought forward to
 * it. Called at splnet from whatever task is in the stack - an
 * application's, inside send(), as well as the stack's own.
 *
 * The stack's task does it at once. Any other task only records the
 * deadline and signals the stack's task, whose next timer_poll() sends the
 * request again; that is as soon as the caller lets go of the stack (the
 * two run at the same priority during a call) and in no case later than
 * the next timeout of any kind, 50 ms. One signal for any number of
 * deadlines until then - and no more than before, when the AbortIO() made
 * here replied to the stack's port and woke its task for nothing.
 *
 * (The request was once aborted, collected and sent again from here in the
 * caller's task. With timer_poll() as it then was - reply taken first, lock
 * after - a caller in that gap removed a message already taken and sent a
 * request that timer_poll() then sent again: queued twice in timer.device,
 * and the stack's timers stopped for good.)
 */
void
tcp_timer_want(u_int32_t at)
{
    /* (without a registration there is nobody whose deadline this is) */
    if(tcpTimer == NULL || deadline == NULL || !tcp_timer_out ||
            (int)(at - tcp_timer_at) >= 0)
        return;
    tcp_timer_at = at;          /* a later deadline asks for nothing more */
    netdiag.dl_want++;
    if(FindTask(NULL) == timerport->mp_SigTask) {
        tcp_timer_rearm = FALSE;
        tcp_timer_bring_forward();
    } else if(!tcp_timer_rearm) {
        tcp_timer_rearm = TRUE;
        netdiag.dl_signal++;
        Signal(timerport->mp_SigTask, 1UL << timerport->mp_SigBit);
    }
}

/*
 * Initialize the timer. This MUST be called before any Timer functions are
 * used (including get_time() and microtime() which use GetSysTime())
 * (see sys/time.h).
 *
 * timerIORequest is used as a template from which all needed timer
 * IO messages are copied. The command field of the request is initialized to
 * TR_ADDREQUEST. Requests node type is initialized to NT_UNKNOWN for us
 * to be able to recognize if it has been used.
 *
 * Note that we need to check manually the version of the opened device. The
 * version number must be at least 36 since we use the GetSysTime() function
 * which is not defined in earlier versions.
 *
 * This initializes all the needed timeoutrequests too.
 */
ULONG
timer_init(void)
{
    LONG error;
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_timer.c) timer_init()\n"));
#endif

    /*
     * Return success if already initialized
     */
    if(timerport != NULL)
        return TRUE;

    /*
     * allocate and initialize the timer message reply port
     */
    timerport = CreateMsgPort();
    if(timerport != NULL) {
        /*
         * allocate and initialize the template message structure
         */
        timerIORequest = (struct timerequest *)
                         CreateIORequest(timerport, sizeof(struct timerequest));
        if(timerIORequest != NULL) {
            /*
             * UNIT_MICROHZ: the protocol timeouts run at their periods.
             * UNIT_VBLANK rounded them to the vertical blank, so the 50 ms
             * fast timeout (PR_FASTHZ) was 60 ms on a 50 Hz display, 55 to
             * 70 under load, and the delayed ACKs and the retransmission
             * deadlines with it.
             */
            error = OpenDevice(TIMERNAME, UNIT_MICROHZ,
                               (struct IORequest *)timerIORequest, 0);
            if(error == 0) {
                /*
                 * Make sure that we got at least V36 timer, since we use some
                 * functions defined only in V36 and later.
                 */
                if((timerIORequest->tr_node.io_Device)->dd_Library.lib_Version >= 36) {
                    /*
                     * initialize TimerBase from timerIORequest
                     */
                    TimerBase = timerIORequest->tr_node.io_Device;
                    /*
                     * Initialize some fields of the IO request to common values
                     */
                    timerIORequest->tr_node.io_Command = TR_ADDREQUEST;
                    /*
                     * NT_UNKNOWN means unused, too (see note on exec/nodes.h)
                     */
                    timerIORequest->tr_node.io_Message.mn_Node.ln_Type = NT_UNKNOWN;

                    /*
                     * create timeout requests for all timeouts;
                     */
                    ifTimer = createTimeoutRequest(if_slowtimo, 1 / IFNET_SLOWHZ, 0);
                    deferTimer = createTimeoutRequest(netdb_defer_timer, NETDB_DEFER_SECS, 0);
                    protoSlowTimer = createTimeoutRequest(proto_slowtimo, 0, 1000000 / PR_SLOWHZ);
                    protoFastTimer = createTimeoutRequest(pffasttimo, 0, 1000000 / PR_FASTHZ);
                    tcpTimer = createTimeoutRequest(tcp_timer_fire, 0, 500000);
                    if(protoFastTimer && protoSlowTimer && ifTimer && deferTimer &&
                            tcpTimer) {
                        can_send_timeouts = TRUE;
                        return (ULONG)(1 << timerport->mp_SigBit);
                    }
                }
            }
        }
    }
    /*
     * clean all in case of any error
     */
    timer_deinit();

    return (0);
}

/*
 * Deinitialize the timer.
 * The requests are cancelled first.
 */
void
timer_deinit(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_timer.c) timer_deinit()\n"));
#endif
    can_send_timeouts = FALSE;
    timers_running = FALSE;

    /* what the modules registered and did not take back */
    {
        int i;

        for(i = 0; i < PERIODIC_MAX; i++)
            if(periodicTimers[i]) {
                deleteTimeoutRequest(periodicTimers[i]);
                periodicTimers[i] = NULL;
            }
    }

    tcp_timer_out = FALSE;
    tcp_timer_rearm = FALSE;
    if(tcpTimer)
        deleteTimeoutRequest(tcpTimer);
    tcpTimer = NULL;
    deadline = NULL;            /* a module's: it may leave memory after this */
    if(protoFastTimer)
        deleteTimeoutRequest(protoFastTimer);
    if(protoSlowTimer)
        deleteTimeoutRequest(protoSlowTimer);
    if(deferTimer)
        deleteTimeoutRequest(deferTimer);
    if(ifTimer)
        deleteTimeoutRequest(ifTimer);

    if(timerIORequest) {
        TimerBase = NULL;
        if(timerIORequest->tr_node.io_Device != NULL)
            CloseDevice((struct IORequest *)timerIORequest);
        DeleteIORequest((struct IORequest *)timerIORequest);
        timerIORequest = NULL;
    }
    if(timerport) {
        DeleteMsgPort(timerport);
        timerport = NULL;
    }
}

/*
 * Function to send all the timeout requests when everything is initialized
 * DON'T even try to call this function otherwise!
 */
void
timer_send(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_timer.c) timer_send()\n"));
#endif
    if(can_send_timeouts != FALSE) {
        /*
         * Start timeout requests
         */
        int i;

        netdiag.tm_reqs += 5;
        timer_sendreq(ifTimer);
        for(i = 0; i < PERIODIC_MAX; i++)
            if(periodicTimers[i])
                timer_sendreq(periodicTimers[i]);
        timers_running = TRUE;
        timer_sendreq(deferTimer);
        timer_sendreq(protoSlowTimer);
        timer_sendreq(protoFastTimer);
        tcp_timer_idle();
        timer_sendreq(tcpTimer);
        tcp_timer_out = TRUE;

        can_send_timeouts = FALSE;
    }
}

/*
 * functions to create new timeoutRequest (after timer is initialized!)
 */
struct timeoutRequest *
createTimeoutRequest(TimerCallback_t fun,
                     ULONG seconds, ULONG micros)
{
    struct timeoutRequest *tr;
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_timer.c) createTimeoutRequest()\n"));
#endif

#if DIAGNOSTIC
    /*
     * sanity check the micros value
     */
    if(micros >= 1000000) {
        log(LOG_ERR, "More than 1000000 microseconds in initTimeoutRequest()\n");
        return NULL;
    }
#endif

    /*
     * allocate IO request
     */
    tr = (struct timeoutRequest *)CreateIORequest(timerport, sizeof(*tr));
    if(tr == NULL)
        return NULL;

    /*
     * copy initial values from the initialized timerrequest
     */

    /* Node */
    tr->timeout_request.tr_node.io_Message.mn_Node.ln_Type =
        timerIORequest->tr_node.io_Message.mn_Node.ln_Type;
    tr->timeout_request.tr_node.io_Message.mn_Node.ln_Pri =
        timerIORequest->tr_node.io_Message.mn_Node.ln_Pri;
    tr->timeout_request.tr_node.io_Message.mn_Node.ln_Name =
        timerIORequest->tr_node.io_Message.mn_Node.ln_Name;

    /* Message */
    tr->timeout_request.tr_node.io_Message.mn_ReplyPort =
        timerIORequest->tr_node.io_Message.mn_ReplyPort;

    /* IORequest */
    tr->timeout_request.tr_node.io_Device = timerIORequest->tr_node.io_Device;
    tr->timeout_request.tr_node.io_Unit = timerIORequest->tr_node.io_Unit;
    tr->timeout_request.tr_node.io_Command = timerIORequest->tr_node.io_Command;
    tr->timeout_request.tr_node.io_Flags = timerIORequest->tr_node.io_Flags;

    /*
     * set our own fields
     */
    tr->timeout_timeval.tv_secs = seconds;
    tr->timeout_timeval.tv_micro = micros;
    tr->timeout_function = fun;

    return tr;
}

void
deleteTimeoutRequest(struct timeoutRequest *tr)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_timer.c) deleteTimeoutRequest()\n"));
#endif
    /*
     * Abort the request if ever used
     */
    if(((struct Node *)tr)->ln_Type != NT_UNKNOWN) {
        timer_collect(tr);
        /*
         * Make sure the signal gets cleared
         */
        SetSignal(0, 1 << timerport->mp_SigBit);
    }
    /*
     * free the request
     */
    DeleteIORequest((struct IORequest *)tr);
}

/*
 * A periodic call for a module (net/netmodule.h).
 */
void *
netperiodic_add(void (*fn)(void), unsigned long ticks)
{
    struct timeoutRequest *tr;
    spl_t s;
    int i;

    if(fn == NULL || ticks == 0 || timerport == NULL)
        return (NULL);
    s = splsoftclock();
    for(i = 0; i < PERIODIC_MAX; i++)
        if(periodicTimers[i] == NULL)
            break;
    if(i == PERIODIC_MAX ||
            (tr = createTimeoutRequest(fn, ticks / hz, (ticks % hz) * tick)) == NULL) {
        splx(s);
        return (NULL);
    }
    periodicTimers[i] = tr;
    netdiag.tm_reqs++;
    if(timers_running)
        timer_sendreq(tr);
    splx(s);
    return (tr);
}

/*
 * Take a periodic call back. At splsoftclock the request is pending in
 * timer.device or replied and still on the port, never half-way through
 * timer_poll() (which keeps this level from before it takes a reply until
 * it has sent the request again): it can be aborted and collected here.
 * The port's signal is left alone - other replies may be waiting on it -
 * so the stack's task may be woken once for nothing.
 */
void
netperiodic_remove(void *handle)
{
    struct timeoutRequest *tr = handle;
    spl_t s;
    int i;

    if(tr == NULL)
        return;
    s = splsoftclock();
    for(i = 0; i < PERIODIC_MAX; i++)
        if(periodicTimers[i] == tr) {
            periodicTimers[i] = NULL;
            netdiag.tm_reqs--;
            if(((struct Node *)tr)->ln_Type != NT_UNKNOWN)
                timer_collect(tr);
            DeleteIORequest((struct IORequest *)tr);
            break;
        }
    splx(s);
}

/*
 * Milliseconds since midnight UT: the time stamp of ICMP and of the IP
 * time stamp option (RFC 792), and what the TCP trace buffer records. In
 * host byte order.
 */
u_long
time_msofday(void)
{
    struct timeval atv;

    GetSysTime(&atv);
    return ((atv.tv_sec % (24 * 60 * 60)) * 1000 + atv.tv_usec / 1000);
}

/* which slot of netdiag.tm_last[] a request is (one reply in 50 ms: the fast timer first) */
static int
timer_slot(struct timeoutRequest *tr)
{
    int i;

    if(tr == protoFastTimer)
        return NDT_FAST;
    if(tr == tcpTimer)
        return NDT_DEADLINE;
    if(tr == protoSlowTimer)
        return NDT_SLOW;
    if(tr == ifTimer)
        return NDT_IF;
    if(tr == deferTimer)
        return NDT_DEFER;
    for(i = 0; i < PERIODIC_MAX; i++)
        if(tr == periodicTimers[i])
            return NDT_PERIODIC + i;
    return -1;
}

BOOL timer_poll(VOID)
{
    struct timeoutRequest *timerReply;
    /* Uncomment the following lines to get debug - however please note: this func runs often */
//#if defined(__AROS__)
//D(bug("[AROSTCP](amiga_timer.c) timer_poll()\n"));
//#endif
    /*
     * Get all messages from the timer reply port.
     */
    /*
     * Enter softclock level BEFORE taking a reply off the port, and keep it
     * until the request has been sent again: a request is then never in
     * between - taken here but not yet handled - while another task is in
     * the stack (netperiodic_remove() relies on it).
     */
    spl_t s = splsoftclock();

    /*
     * A deadline before the one the deadline request is out for, asked for
     * by another task (tcp_timer_want()). Its signal alone brings us here,
     * with no reply on the port.
     */
    if(tcp_timer_rearm) {
        tcp_timer_rearm = FALSE;
        if(tcpTimer != NULL && deadline != NULL && tcp_timer_out)
            tcp_timer_bring_forward();
    }

    if(timerReply = (struct timeoutRequest *)GetMsg(timerport)) {
        int slot = timer_slot(timerReply);

        netdiag.tm_replied++;
        if(slot >= 0)
            netdiag.tm_last[slot] = netdiag.ml_iter;
        /*
         * handle the timeout
         */
        handleTimeoutRequest(timerReply);
        /*
         * restart timeout request
         */
        timer_sendreq(timerReply);
        /*
         * restore previous interrupt level
         */
        splx(s);

        return TRUE;
    }
    netdiag.tm_empty++;
    splx(s);

    return FALSE;
}
