/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * sd_common.h - what the mDNS tools share: opening dnssd.library, output
 * and string helpers without the C library, a session and the wait for its
 * events, and a browse that resolves what it finds.
 *
 * A session has a message port of the tool's own: mdnsd puts a message on
 * it when the session has events, and the tool reads them in its own task
 * (DNSSD_ReadEvent()). A SDSession keeps the session, its port, an optional
 * time limit and a buffer for the events, so that a tool sleeps until there
 * are events, Ctrl-C or the time is up.
 */

#ifndef MDNS_TOOLS_SD_COMMON_H
#define MDNS_TOOLS_SD_COMMON_H

#include <exec/types.h>
#include <exec/ports.h>
#include <devices/timer.h>
#include <dnssd/dnssd.h>

/* The four numbers of an address in network byte order, for "%ld.%ld.%ld.%ld". */
#define SD_IP_ARGS(a) \
    (LONG)((UBYTE *)&(a))[0], (LONG)((UBYTE *)&(a))[1], \
    (LONG)((UBYTE *)&(a))[2], (LONG)((UBYTE *)&(a))[3]

/* What a browse for the types on the link asks for (RFC 6763 section 9). */
#define SD_TYPE_SERVICES    "_services._dns-sd._udp"

/*------------------------------------------------------------------------*/
/* The library */

extern struct Library *DNSSDBase;

int  SDAppOpen(void);
void SDAppClose(void);
void SDAppPrintError(CONST_STRPTR where, LONG code);

/*------------------------------------------------------------------------*/
/* Native output and string helpers (no stdio / string.h) */

void SDPrintf(CONST_STRPTR fmt, ...);
/* TXT data, as it is on the wire, as its items with a space between; a
 * space or backslash in an item after a backslash, a byte that cannot be
 * printed as \xNN (binary values are allowed, RFC 6763 section 6.5). */
void SDPrintTXT(const UBYTE *txt, LONG length);
/* An address as text: "192.168.1.2", "2001:db8::1", "fe80::1%2" with the
 * interface of a link-local one. */
void SDAddressText(const struct DNSSDAddress *a, char *text, LONG max);
/* "192.168.1.2", "2001:db8::1" or "fe80::1%2" as an address; FALSE if it
 * is not one. */
BOOL SDParseAddress(const char *text, struct DNSSDAddress *a);

LONG SDStrLen(const char *s);
void SDStrCopy(char *d, const char *s, LONG max);
void SDStrAppend(char *d, const char *s, LONG max);
BOOL SDStrEqual(const char *a, const char *b);
/* Parse one non-negative decimal LONG, rejecting empty, trailing and
 * overflowing input. */
BOOL SDParseNum(const char *s, LONG *value);
/* A NULL-terminated vector of "key=value" items as TXT data, each a
 * string after its length (RFC 6763 section 6); in an item "\xNN" is the
 * byte NN and "\\" a backslash, so that values may be binary. Its length,
 * or -1 when it does not fit in max, an item is longer than 255 bytes or an
 * escape is not one. */
LONG SDTXTFromItems(UBYTE *txt, LONG max, char **items);
/* The items as one line, a space between them, for showing. */
void SDItemsText(char *out, LONG max, char **items);

/*------------------------------------------------------------------------*/
/* A session and its events */

#define SD_WAIT_EVENTS  0   /* read with SDSessionRead() until it says none */
#define SD_WAIT_BREAK   1   /* Ctrl-C */
#define SD_WAIT_TIME    2   /* the time given to SDSessionOpen() is up */

struct SDSession
{
    DNSSDSessionRef       bs_Session;
    struct MsgPort       *bs_Port;      /* where mdnsd tells of events */
    struct MsgPort       *bs_TimerPort;
    struct timerequest   *bs_Timer;     /* only with a time limit */
    BOOL                  bs_TimerOn;
    struct DNSSDEvent    *bs_Event;     /* the last event read */
    ULONG                 bs_Size;      /* bytes there */
};

/* Opens a session; seconds > 0 is how long to wait in all. DNSSD_OK or
 * DNSSD_ERR_*. */
LONG SDSessionOpen(struct SDSession *s, LONG seconds);
/* Sleeps until one of the SD_WAIT_* happens. */
LONG SDSessionWait(struct SDSession *s);
/* The next event, valid until the next call; NULL when there is none
 * (*rc DNSSD_ERR_EMPTY) or the session failed (*rc says why). */
const struct DNSSDEvent *SDSessionRead(struct SDSession *s, LONG *rc);
/* Ends the session (its operations with it) and frees what it holds. */
void SDSessionClose(struct SDSession *s);

/* A part of an event, or NULL when it has none. */
const UBYTE *SDEventPart(const struct DNSSDEvent *e, const struct DNSSDRef *r);
/* A name of an event as text ("_http._tcp"), or "" if it has none. */
void SDEventName(const struct DNSSDEvent *e, const struct DNSSDRef *r,
    char *text, LONG max);
/* The first IPv4 address of an event, in network byte order; 0 if none. */
ULONG SDEventIPv4(const struct DNSSDEvent *e);
/* An event's addresses, at most max of them, IPv4 first; how many. */
ULONG SDEventAddresses(const struct DNSSDEvent *e, struct DNSSDAddress *out,
    ULONG max);

/*------------------------------------------------------------------------*/
/* A browse that resolves what it finds */

#define SD_TXT_SHOWN    1300    /* bytes of TXT data a tool keeps */
#define SD_ADDRESSES    8       /* addresses of a host a tool keeps */

/* An instance, resolved: what a tool shows of it. */
struct SDService
{
    char    sv_Name[64];        /* the instance's own label */
    char    sv_Type[256];       /* "_http._tcp" */
    char    sv_Target[256];     /* its host, "printer.local" */
    ULONG   sv_Interface;
    ULONG   sv_Address;         /* its first IPv4, network byte order; 0 if none */
    UWORD   sv_AddressCount;    /* every address it has, IPv4 and IPv6 */
    struct DNSSDAddress sv_Addresses[SD_ADDRESSES];
    UWORD   sv_Port;
    UWORD   sv_TXTLength;
    UBYTE   sv_TXT[SD_TXT_SHOWN];
};

#define SD_FOUND_NONE       0   /* nothing to tell */
#define SD_FOUND_ADD        1   /* an instance, resolved */
#define SD_FOUND_REMOVE     2   /* an instance told before is gone */
#define SD_FOUND_TYPE       3   /* type enumeration: a type on the link */
#define SD_FOUND_NOTYPE     4   /* that type is gone */
#define SD_FOUND_END        5   /* the browse ended: *rc says why */
#define SD_FOUND_UPDATE     6   /* an instance told before says something else
                                 * now: its addresses, port, host or TXT */

/*
 * What a browse finds is resolved, and followed: a change of an instance's
 * addresses, port, host or TXT is told again (SD_FOUND_UPDATE), until it
 * goes. A resolve that follows runs as long as the instance is there, and
 * mdnsd allows a session so many operations at a time; past
 * SD_FINDER_FOLLOWED instances the others are resolved once each, a few at
 * a time, and followed when one of those followed goes: what changed in
 * one while it was not followed is told then (SD_FOUND_UPDATE), late.
 */
#define SD_FINDER_INSTANCES 64  /* instances kept track of */
#define SD_FINDER_FOLLOWED  16  /* followed at a time */
#define SD_FINDER_RESOLVES  4   /* resolved once at a time; the others wait */
/* The browse, the followed and those resolved once: 1 + 16 + 4 = 21
 * operations at most, within the 24 mdnsd allows a session on the 68k (an
 * operation that ends or is cancelled gives its room back at once). One
 * mdnsd refuses (DNSSD_ERR_BUSY) waits for the next room. */

struct SDFinderInstance;

struct SDFinder
{
    struct SDSession         *bf_Session;
    DNSSDOperationRef         bf_Browse;
    ULONG                     bf_BrowseID;
    BOOL                      bf_Types;     /* a type enumeration */
    struct SDFinderInstance  *bf_Instances[SD_FINDER_INSTANCES];
};

/* Browses type ("_http._tcp", "_printer._sub._ipp._tcp", or
 * SD_TYPE_SERVICES for the types on the link). DNSSD_OK or an error. */
LONG SDFinderStart(struct SDFinder *f, struct SDSession *s, CONST_STRPTR type);
/* Takes one of the session's events; SD_FOUND_* and, for ADD, UPDATE and
 * REMOVE, the instance in *out (for TYPE and NOTYPE its sv_Type). */
LONG SDFinderEvent(struct SDFinder *f, const struct DNSSDEvent *e,
    struct SDService *out, LONG *rc);
void SDFinderStop(struct SDFinder *f);

#endif /* MDNS_TOOLS_SD_COMMON_H */
