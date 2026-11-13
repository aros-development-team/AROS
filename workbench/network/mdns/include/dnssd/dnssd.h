/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd/dnssd.h - dnssd.library, the system's DNS-SD service (RFC 6763 over
 * multicast DNS, RFC 6762)
 *
 * A program opens a session with a message port of its own; each request
 * (an address lookup, a browse, a resolve, a registration, a record query)
 * is an operation of the session. mdnsd, the system's responder, keeps the
 * results and tells the session's port when it has some: the program then
 * reads them, one event at a time, into a buffer of its own
 * (DNSSD_ReadEvent()). Nothing of the daemon's - no pointer, no message - is
 * ever left with the program, and nothing arrives for an operation once it
 * is cancelled or its session closed.
 *
 * Every structure here has a fixed layout of fixed-width fields, the same on
 * every CPU, and no pointer. Events say their own size and version; a
 * program reads only what its version has.
 */
#ifndef DNSSD_DNSSD_H
#define DNSSD_DNSSD_H

#include <exec/types.h>
#include <utility/tagitem.h>

#define DNSSDNAME           "dnssd.library"
#define DNSSD_VERSION       1

/* Results: DNSSD_OK, or a negative error. */
#define DNSSD_OK            0
#define DNSSD_ERR_NOENGINE  (-1)    /* mdnsd is not running */
#define DNSSD_ERR_BADPARAM  (-2)
#define DNSSD_ERR_NOMEM     (-3)
#define DNSSD_ERR_BUSY      (-4)    /* too many sessions or operations */
#define DNSSD_ERR_STALE     (-5)    /* the session or operation is gone (mdnsd restarted) */
#define DNSSD_ERR_EMPTY     (-6)    /* no event to read now */
#define DNSSD_ERR_TOOSMALL  (-7)    /* the buffer is smaller than the event: *size says how big */
#define DNSSD_ERR_TIMEOUT   (-8)
#define DNSSD_ERR_NOTFOUND  (-9)
#define DNSSD_ERR_CANCELLED (-10)
#define DNSSD_ERR_BADNAME   (-11)   /* not a name DNS can carry */
#define DNSSD_ERR_EXISTS    (-12)   /* this host has that service registered already */
#define DNSSD_ERR_CONFLICT  (-13)   /* another host has the name (of a host registered for) */

#define DNSSD_TXT_MAX           8900    /* bytes of TXT data a service may have */
#define DNSSD_HOST_ADDRESSES    8       /* addresses of a host registered for */

/* Address families of struct DNSSDAddress (and of DNSSD_Family). */
#define DNSSD_FAMILY_ANY    0
#define DNSSD_FAMILY_IPV4   4
#define DNSSD_FAMILY_IPV6   6

/* Opaque handles. */
typedef struct DNSSDSession   *DNSSDSessionRef;
typedef struct DNSSDOperation *DNSSDOperationRef;

/* A part of an event: dr_Size bytes at dr_Offset from the event's start. */
struct DNSSDRef
{
    ULONG   dr_Offset;
    ULONG   dr_Size;
};

/* One address: 4 bytes for IPv4, 16 for IPv6, and the interface index it
 * was learned on (the zone of an IPv6 link-local address). */
struct DNSSDAddress
{
    UWORD   da_Family;              /* DNSSD_FAMILY_IPV4 or DNSSD_FAMILY_IPV6 */
    UWORD   da_Length;              /* 4 or 16 */
    ULONG   da_Interface;
    UBYTE   da_Bytes[16];
};

/*
 * Event kinds: what an event is about. An event has the layout of mdnsd's
 * canonical message (dnssd_model.h, version 2): it names things as DNS
 * wire labels - DNSSD_NameToText() makes text of them - and an
 * instance by its own label, as it is shown.
 */
#define DNSSD_EVENT_ADD         1   /* Browse: an instance on de_Interface */
#define DNSSD_EVENT_REMOVE      2   /* Browse: that instance is gone; Resolve: the service is gone */
#define DNSSD_EVENT_RESOLVED    3   /* Resolve: target, port, TXT and every address known, again when they change */
#define DNSSD_EVENT_RESOLVE_FAILED 4
#define DNSSD_EVENT_TYPE        5   /* Browse of "_services._dns-sd._udp": a service type on the link */
#define DNSSD_EVENT_ADDRESS     6   /* LookupAddress: an address of de_Name */
#define DNSSD_EVENT_RECORD      7   /* QueryRecord: a record of de_Name, de_RRType, de_RData */
#define DNSSD_EVENT_REGISTERED  8   /* Register: announced under de_Instance - again if a conflict renamed it */
#define DNSSD_EVENT_END         9   /* the operation is over: de_Result says how (DNSSD_OK, DNSSD_ERR_TIMEOUT, ...) */
#define DNSSD_EVENT_LOST        10  /* events were dropped: start the operation again */

#define DNSSD_EVENTF_REMOVED    0x00000008  /* TYPE, ADDRESS, RECORD: what it reports is gone */

#define DNSSD_EVENT_VERSION     2

/*
 * An event, as DNSSD_ReadEvent() copies it: this header, then its parts,
 * each at its de_*.dr_Offset from the start, dr_Size bytes (0: not there).
 *   de_Name      the name it is about, DNS wire labels (ADDRESS, RECORD)
 *   de_Instance  the instance's own label, UTF-8, as it is shown (dots allowed)
 *   de_Type      the service type, wire labels ("_ipp._tcp")
 *   de_Domain    wire labels ("local")
 *   de_Target    the host that offers it, wire labels (RESOLVED)
 *   de_TXT       the TXT record's data, length-prefixed strings
 *   de_RData     a record's data, the names in it uncompressed (RECORD)
 *   de_Addresses de_AddressCount struct DNSSDAddress, sorted, each once
 * de_Operation is the operation's DNSSD_OperationID().
 */
struct DNSSDEvent
{
    ULONG           de_Size;        /* bytes of the whole event */
    UWORD           de_Version;     /* DNSSD_EVENT_VERSION */
    UWORD           de_HeaderSize;  /* sizeof(struct DNSSDEvent) of its version */
    UWORD           de_Kind;        /* DNSSD_EVENT_* */
    UWORD           de_RRType;
    ULONG           de_Flags;       /* DNSSD_EVENTF_* */
    LONG            de_Result;      /* DNSSD_EVENT_END: how the operation ended */
    ULONG           de_Incarnation; /* mdnsd's */
    ULONG           de_Session;
    ULONG           de_Operation;   /* DNSSD_OperationID() of its operation */
    ULONG           de_Sequence;    /* counts the session's events from 1 */
    ULONG           de_Interface;   /* the link it is about, 0 for none or all */
    ULONG           de_TTL;         /* seconds */
    UWORD           de_Priority;    /* Resolve: the SRV record's */
    UWORD           de_Weight;
    UWORD           de_Port;
    UWORD           de_AddressCount;
    struct DNSSDRef de_Name;
    struct DNSSDRef de_Instance;
    struct DNSSDRef de_Type;
    struct DNSSDRef de_Domain;
    struct DNSSDRef de_Target;
    struct DNSSDRef de_TXT;
    struct DNSSDRef de_RData;
    struct DNSSDRef de_Addresses;
};

/* Tags. */
#define DNSSD_Dummy         (TAG_USER + 0x0d55d000)
#define DNSSD_Family        (DNSSD_Dummy + 1)   /* LookupAddress: DNSSD_FAMILY_*, default ANY */
#define DNSSD_Interface     (DNSSD_Dummy + 2)   /* only this interface index, default all */
#define DNSSD_Timeout       (DNSSD_Dummy + 3)   /* seconds, 1-60; LookupAddress default 3, Resolve 5; QueryRecord: none */
#define DNSSD_Domain        (DNSSD_Dummy + 5)   /* Browse, Resolve: default "local" */
#define DNSSD_Subtype       (DNSSD_Dummy + 6)   /* Browse: only instances of this subtype ("_printer");
                                                 * Register: a subtype it has (repeatable) */
#define DNSSD_TXT           (DNSSD_Dummy + 7)   /* Register, Update: TXT data, length-prefixed strings */
#define DNSSD_TXTLength     (DNSSD_Dummy + 8)   /* its length in bytes, at most 1300 */
#define DNSSD_Port          (DNSSD_Dummy + 9)   /* Update: another port */
#define DNSSD_Host          (DNSSD_Dummy + 10)  /* Register for another host: its name ("printer") */
#define DNSSD_HostAddress   (DNSSD_Dummy + 11)  /* Register, Update: one of its addresses, struct
                                                 * DNSSDAddress *, IPv4 or IPv6 - one tag each, up to
                                                 * 8; an IPv6 link-local one with its da_Interface */

/* DNSSD_GetStatusA() */
#define DNSSD_Status_Incarnation (DNSSD_Dummy + 0x100)  /* ULONG *: mdnsd's incarnation */
#define DNSSD_Status_Sessions    (DNSSD_Dummy + 0x101)  /* ULONG *: open sessions */
#define DNSSD_Status_Operations  (DNSSD_Dummy + 0x102)  /* ULONG *: operations in progress */

#endif /* DNSSD_DNSSD_H */
