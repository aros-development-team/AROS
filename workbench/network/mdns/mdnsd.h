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
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/tasks.h>

#include <bonami/mdns.h>

#include "mdns_dns.h"
#include "mdns_ipc.h"

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
#define MDNS_PROPOSED_MAX       8

/* a record that went to the group this many seconds ago or less may be
 * answered to the asker alone when the question allows it: a quarter of
 * the life of the shortest-lived records, the host's */
#define MDNS_UNICAST_SECONDS    (MDNS_TTL_HOST / 4)

/* A service a client registered through bonami.library. */
struct MDNSRegisteredService
{
    struct Node     rs_Node;
    char            rs_Instance[MDNS_MAX_NAME + 1];
    char            rs_Type[MDNS_MAX_TYPE + 1];
    char            rs_Domain[MDNS_MAX_DOMAIN + 1];
    char            rs_TXT[MDNS_MAX_TXT + 1];
    UWORD           rs_Port;
    UWORD           rs_State;       /* MDNS_NAME_* */
    UWORD           rs_Step;        /* probes or announcements sent */
    BOOL            rs_Proxy;       /* for another host: its name and
                                     * address were given by the client */
    char            rs_HostTarget[MDNS_MAX_HOST + 8];
                                    /* that host, "name.local" */
    ULONG           rs_HostAddr;    /* and its address */
    ULONG           rs_TTL;         /* of its records, 0 for the defaults */
    APTR            rs_ClientRef;   /* the handle the client has */
    ULONG           rs_TaskID;      /* stable ID of the registering task */
    ULONG           rs_NextTick;    /* mdnsTicks() of the next step */
    ULONG           rs_Answered;    /* md_PacketSerial it last answered */
    ULONG           rs_Multicast;   /* mdnsNow() when its records last went
                                     * to the group */
    ULONG           rs_AnswerTick;  /* when an answer to a browse question
                                     * is due, or 0 for none */
    LONG            rs_AnswerIf;    /* to which interface, or -1 for all */
};

/* Known PTR answers retain their owner and remaining lifetime. */
struct MDNSKnownAnswer
{
    char owner[256];
    char target[256];
    ULONG ttl;
};

struct MDNSDiscovery;
struct MDNSQueryState;

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

    struct MDNSDiscovery   *md_Discovery;
    struct MDNSQueryState  *md_Query;
    ULONG                   md_DaemonID;
    struct MsgPort         *md_Port;        /* MDNS_IPC_PORT, the clients' */
    struct List             md_Services;    /* MDNSRegisteredService */
    ULONG                   md_NextHandle;
    ULONG                   md_PacketSerial; /* counts received queries */
    BOOL                    md_AnswerPending; /* an rs_AnswerTick is set */
};

/* Text of at most max characters: its length, and a copy that always ends
 * with a NUL within size bytes. */
static inline LONG
mdnsTextLength(const char *text, LONG max)
{
    LONG n = 0;

    while (n < max && text[n] != '\0') {
        n++;
    }
    return n;
}

static inline void
mdnsTextCopy(char *to, LONG size, const char *from)
{
    LONG n = 0;

    while (n < size - 1 && from[n] != '\0') {
        to[n] = from[n];
        n++;
    }
    to[n] = '\0';
}

/* mdns_discovery.c: daemon-owned cache and bounded event queues. */
LONG mdnsDiscoveryMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg);
void mdnsDiscoveryReap(struct MDNSDaemon *md);
void mdnsDiscoveryRecord(struct MDNSDaemon *md, const struct MDNSRecord *r,
    const UBYTE *pkt, LONG length, LONG ownerOffset);
void mdnsDiscoveryTick(struct MDNSDaemon *md);
void mdnsDiscoveryReset(struct MDNSDaemon *md);
void mdnsDiscoveryRelease(struct MDNSDaemon *md, ULONG taskID);
void mdnsDiscoveryCancel(struct MDNSDaemon *md);
void mdnsDiscoveryFree(struct MDNSDaemon *md);

/* mdns_query.c: generic record questions and their event queues. */
LONG mdnsQueryMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg);
void mdnsQueryRecord(struct MDNSDaemon *md, const struct MDNSRecord *record,
    const UBYTE *packet, LONG length);
void mdnsQueryTick(struct MDNSDaemon *md);
void mdnsQueryReset(struct MDNSDaemon *md);
void mdnsQueryRelease(struct MDNSDaemon *md, ULONG owner);
void mdnsQueryReap(struct MDNSDaemon *md);
void mdnsQueryCancel(struct MDNSDaemon *md);
void mdnsQueryFree(struct MDNSDaemon *md);

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
void mdnsNetSendAll(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen);
void mdnsNetNameHost(struct MDNSDaemon *md);

/* mdns_host.c */
void mdnsHostRestart(struct MDNSDaemon *md);
BOOL mdnsHostTick(struct MDNSDaemon *md);
void mdnsHostGoodbye(struct MDNSDaemon *md);
void mdnsHostHandleQuery(struct MDNSDaemon *md, struct MDNSQuestion *q);
void mdnsHostHandleProbe(struct MDNSDaemon *md,
    const struct MDNSRecord *theirs, LONG count);
void mdnsHostHandleRecord(struct MDNSDaemon *md, const struct MDNSRecord *r);
void mdnsHostFqdn(const struct MDNSDaemon *md, char *buf, LONG buflen);
void mdnsAddrData(ULONG addr, UBYTE *data);
LONG mdnsResponseBegin(struct MDNSDaemon *md, UBYTE *pkt);

/* mdns_reg.c */
LONG mdnsRegAdd(struct MDNSDaemon *md, struct MDNSIPCMsg *msg);
LONG mdnsRegUpdate(struct MDNSDaemon *md, const struct MDNSIPCMsg *msg);
void mdnsRegRemove(struct MDNSDaemon *md, const struct MDNSIPCMsg *msg);
void mdnsRegRelease(struct MDNSDaemon *md, ULONG taskID);
void mdnsRegFree(struct MDNSDaemon *md);
BOOL mdnsRegTick(struct MDNSDaemon *md);
void mdnsRegAnnounceAll(struct MDNSDaemon *md);
void mdnsRegRestartAll(struct MDNSDaemon *md);
void mdnsRegGoodbyeAll(struct MDNSDaemon *md);
void mdnsRegHandleQuery(struct MDNSDaemon *md, struct MDNSQuestion *q,
    const struct MDNSKnownAnswer *known, LONG knownCount);
void mdnsRegHandleProbe(const struct MDNSDaemon *md,
    const struct MDNSRecord *theirs, LONG count, const UBYTE *pkt,
    LONG pktlen);
void mdnsRegHandleRecord(const struct MDNSDaemon *md,
    const struct MDNSRecord *r, const UBYTE *pkt, LONG pktlen);

/* mdns_client.c */
void mdnsClientMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg);
void mdnsClientReap(struct MDNSDaemon *md);

/* mdns_packet.c */
void mdnsHandlePacket(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    ULONG srcaddr, UWORD srcport);

#endif /* MDNSD_MDNSD_H */
