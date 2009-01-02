/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
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
  *protoSlowTimer = NULL, 
  *protoFastTimer = NULL;

static BOOL can_send_timeouts = FALSE; 

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
  if (timerport != NULL)
    return TRUE;

  /*
   * allocate and initialize the timer message reply port
   */
  timerport = CreateMsgPort();
  if (timerport != NULL) {
    /*
     * allocate and initialize the template message structure
     */
    timerIORequest = (struct timerequest *)
      CreateIORequest(timerport, sizeof(struct timerequest));
    if (timerIORequest != NULL) {
      error = OpenDevice(TIMERNAME, UNIT_VBLANK, 
			 (struct IORequest *)timerIORequest, 0);
      if (error == 0) {
	/*
	 * Make sure that we got at least V36 timer, since we use some
	 * functions defined only in V36 and later.
	 */
	if ((timerIORequest->tr_node.io_Device)->dd_Library.lib_Version >= 36) {
	  /*
	   * initialize TimerBase from timerIORequest
	   */
	  TimerBase = (struct Library *)timerIORequest->tr_node.io_Device;
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
	  protoSlowTimer = createTimeoutRequest(pfslowtimo, 0, 1000000 / PR_SLOWHZ); 
	  protoFastTimer = createTimeoutRequest(pffasttimo, 0, 1000000 / PR_FASTHZ); 
	  if (protoFastTimer && protoSlowTimer && arpTimer && ifTimer) {
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

  if (protoFastTimer)
    deleteTimeoutRequest(protoFastTimer);
  if (protoSlowTimer)
    deleteTimeoutRequest(protoSlowTimer);
  if (arpTimer)
    deleteTimeoutRequest(arpTimer);
  if (ifTimer)
    deleteTimeoutRequest(ifTimer);

  if (timerIORequest) {
    TimerBase = NULL;
    if (timerIORequest->tr_node.io_Device != NULL)
      CloseDevice((struct IORequest *)timerIORequest);
    DeleteIORequest((struct IORequest *)timerIORequest);
    timerIORequest = NULL;
  }
  if (timerport) {
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
  if (can_send_timeouts != FALSE) {
    /*
     * Start timeout requests
     */
    sendTimeoutRequest(ifTimer);
    sendTimeoutRequest(arpTimer);
    sendTimeoutRequest(protoSlowTimer);
    sendTimeoutRequest(protoFastTimer);

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
  if (micros >= 1000000) {
    log(LOG_ERR, "More than 1000000 microseconds in initTimeoutRequest()\n");
    return NULL;
  }
#endif

  /*
   * allocate IO request
   */
  tr = (struct timeoutRequest *)CreateIORequest(timerport, sizeof(*tr));
  if (tr == NULL)
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
  if (((struct Node *)tr)->ln_Type != NT_UNKNOWN) {
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
  if (timerReply = (struct timeoutRequest *)GetMsg(timerport)) {
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
