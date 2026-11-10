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
#include <netinet/in.h>
#include <net/sana2arp.h>	/* for arp timeout */

#include <kern/amiga_includes.h>
#include <kern/amiga_time.h>
#include <kern/amiga_netdb.h>           /* netdb_defer_timer() */

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
                                  *arpTimer = NULL,
                                   *autoipTimer = NULL,
                                   *deferTimer = NULL,
                                    *protoSlowTimer = NULL,
                                     *protoFastTimer = NULL;

static BOOL can_send_timeouts = FALSE;

/*
 * The TCP retransmission deadlines (t_rexmt_at: RTO, loss probe) on a
 * request of their own, so that they do not wait for the next 50 ms tick:
 * it waits for the earliest of all connections'. tcp_timer_want() brings
 * it forward when a deadline comes before it; when it fires,
 * tcp_rexmt_due() runs the due ones and gives the next deadline, which it
 * is sent again for (timer_poll()), or half a second when none is set.
 */
extern u_int32_t tcp_ms(void);
extern u_int32_t tcp_rexmt_due(int *);
static struct timeoutRequest *tcpTimer = NULL;
static u_int32_t tcp_timer_at;          /* when it is due (tcp_ms()) */
static BOOL tcp_timer_out = FALSE;      /* sent and not back yet */

static void
tcp_timer_set(u_int32_t at)
{
    long d = (long)(at - tcp_ms());

    if(d < 1)
        d = 1;
    tcpTimer->timeout_timeval.tv_sec = d / 1000;
    tcpTimer->timeout_timeval.tv_usec = (d % 1000) * 1000;
    tcp_timer_at = at;
}

/* timer_poll() calls it, then sends the request again */
static void
tcp_timer_fire(void)
{
    u_int32_t next;
    int any;

    tcp_timer_out = FALSE;
    next = tcp_rexmt_due(&any);
    tcp_timer_set(any ? next : tcp_ms() + 500);
    tcp_timer_out = TRUE;
}

/* A retransmission deadline was set: bring the request forward to it. */
void
tcp_timer_want(u_int32_t at)
{
    if(tcpTimer == NULL || !tcp_timer_out || (int)(at - tcp_timer_at) >= 0)
        return;
    AbortIO((struct IORequest *)tcpTimer);
    WaitIO((struct IORequest *)tcpTimer);
    tcp_timer_set(at);
    sendTimeoutRequest(tcpTimer);
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
                    arpTimer = createTimeoutRequest(arptimer, ARPT_AGE, 0);
                    autoipTimer = createTimeoutRequest(autoip_timer, 1, 0);
                    deferTimer = createTimeoutRequest(netdb_defer_timer, NETDB_DEFER_SECS, 0);
                    protoSlowTimer = createTimeoutRequest(pfslowtimo, 0, 1000000 / PR_SLOWHZ);
                    protoFastTimer = createTimeoutRequest(pffasttimo, 0, 1000000 / PR_FASTHZ);
                    tcpTimer = createTimeoutRequest(tcp_timer_fire, 0, 500000);
                    if(protoFastTimer && protoSlowTimer && arpTimer && ifTimer && autoipTimer && deferTimer &&
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

    tcp_timer_out = FALSE;
    if(tcpTimer)
        deleteTimeoutRequest(tcpTimer);
    tcpTimer = NULL;
    if(protoFastTimer)
        deleteTimeoutRequest(protoFastTimer);
    if(protoSlowTimer)
        deleteTimeoutRequest(protoSlowTimer);
    if(deferTimer)
        deleteTimeoutRequest(deferTimer);
    if(autoipTimer)
        deleteTimeoutRequest(autoipTimer);
    if(arpTimer)
        deleteTimeoutRequest(arpTimer);
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
        sendTimeoutRequest(ifTimer);
        sendTimeoutRequest(arpTimer);
        sendTimeoutRequest(autoipTimer);
        sendTimeoutRequest(deferTimer);
        sendTimeoutRequest(protoSlowTimer);
        sendTimeoutRequest(protoFastTimer);
        tcp_timer_set(tcp_ms() + 500);
        sendTimeoutRequest(tcpTimer);
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
        AbortIO((struct IORequest *)tr);
        WaitIO((struct IORequest *)tr);
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
    if(timerReply = (struct timeoutRequest *)GetMsg(timerport)) {
        /*
         * enter softclock interrupt level
         */
        spl_t s = splsoftclock();
        /*
         * handle the timeout
         */
        handleTimeoutRequest(timerReply);
        /*
         * restart timeout request
         */
        sendTimeoutRequest(timerReply);
        /*
         * restore previous interrupt level
         */
        splx(s);

        return TRUE;
    }

    return FALSE;
}
