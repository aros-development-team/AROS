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
#include <sys/socket.h>
#include <netinet/in.h>

#include "mdns_dns.h"
#include "dnssd_ipc_port.h"
#include "dnssd_link.h"
#include "dnssd_reg.h"

/* What the responder answers its front end (dnssd_server.c). */
#define MDNSD_OK                0
#define MDNSD_ERR_BADPARAM      (-1)
#define MDNSD_ERR_NOMEM         (-2)
#define MDNSD_ERR_DUPLICATE     (-4)
#define MDNSD_ERR_NOTFOUND      (-5)

/* The longest time to live a client may give its records (RFC 6762
 * section 10: 75 minutes, the longest the defaults are). */
#define MDNSD_TTL_MAX           4500

/* This host's own label (RFC 1035: 63 bytes) and what the system calls
 * itself, as text. */
#define MDNSD_HOST_MAX          64

/* Bounded daemon policy; overflow is explicit and never publishes a partial
 * canonical snapshot. The socket array is derived from md_LinkInventory. */
#define MDNS_IFS_MAX            32
#define MDNS_ADDRESS_MAX        256
#define MDNS_LINK_SNAPSHOT_MAX  65536u

struct MDNSInterface
{
    ULONG   mi_Index;           /* stack interface index: link identity */
};

/*
 * A name's publication on one link (RFC 6762 sections 8 and 14). The name
 * and its data are their owner's - the host, a service - and the same on
 * every link; where the name is in probing and announcing is the link's,
 * and on it each address family's apart: IPv4 and IPv6 reach different
 * listeners, so a family that comes is probed for alone while the other
 * keeps answering. A family not usable on the link is off.
 */
#define MDNS_FAM_IPV4           0
#define MDNS_FAM_IPV6           1
#define MDNS_FAMS               2

#define MDNS_PUB_OFF            0
#define MDNS_PUB_PROBING        1
#define MDNS_PUB_ANNOUNCING     2
#define MDNS_PUB_ANNOUNCED      3

struct MDNSFamilyPub
{
    UBYTE   fp_State;           /* MDNS_PUB_* */
    UBYTE   fp_Step;            /* probes, or announcements, sent this round */
    UBYTE   fp_Updates;         /* announcements due for changed data (8.4) */
    UBYTE   fp_Pad;
    ULONG   fp_Next;            /* mdnsTicks() of the next step, when one is due */
    ULONG   fp_Multicast;       /* mdnsTicks() the records last went to the
                                 * group by this family on this link, or 0 */
};

struct MDNSPub
{
    ULONG                   p_Link;     /* interface index: the link's identity */
    struct MDNSFamilyPub    p_Family[MDNS_FAMS];
};

/* what a family's publication sends now (mdnsFamilyStep()) */
#define MDNS_STEP_NONE          0
#define MDNS_STEP_PROBE         1       /* a probe */
#define MDNS_STEP_PROBE_FIRST   2       /* the first, asking for a unicast answer */
#define MDNS_STEP_ANNOUNCE      3       /* its records, to the group */

/* A name is asked for three times, 250 ms apart, before it is announced
 * (RFC 6762 section 8.1), and then announced twice, a second apart. */
#define MDNS_NAME_PROBING       0
#define MDNS_NAME_ANNOUNCED     1
#define MDNS_PROBE_COUNT        3
/* Times in mdnsTicks(), fiftieths of a second, from the tick something
 * went out in: a span of n ticks is (n - 1) to n of them. So that it is
 * never shorter than RFC 6762 asks: 250 ms between probes and after the
 * last one before announcing (section 8.1), a second between announcements
 * (section 8.3) and before probing again after a tie (section 8.2). */
#define MDNS_PROBE_TICKS        14      /* 260-280 ms */
#define MDNS_SECOND_TICKS       51      /* 1000-1020 ms */
#define MDNS_ANNOUNCE_COUNT     2
#define MDNS_PROPOSED_MAX       8

/* a record that went to the group this many seconds ago or less may be
 * answered to the asker alone when the question allows it: a quarter of
 * the life of the shortest-lived records, the host's */
#define MDNS_UNICAST_SECONDS    (MDNS_TTL_HOST / 4)

/* The TXT data of a registered service: length-prefixed strings, as a
 * client gives them. RFC 6763 section 6.2: it should not exceed 1300
 * bytes, and may be up to 8900 when it has to. */
#define MDNS_TXT_DATA_MAX       8900

/* Its name is unique: three probes, then announced (RFC 6762 section 8);
 * a conflict over a proxied host's name, which is not this host's to
 * rename, ends the registration. */
#define MDNS_NAME_CONFLICT      2

struct MDNSRegisteredService;
/* Told when a registered service's name is announced (MDNSD_OK; again,
 * after a conflict renamed it), or that another host has the name of the
 * host it stands in for (MDNSD_ERR_DUPLICATE): it is then withdrawn. */
typedef void (*MDNSRegNotify)(APTR data, const struct MDNSRegisteredService *rs,
    LONG result);

/*
 * A service a client registered through dnssd.library: its
 * registration (dnssd_reg.h) and the names built from it, as wire names;
 * its TXT data as it goes on the wire; where it is in probing and
 * announcing.
 */
struct MDNSRegisteredService
{
    struct Node     rs_Node;
    struct dnssd_reg rs_Reg;
    UBYTE           rs_Name[DNSSD_REG_NAME_MAX];     /* <instance>.<type>.<domain> */
    UBYTE           rs_TypeName[DNSSD_REG_NAME_MAX]; /* <type>.<domain> */
    UBYTE           rs_HostName[DNSSD_REG_NAME_MAX]; /* a host it stands in for */
    UWORD           rs_TXTLength;
    UBYTE          *rs_TXT;         /* AllocVec()'d, rs_TXTLength bytes, or NULL */
    MDNSRegNotify   rs_Notify;      /* or NULL */
    APTR            rs_NotifyData;
    UWORD           rs_State;       /* MDNS_NAME_* */
    UWORD           rs_Step;        /* probes or announcements sent */
    ULONG           rs_TTL;         /* of its records, 0 for the defaults */
    APTR            rs_ClientRef;   /* the handle the client has */
    ULONG           rs_NextTick;    /* mdnsTicks() of the next step */
    ULONG           rs_Answered;    /* md_PacketSerial it last answered */
    ULONG           rs_Multicast;   /* mdnsNow() when its records last went
                                     * to the group */
    ULONG           rs_MulticastTick[MDNS_IFS_MAX];
                                    /* exact send tick on each interface */
    ULONG           rs_TypeTick[MDNS_IFS_MAX];
                                    /* the service-type PTR on each link */
    ULONG           rs_AnswerTick;  /* when an answer to a browse question
                                     * is due, or 0 for none */
    LONG            rs_AnswerIf;    /* to which interface, or -1 for all */
    UWORD           rs_AnswerFamilies; /* pending AF_INET/AF_INET6 bit mask */
    UWORD           rs_AnswerSubtypes; /* subtype PTRs pending for a query */
};

/* Known PTR answers retain their owner and remaining lifetime. */
struct MDNSKnownAnswer
{
    UBYTE owner[256];   /* wire names */
    UBYTE target[256];
    ULONG ttl;
};

struct MDNSCache;
struct DNSSDServer;

/* The daemon has one task and one of these. */
struct MDNSDaemon
{
    struct Library         *md_SocketBase;  /* opened by this task itself */
    LONG                    md_Socket;      /* md_Socket4 or md_Socket6, -1
                                             * while there is no link at all */
    LONG                    md_Socket4;     /* the IPv4 UDP socket of every link, or -1 */
    LONG                    md_Socket6;     /* the IPv6 UDP socket of every link, or -1 */
    struct dnssd_link_inventory md_LinkInventory;
    struct dnssd_link_entry md_Links[MDNS_IFS_MAX];
    struct dnssd_link_entry md_LinkScratch[MDNS_IFS_MAX];
    struct dnssd_address_entry md_Addresses[MDNS_ADDRESS_MAX];
    struct dnssd_address_entry md_AddressScratch[MDNS_ADDRESS_MAX];
    struct MDNSInterface    md_Ifs[MDNS_IFS_MAX];
    LONG                    md_IfCount;
    LONG                    md_SendIf;      /* interface of the packet being
                                             * handled, or -1 */
    UWORD                   md_SendFamily;  /* AF_INET/AF_INET6, or 0 */
    UWORD                   md_SendPad;
    UBYTE                  *md_RxBuf;       /* MDNS_MAX_PACKET bytes each */
    UBYTE                  *md_TxBuf;
    ULONG                   md_HostAddr;    /* the first interface's */
    ULONG                   md_RxPackets;
    ULONG                   md_TxPackets;
    ULONG                   md_RxAnswers;

    char                    md_HostName[MDNSD_HOST_MAX + 1];
                                            /* the name in use: the system's
                                             * first label, with "-2", "-3"
                                             * after it lost the name */
    char                    md_HostBase[MDNSD_HOST_MAX + 1];
                                            /* what the system calls itself */
    ULONG                   md_HostSuffix;  /* 0, or the number after it */
    struct MDNSPub         *md_HostPubs;    /* AllocVec()'d: the host's name
                                             * on each link it is published on */
    UWORD                   md_HostPubCount;
    UWORD                   md_HostPubMax;
    BOOL                    md_HostPubsShort; /* a publication of the host's
                                             * found no memory (mdnsHostTick()) */
    ULONG                   md_HostPubsRetry; /* mdnsTicks() it is tried again */
    BOOL                    md_HostRenamed; /* a new name: its services are
                                             * announced again once it is the
                                             * host's on every link */

    /*
     * Where the answer being sent goes when it is not for the group: to
     * the one who asked (RFC 6762 sections 5.4 and 6.7). md_ReplyFamily is
     * zero for the group. md_ReplyLegacy: the question came from another port
     * than 5353, from a resolver that knows plain DNS only, and the answer
     * repeats its ID and question, lives ten seconds at most and has no
     * cache-flush bit.
     */
    UWORD                   md_ReplyFamily; /* AF_INET or AF_INET6; 0=group */
    UWORD                   md_ReplyPad;
    ULONG                   md_ReplyAddr;   /* 0xAABBCCDD == A.B.C.D */
    struct in6_addr         md_ReplyAddr6;
    UWORD                   md_ReplyPort;
    UWORD                   md_ReplyId;
    BOOL                    md_ReplyLegacy;
    struct MDNSQuestion    *md_ReplyQuestion;
    UWORD                   md_QueryFamily; /* family of the query in hand */
    UWORD                   md_QueryPad;
    ULONG                   md_QueryAddr;   /* who sent the query in hand */
    struct in6_addr         md_QueryAddr6;
    UWORD                   md_QueryPort;

    struct MDNSCache       *md_Cache;
    struct DNSSDServer     *md_DNSSD;   /* dnssd.library's sessions */
    struct MsgPort         *md_DNSSDPort;
    ULONG                   md_DaemonID;
    IPTR                    md_Memory;      /* AvailMem(MEMF_ANY) at start */
    struct List             md_Services;    /* MDNSRegisteredService */
    ULONG                   md_NextHandle;
    ULONG                   md_PacketSerial; /* counts received queries */
    BOOL                    md_AnswerPending; /* an rs_AnswerTick is set */
    ULONG                   md_TypesAnswerTick;
                                            /* delayed type-enumeration reply */
    LONG                    md_TypesAnswerIf;
                                            /* its interface, or -1 for all */
    UWORD                   md_TypesAnswerFamilies;
                                            /* pending address-family mask */
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

/*
 * What mdnsd may take of a kind of memory: a share of the memory free as it
 * started (md_Memory), within floor and cap - the machine's memory, not its
 * CPU, says how much there is.
 */
static inline ULONG
mdnsShare(const struct MDNSDaemon *md, ULONG divisor, ULONG floor, ULONG cap)
{
    /* AvailMem() is an IPTR: past 4 GB on 64-bit, kept whole */
    IPTR share = md->md_Memory / divisor;

    return share < floor ? floor : share > cap ? cap : (ULONG)share;
}

/* mdns_cache.c: the records heard on the links and the questions that
 * keep them, within 1/128 of the free memory (16 KB to 256 KB) */
#define MDNS_CACHE_SHARE    128
#define MDNS_CACHE_FLOOR    (16 * 1024)
#define MDNS_CACHE_CAP      (256 * 1024)
#define MDNS_CACHE_LOCAL    1       /* one of the host's own records */

struct MDNSCacheEntry
{
    struct MinNode  ce_Node;
    ULONG           ce_Link;        /* interface index it was heard on */
    ULONG           ce_Received;    /* mdnsTicks() */
    ULONG           ce_Expires;     /* mdnsTicks() */
    ULONG           ce_TTL;         /* seconds, as received */
    ULONG           ce_Next;        /* the next refresh, 0 not set */
    ULONG           ce_Size;        /* bytes allocated */
    UBYTE          *ce_Name;        /* wire labels, uncompressed */
    UBYTE          *ce_Data;        /* RDATA, its names expanded */
    UWORD           ce_Type;
    UWORD           ce_NameLength;
    UWORD           ce_DataLength;
    UBYTE           ce_Flags;
    UBYTE           ce_Refresh;     /* refreshes asked: 0-4 */
};

/* Told of a record that matches a watch as it comes (added) and goes. */
typedef void (*MDNSCacheNotify)(APTR data, const struct MDNSCacheEntry *entry,
    BOOL added);

BOOL mdnsCacheInit(struct MDNSDaemon *md);
void mdnsCacheFree(struct MDNSDaemon *md);
void mdnsCacheRecord(const struct MDNSDaemon *md, const UBYTE *pkt, LONG length,
    LONG offset, ULONG link, BOOL local);
ULONG mdnsCacheTick(struct MDNSDaemon *md);
void mdnsCacheLinksChanged(const struct MDNSDaemon *md);
APTR mdnsCacheWatch(const struct MDNSDaemon *md, const UBYTE *name, UWORD type,
    ULONG link, MDNSCacheNotify notify, APTR data);
void mdnsCacheUnwatch(const struct MDNSDaemon *md, APTR watch);
void mdnsCacheDropLocal(const struct MDNSDaemon *md, const UBYTE *owner,
    const UBYTE *target);

/*
 * The sooner of two mdnsTicks() times, 0 standing for none: what each part
 * says is due next, for the main loop's wait (wait_and_receive()). Ticks
 * are compared as a difference, so their wrapping does not matter.
 */
static inline ULONG
mdnsSooner(ULONG a, ULONG b)
{
    if (a == 0) {
        return b;
    }
    if (b == 0) {
        return a;
    }
    return (LONG)(a - b) <= 0 ? a : b;
}

/* dnssd_server.c: dnssd.library's sessions and operations. */
BOOL dnssdServerInit(struct MDNSDaemon *md, ULONG gone);
void dnssdServerGone(struct MDNSDaemon *md);
void dnssdServerMessage(struct MDNSDaemon *md, struct DNSSDIPCMsg *msg);
void dnssdServerReap(struct MDNSDaemon *md);
ULONG dnssdServerTick(struct MDNSDaemon *md);
void dnssdServerCancel(struct MDNSDaemon *md);
void dnssdServerFree(struct MDNSDaemon *md);

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
BOOL mdnsNetRescan(struct MDNSDaemon *md, struct dnssd_link_changes *changes);
void mdnsNetClose(struct MDNSDaemon *md);
void mdnsNetSystemName(struct Library *SocketBase, char *base);
BOOL mdnsNetIsOwn(struct MDNSDaemon *md, ULONG addr);
BOOL mdnsNetIsOwn6(struct MDNSDaemon *md, const struct in6_addr *addr);
const struct dnssd_address_entry *mdnsNetAddressAt(
    const struct MDNSDaemon *md, LONG index, UWORD family, size_t ordinal);
ULONG mdnsNetAddress4(const struct MDNSDaemon *md, LONG index);
BOOL mdnsNetAddress6(const struct MDNSDaemon *md, LONG index,
    struct in6_addr *address);
BOOL mdnsNetSendOn(struct MDNSDaemon *md, LONG index, const UBYTE *pkt,
    LONG pktlen);
BOOL mdnsNetSendOnFamily(struct MDNSDaemon *md, LONG index, UWORD fam,
    const UBYTE *pkt, LONG pktlen);
LONG mdnsNetSlot(const struct MDNSDaemon *md, ULONG link);
BOOL mdnsNetHasFamily(const struct MDNSDaemon *md, LONG index, UWORD fam);

/* mdns_pub.c: publications per link and family */
UWORD mdnsFamOf(UWORD af);              /* AF_INET/AF_INET6 -> MDNS_FAM_* */
UWORD mdnsDnssdFamily(UWORD fam);       /* MDNS_FAM_* -> DNSSD_ADDRESS_* */
UWORD mdnsFamOfDnssd(UWORD family);     /* DNSSD_ADDRESS_* -> MDNS_FAM_* */
struct MDNSPub *mdnsPubFind(struct MDNSPub *pubs, UWORD count, ULONG link);
struct MDNSPub *mdnsPubAdd(struct MDNSPub **pubs, UWORD *count, UWORD *max,
    ULONG link);
void mdnsPubRemove(struct MDNSPub *pubs, UWORD *count, ULONG link);
void mdnsFamilyStart(struct MDNSFamilyPub *fp, ULONG next);
BOOL mdnsFamilyAnswers(const struct MDNSFamilyPub *fp);
UWORD mdnsFamilyStep(struct MDNSFamilyPub *fp, ULONG now, ULONG *soonest);
BOOL mdnsPubAnswers(const struct MDNSPub *pub);
void mdnsPubUpdate(struct MDNSPub *pub, ULONG now);
void mdnsNetSendAll(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen);

/* mdns_host.c */
void mdnsHostLinksChanged(struct MDNSDaemon *md,
    const struct dnssd_link_changes *changes);
void mdnsHostLinksClear(struct MDNSDaemon *md);
BOOL mdnsHostReady(const struct MDNSDaemon *md);
BOOL mdnsHostAnswersOn(const struct MDNSDaemon *md, LONG index, UWORD af);
ULONG mdnsHostTick(struct MDNSDaemon *md);
void mdnsHostGoodbye(struct MDNSDaemon *md);
void mdnsHostRename(struct MDNSDaemon *md, const char *base);
void mdnsHostSetName(struct MDNSDaemon *md, const char *base, ULONG suffix);
void mdnsHostHandleQuery(struct MDNSDaemon *md, struct MDNSQuestion *q);
void mdnsHostHandleProbe(struct MDNSDaemon *md,
    const struct MDNSRecord *theirs, LONG count);
void mdnsHostHandleRecord(struct MDNSDaemon *md, const struct MDNSRecord *r);
void mdnsHostFqdn(const struct MDNSDaemon *md, char *buf, LONG buflen);
void mdnsAddrData(ULONG addr, UBYTE *data);
void mdnsAddr6Data(const struct in6_addr *addr, UBYTE *data);
LONG mdnsResponseBegin(struct MDNSDaemon *md, UBYTE *pkt);
BOOL mdnsReplyIsUnicast(const struct MDNSDaemon *md);

/* mdns_reg.c */
LONG mdnsRegAddCanonical(struct MDNSDaemon *md, const struct dnssd_reg *reg,
    const UBYTE *txt, UWORD txtLength, ULONG ttl,
    MDNSRegNotify notify, APTR data, APTR *ref);
LONG mdnsRegUpdateRef(struct MDNSDaemon *md, APTR ref, const UBYTE *txt,
    UWORD txtLength, UWORD port, const struct dnssd_model_wire_address *addrs,
    LONG addrCount, ULONG ttl);
void mdnsRegRemoveRef(struct MDNSDaemon *md, APTR ref);
void mdnsRegFree(struct MDNSDaemon *md);
ULONG mdnsRegTick(struct MDNSDaemon *md);
void mdnsRegAnnounceAll(struct MDNSDaemon *md);
void mdnsRegRestartAll(struct MDNSDaemon *md);
void mdnsRegGoodbyeAll(struct MDNSDaemon *md);
void mdnsRegHandleQuery(struct MDNSDaemon *md, struct MDNSQuestion *q,
    const struct MDNSKnownAnswer *known, LONG knownCount);
void mdnsRegHandleProbe(const struct MDNSDaemon *md,
    const struct MDNSRecord *theirs, LONG count, const UBYTE *pkt,
    LONG pktlen);
void mdnsRegHandleRecord(struct MDNSDaemon *md,
    const struct MDNSRecord *r, const UBYTE *pkt, LONG pktlen);

/* mdns_packet.c */
void mdnsHandlePacket(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    ULONG srcaddr, UWORD srcport);
void mdnsHandlePacket6(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    const struct in6_addr *srcaddr, UWORD srcport);
/* What the host sends to the group, into its own cache. */
void mdnsReflectPacket(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen,
    LONG ifindex);

#endif /* MDNSD_MDNSD_H */
