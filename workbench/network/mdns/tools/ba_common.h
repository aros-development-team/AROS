/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * ba_common.h - what the mDNS tools share: opening bonami.library, output
 * and string helpers without the C library, and waiting for the events of
 * one discovery operation.
 *
 * Discovery is queued (DISCOVERY.md): an operation's events are read with
 * ReadMDNSEvent() in the tool's own task, and a request lent to mdnsd with
 * RequestMDNSEvent() comes back when there is something to read. A BAWatch
 * keeps that request, its port and an optional time limit, so that a tool
 * sleeps until there are events, Ctrl-C or the time is up.
 */

#ifndef BONAMI_TOOLS_BA_COMMON_H
#define BONAMI_TOOLS_BA_COMMON_H

#include <exec/types.h>
#include <exec/ports.h>
#include <devices/timer.h>
#include <bonami/mdns.h>

/* The four numbers of an address in network byte order, for "%ld.%ld.%ld.%ld". */
#define BA_IP_ARGS(a) \
    (LONG)((UBYTE *)&(a))[0], (LONG)((UBYTE *)&(a))[1], \
    (LONG)((UBYTE *)&(a))[2], (LONG)((UBYTE *)&(a))[3]

/*------------------------------------------------------------------------*/
/* Library / engine lifecycle */

int  BAAppOpen(void);
void BAAppClose(void);
LONG BAAppOpenEngine(void);
void BAAppPrintMDNSError(CONST_STRPTR where, LONG code);

/*------------------------------------------------------------------------*/
/* Native output and string helpers (no stdio / string.h) */

void BAPrintf(CONST_STRPTR fmt, ...);
/* One line for a discovery event: action, name, type, address, port and
 * the text record's items. */
void BAPrintEvent(CONST_STRPTR action, const struct MDNSDiscoveryEvent *ev);
/* The text record, as it is on the wire, as its items with a space
 * between; a byte that cannot be printed is shown as '.'. */
void BAPrintTXT(const UBYTE *txt, LONG length);

LONG BAStrLen(const char *s);
void BAStrCopy(char *d, const char *s, LONG max);
void BAStrAppend(char *d, const char *s, LONG max);
BOOL BAStrEqual(const char *a, const char *b);
/* Parse one non-negative decimal LONG, rejecting empty, trailing and
 * overflowing input. */
BOOL BAParseNum(const char *s, LONG *value);
/* Join a NULL-terminated item vector for MDNS_TXTRecord, escaping item
 * spaces and backslashes. FALSE means the complete value does not fit or
 * one DNS-SD character-string exceeds its 255-byte wire limit. */
BOOL BAEncodeTXT(char *out, LONG max, char **items);

/*------------------------------------------------------------------------*/
/* Waiting for one operation's events */

#define BA_WAIT_EVENTS  0   /* read with ReadMDNSEvent() until NOTREADY */
#define BA_WAIT_BREAK   1   /* Ctrl-C */
#define BA_WAIT_TIME    2   /* the time given to BAWatchStart() is up */
#define BA_WAIT_END     3   /* the operation ended; bw_Result says why */

struct BAWatch
{
    APTR                  bw_Ref;       /* the operation */
    struct MsgPort       *bw_Port;      /* where the request comes back */
    struct MDNSEventRequest bw_Request;
    BOOL                  bw_Lent;      /* mdnsd holds the request */
    LONG                  bw_Result;    /* MDNSERR_* when it ended */
    struct MsgPort       *bw_TimerPort;
    struct timerequest   *bw_Timer;     /* only with a time limit */
    BOOL                  bw_TimerOn;
};

/* Begins to watch ref; seconds > 0 is how long to wait in all. 0 when it
 * could, or an MDNSERR_* code. */
LONG BAWatchStart(struct BAWatch *watch, APTR ref, LONG seconds);
/* Sleeps until one of the BA_WAIT_* happens. After BA_WAIT_EVENTS the
 * caller reads the events and calls it again: it lends the request anew. */
LONG BAWatchWait(struct BAWatch *watch);
/* After StopBrowse()/StopResolve() of its operation: takes the request
 * back, which mdnsd has replied by then, and frees what it holds. */
void BAWatchEnd(struct BAWatch *watch);

#endif /* BONAMI_TOOLS_BA_COMMON_H */
