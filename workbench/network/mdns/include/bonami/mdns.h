/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 */
#ifndef BONAMI_MDNS_H
#define BONAMI_MDNS_H
/*------------------------------------------------------------------------*/
/*
 * mdns.h - Multicast DNS / DNS-SD API (bonami.library)
 *
 * bonami.library is the AmigaOS subsystem; this header names the wire protocol
 * (mDNS, RFC 6762 / DNS-SD, RFC 6763).  Function names follow Bonjour / Avahi
 * semantics with Amiga TagItem (*A), Hook callbacks, and Roadshow-style tags.
 *
 * Compare: bsdsocket.library exposes socket()/bind() (POSIX) plus
 * ConfigureInterfaceTagList() (Amiga).  Here: RegisterServiceA() (DNS-SD)
 * plus InquiryFromNIPCTagsA() in bonami/nipc.h (Envoy).
 */
/*------------------------------------------------------------------------*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <libraries/bonamibase.h>

/*------------------------------------------------------------------------*/
/* Library identity (subsystem name, not API prefix) */

#define BONAMI_LIB_VERSION   41
#define BONAMI_LIB_REVISION  2

/*------------------------------------------------------------------------*/
/* DNS-SD well-known values */

#define MDNS_DOMAIN_LOCAL       "local"
#define MDNS_TYPE_SERVICES_DNS_SD "_services._dns-sd._udp"
#define MDNS_TYPE_ENVOY_NIPC    "_envoy-nipc._tcp"

#define MDNS_TXTKEY_ENTITY      "entity"
#define MDNS_TXTKEY_VERSION     "vers"
#define MDNS_TXTKEY_HOST        "host"

/*------------------------------------------------------------------------*/
/* Buffer limits */

#define MDNS_MAX_NAME           64
#define MDNS_MAX_TYPE           32
#define MDNS_MAX_DOMAIN         16
#define MDNS_MAX_HOST           64
#define MDNS_MAX_TXT            256

/*------------------------------------------------------------------------*/
/* TagItem arguments — RegisterServiceA, BrowseServiceTypeA, etc. */

#define MDNS_Dummy              (TAG_USER + 0xB3000)

#define MDNS_ServiceName        (MDNS_Dummy + 1)   /* STRPTR instance name */
#define MDNS_ServiceType        (MDNS_Dummy + 2)   /* STRPTR e.g. _http._tcp */
#define MDNS_Port               (MDNS_Dummy + 3)   /* UWORD TCP/UDP port */
#define MDNS_TXTRecord          (MDNS_Dummy + 4)   /* STRPTR, escaped space-separated TXT items */
#define MDNS_Domain             (MDNS_Dummy + 5)   /* STRPTR, default local */
#define MDNS_HostName           (MDNS_Dummy + 6)   /* STRPTR target host FQDN */
#define MDNS_InterfaceName      (MDNS_Dummy + 7)   /* STRPTR Roadshow if name */
#define MDNS_Flags              (MDNS_Dummy + 8)   /* ULONG MDNSFLG_* */
#define MDNS_UserData           (MDNS_Dummy + 9)   /* APTR hook userdata hint */
#define MDNS_ReplyPort          (MDNS_Dummy + 10)  /* struct MsgPort * sync */
#define MDNS_RegisterRef        (MDNS_Dummy + 11)  /* MDNSRegisterRef output */
#define MDNS_BrowseRef          (MDNS_Dummy + 12)  /* MDNSBrowseRef output */
#define MDNS_ResolveRef         (MDNS_Dummy + 13)  /* MDNSResolveRef output */

/* AROS registration extensions; these values do not overlap upstream tags.
 * TXT items are separated by spaces; escape an item's space or backslash
 * with a backslash. Each decoded item becomes one DNS-SD TXT string. */
#define MDNS_HostAddr           (MDNS_Dummy + 14)  /* ULONG IPv4, network order */
#define MDNS_TimeToLive         (MDNS_Dummy + 17)  /* ULONG seconds, 1..4500 */

/* AROS discovery: each operation has one bounded event queue,
 * read with ReadMDNSEvent() in the caller's own task. Nothing runs in mdnsd
 * on the caller's behalf; there are no daemon callbacks. */
#define MDNS_EventMode         (MDNS_Dummy + 18)
#define MDNS_EVENT_QUEUE       1
#define MDNS_DISCOVERY_TXT_MAX 512

/* Counted wire TXT strings; names are individual labels, without escaping.
 * Address and interface are network byte order. No embedded pointers. */
struct MDNSDiscoveryEvent
{
    ULONG de_Code;
    ULONG de_Address;
    ULONG de_Interface;
    UWORD de_Port;
    UWORD de_TXTLength;
    char de_Name[64];
    char de_Type[MDNS_MAX_TYPE + 1];
    UBYTE de_TXT[MDNS_DISCOVERY_TXT_MAX];
};

/* Optional wakeup for a queue: a message the caller lends to mdnsd with
 * RequestMDNSEvent(). The caller sets er_Msg.mn_ReplyPort and mn_Length;
 * mdnsd sets er_Result and replies to it exactly once. Until then it
 * belongs to mdnsd: StopBrowse()/StopResolve() return only after it is
 * back on the reply port.
 *   MDNS_OK           events are waiting: read until MDNSERR_NOTREADY
 *   MDNSERR_BUSY      the queue overflowed: stop and start again
 *   MDNSERR_CANCELLED the operation was stopped
 *   MDNSERR_NOENGINE  mdnsd ended */
struct MDNSEventRequest
{
    struct Message er_Msg;
    LONG           er_Result;
    ULONG          er_Reserved;
};

/* GetMDNSStatusA output counters (ti_Data = ULONG *) */
#define MDNS_StatusRxPackets    (MDNS_Dummy + 20)  /* ULONG * RX datagram count */
#define MDNS_StatusTxPackets    (MDNS_Dummy + 21)  /* ULONG * TX datagram count */
#define MDNS_StatusRxAnswers    (MDNS_Dummy + 22)  /* ULONG * RX answer records */
#define MDNS_StatusDbgRecs      (MDNS_Dummy + 23)  /* ULONG * records into handler */
#define MDNS_StatusDbgPtr       (MDNS_Dummy + 24)  /* ULONG * PTR records seen */
#define MDNS_StatusDbgNotify    (MDNS_Dummy + 25)  /* ULONG * hook fires attempted */

/*------------------------------------------------------------------------*/
/* Registration / browse flags (DNSServiceFlags subset) */

#define MDNSFLG_DEFAULT         0
#define MDNSFLG_NOAUTORENAME    1   /* kDNSServiceFlagsNoAutoRename */
#define MDNSFLG_SHARED          2   /* kDNSServiceFlagsShareConnection */

/*------------------------------------------------------------------------*/
/* Hook event codes — BrowseServiceTypeA / ResolveServiceA */

#define MEVENT_ADD              1
#define MEVENT_REMOVE           2
#define MEVENT_RESOLVED         3
#define MEVENT_RESOLVE_FAILED   4

/*------------------------------------------------------------------------*/
/* Return codes (0 = success, negative = MDNSERR_*) */

#define MDNS_OK                 0
#define MDNSERR_BADPARAM        (-1)
#define MDNSERR_NOMEM           (-2)
#define MDNSERR_TIMEOUT         (-3)
#define MDNSERR_DUPLICATE       (-4)
#define MDNSERR_NOTFOUND        (-5)
#define MDNSERR_BADTYPE         (-6)
#define MDNSERR_NETWORK         (-7)
#define MDNSERR_NOENGINE        (-8)   /* shared engine not available */
#define MDNSERR_NOTREADY        (-9)   /* library present, engine stub */
#define MDNSERR_BUSY            (-10)
#define MDNSERR_CANCELLED       (-11)

/*------------------------------------------------------------------------*/
/* Opaque operation handles — typedefs in libraries/bonamibase.h */

/*------------------------------------------------------------------------*/
/* Service instance snapshot (browse or resolve result) */

struct MDNSServiceInstance
{
    struct Node    si_Node;
    char           si_Name[MDNS_MAX_NAME + 1];
    char           si_Type[MDNS_MAX_TYPE + 1];
    char           si_Domain[MDNS_MAX_DOMAIN + 1];
    char           si_HostName[MDNS_MAX_HOST + 1];
    ULONG          si_HostAddr;     /* IPv4, network byte order */
    UWORD          si_Port;
    UWORD          si_TXTLength;
    UBYTE          si_TXT[MDNS_MAX_TXT];
};

/*------------------------------------------------------------------------*/
/* Hook packet structures */

struct MDNSBrowseMsg
{
    ULONG                   bm_Code;    /* MEVENT_ADD / MEVENT_REMOVE */
    struct MDNSServiceInstance bm_Instance;
};

struct MDNSResolveMsg
{
    ULONG                   rm_Code;    /* MEVENT_RESOLVED / MEVENT_RESOLVE_FAILED */
    struct MDNSServiceInstance rm_Instance;
};

/*------------------------------------------------------------------------*/
/* Function prototypes: clib/bonami_protos.h (via proto/bonami.h) */

/*------------------------------------------------------------------------*/

#endif /* BONAMI_MDNS_H */
