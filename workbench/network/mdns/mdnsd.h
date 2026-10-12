/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdnsd.h - what the parts of mdnsd share
 */

#ifndef MDNSD_MDNSD_H
#define MDNSD_MDNSD_H

#include <exec/types.h>
#include <exec/libraries.h>

#include "mdns_dns.h"

#define MDNS_DOMAIN_LOCAL       "local"
#define MDNS_MAX_HOST           64

/* An interface mdnsd speaks on, with its own socket. */
#define MDNS_IFS_MAX            8

struct MDNSInterface
{
    char    mi_Name[20];
    ULONG   mi_Addr;            /* 0xAABBCCDD == A.B.C.D */
    LONG    mi_Socket;
};

/* A name is asked for three times, 250 ms apart, before it is announced
 * (RFC 6762 section 8.1), and then announced twice, a second apart. */
#define MDNS_NAME_PROBING       0
#define MDNS_NAME_ANNOUNCED     1
#define MDNS_PROBE_COUNT        3
#define MDNS_PROBE_TICKS        13      /* 250 ms in 1/50 s */
#define MDNS_ANNOUNCE_COUNT     2

/* The daemon has one task and one of these. */
struct MDNSDaemon
{
    struct Library         *md_SocketBase;  /* opened by this task itself */
    LONG                    md_Socket;      /* the first interface's, or -1:
                                             * "is there a link at all" */
    struct MDNSInterface    md_Ifs[MDNS_IFS_MAX];
    LONG                    md_IfCount;
    LONG                    md_SendIf;      /* interface of the packet being
                                             * handled, or -1 */
    UBYTE                  *md_RxBuf;       /* MDNS_MAX_PACKET bytes each */
    UBYTE                  *md_TxBuf;
    ULONG                   md_HostAddr;    /* the first interface's */
    ULONG                   md_RxPackets;
    ULONG                   md_TxPackets;

    char                    md_HostName[MDNS_MAX_HOST + 1];
                                            /* the name in use: the system's
                                             * first label, with "-2", "-3"
                                             * after it lost the name */
    char                    md_HostBase[MDNS_MAX_HOST + 1];
                                            /* what the system calls itself */
    ULONG                   md_HostSuffix;  /* 0, or the number after it */
    UWORD                   md_HostState;   /* MDNS_NAME_* */
    UWORD                   md_HostStep;    /* probes or announcements sent */
    ULONG                   md_HostNextTick;
    ULONG                   md_HostMulticast; /* mdnsNow() when the address
                                             * last went to the group */

    /*
     * Where the answer being sent goes when it is not for the group: to
     * the one who asked (RFC 6762 sections 5.4 and 6.7). md_ReplyAddr is 0
     * for the group. md_ReplyLegacy: the question came from another port
     * than 5353, from a resolver that knows plain DNS only, and the answer
     * repeats its ID and question, lives ten seconds at most and has no
     * cache-flush bit.
     */
    ULONG                   md_ReplyAddr;   /* 0xAABBCCDD == A.B.C.D */
    UWORD                   md_ReplyPort;
    UWORD                   md_ReplyId;
    BOOL                    md_ReplyLegacy;
    struct MDNSQuestion    *md_ReplyQuestion;
    ULONG                   md_QueryAddr;   /* who sent the query in hand */
    UWORD                   md_QueryPort;
};

/* mdnsd.c */
ULONG mdnsNow(void);            /* seconds */
ULONG mdnsTicks(void);          /* fiftieths of a second */

/* mdnsd_service.c: the daemon's place in the stack's service registry.
 * Signal masks; zero where the registry does not signal. */
struct MDNSDSignals
{
    ULONG ms_Stop;      /* the stack is shutting down */
    ULONG ms_Begin;     /* reconfiguration starts: let go of the stack */
    ULONG ms_End;       /* reconfiguration is over: take it up again */
};

BOOL mdnsdServiceOpen(struct MDNSDSignals *sig);
void mdnsdServiceClose(void);

/* mdns_net.c */
BOOL mdnsNetRescan(struct MDNSDaemon *md);
void mdnsNetClose(struct MDNSDaemon *md);
BOOL mdnsNetIsOwn(struct MDNSDaemon *md, ULONG addr);
BOOL mdnsNetSendOn(struct MDNSDaemon *md, LONG index, const UBYTE *pkt,
    LONG pktlen);
void mdnsNetNameHost(struct MDNSDaemon *md);

/* mdns_host.c */
void mdnsHostRestart(struct MDNSDaemon *md);
BOOL mdnsHostTick(struct MDNSDaemon *md);
void mdnsHostGoodbye(struct MDNSDaemon *md);
void mdnsHostHandleQuery(struct MDNSDaemon *md, struct MDNSQuestion *q);
void mdnsHostHandleProbe(struct MDNSDaemon *md,
    const struct MDNSRecord *theirs, LONG count);
void mdnsHostHandleRecord(struct MDNSDaemon *md, const struct MDNSRecord *r);

/* mdns_packet.c */
void mdnsHandlePacket(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    ULONG srcaddr, UWORD srcport);

#endif /* MDNSD_MDNSD_H */
