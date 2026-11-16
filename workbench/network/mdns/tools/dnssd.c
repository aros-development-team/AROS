/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * dnssd - a DNS-SD command-line tool in the spirit of Apple's dns-sd and
 *         Avahi's avahi-browse, on dnssd.library.
 */

/******************************************************************************

    NAME

        dnssd -B|-Z <type> [<domain>]
        dnssd -L <name> <type> [<domain>]
        dnssd -R <name> <type> <domain> <port> [-t <seconds>] [<key=value> ...]
        dnssd -P <name> <type> <domain> <port> <host> <ip> [<key=value> ...]
        dnssd -q <name> <rrtype>
        dnssd -G v4|v6|v4v6 <hostname>
        dnssd -E | -F | -V | -h

    SYNOPSIS

    LOCATION

        C:

    FUNCTION

        Finds and publishes services on the local network with DNS Service
        Discovery over multicast DNS (RFC 6763, RFC 6762), through
        dnssd.library and the system's responder, mdnsd.

        -B browses for the instances of a service type and resolves each:
        a line for each one as it comes (ADD, with its address, port and
        TXT items), when what it says changes (UPDATE) and when it goes
        (REMOVE). Up to 16 instances are followed at a time; past them the
        others are resolved once, and followed when room is made. The type "_services._dns-sd._udp"
        lists the service types on the link instead (TYPE, NO TYPE). A
        subtype is browsed as "_printer._sub._ipp._tcp".

        -Z browses as -B and writes what it finds as zone records: PTR, SRV,
        TXT and the host's A record.

        -L resolves one instance: its host, port, TXT items and every
        address, IPv4 and IPv6, or "Not resolved." after five seconds. -B
        shows one address of each instance, IPv4 if it has one; -Z writes A
        and AAAA records for them all.

        -R publishes a service of this computer until Ctrl-C; -t sets the
        time to live of its records (1-4500 seconds). -P publishes it for
        another host, one that does not speak mDNS itself, with that host's
        name and address: IPv4, or IPv6 ("fe80::1%2", a link-local one with
        its interface). A TXT value may be binary: "\xNN" in an item is the
        byte NN and "\\" a backslash. TXT data is shown so too, with "\ "
        for a space inside an item.

        -q asks for any record (A, AAAA, PTR, TXT, SRV, ANY or a number)
        until Ctrl-C. -G looks a host's addresses up. A name without a dot
        is taken to be in "local".

        -E and -F list the domains for registering and browsing: only
        "local", which is what multicast DNS serves. The domain may be
        given as "." or "local" for it. -V shows dnssd.library's version.

    INPUTS

    RESULT

        0, 5 for bad arguments or nothing found, 20 for an error.

    EXAMPLE

        dnssd -B _http._tcp
        dnssd -B _services._dns-sd._udp
        dnssd -L "My Mac" _companion-link._tcp
        dnssd -R AmigaShare _http._tcp . 80 path=/ vers=1
        dnssd -q _http._tcp.local PTR
        dnssd -G v4 printer.local

    SEE ALSO

        Nearby, Share, Handshake

******************************************************************************/

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dnssd.h>
#include <utility/tagitem.h>

#include "sd_common.h"

static const char version_tag[] __attribute__((used)) =
    "\0$VER: dnssd 42.0 (13.11.2026)";

#define DS_DNS_A       1
#define DS_DNS_PTR     12
#define DS_DNS_TXT     16
#define DS_DNS_AAAA    28
#define DS_DNS_SRV     33
#define DS_DNS_ANY     255

/*------------------------------------------------------------------------*/
/* Helpers */

/*
 * Multicast DNS serves the "local" domain only. dns-sd's spellings of the
 * default ("", ".") and "local"/"local." are taken; for anything else a
 * note is printed and "local" is used, so that a script is not misled.
 */
static int
DSDomainLocal(CONST_STRPTR domain)
{
    if (domain == NULL) {
        return 1;
    }
    return SDStrEqual((const char *)domain, "") ||
           SDStrEqual((const char *)domain, ".") ||
           SDStrEqual((const char *)domain, "local") ||
           SDStrEqual((const char *)domain, "local.");
}

static void
DSDomainNote(CONST_STRPTR domain)
{
    if (!DSDomainLocal(domain)) {
        SDPrintf("(note: domain \"%s\" is not served; using local)\n", domain);
    }
}

/* What a session ended with, said the way a user can act on. */
static int
DSEnded(CONST_STRPTR what, LONG rc)
{
    if (rc == DNSSD_ERR_BUSY) {
        SDPrintf("\n%s: more answers came than could be kept; run it again.\n",
            what);
    } else if (rc == DNSSD_ERR_STALE || rc == DNSSD_ERR_NOENGINE) {
        SDPrintf("\n%s: mdnsd ended.\n", what);
    } else {
        SDAppPrintError(what, rc);
    }
    return 20;
}

/* A name as the library takes it: one without a dot is in "local". */
static void
DSLocalName(CONST_STRPTR name, char *out, LONG max)
{
    LONG i;

    SDStrCopy(out, (const char *)name, max);
    for (i = 0; out[i] != '\0' && out[i] != '.'; i++) {
    }
    if (out[i] == '\0') {
        SDStrAppend(out, ".local", max);
    }
}

/* One line for an instance: action, name, type, an address (IPv4 if it
 * has one, else IPv6: -L lists them all), port and the TXT record's items. */
static void
DSPrintService(CONST_STRPTR action, const struct SDService *sv)
{
    char address[64];

    SDPrintf("%-8s %-28s %-22s ", action, sv->sv_Name, sv->sv_Type);
    if (sv->sv_AddressCount != 0) {
        SDAddressText(&sv->sv_Addresses[0], address, (LONG)sizeof(address));
    } else {
        SDStrCopy(address, "-", (LONG)sizeof(address));
    }
    SDPrintf("%-15s %5ld  ", address, (LONG)sv->sv_Port);
    SDPrintTXT(sv->sv_TXT, sv->sv_TXTLength);
    SDPrintf("\n");
}

/*------------------------------------------------------------------------*/
/* -E / -F : multicast DNS serves the link-local domain only, so both lists
 * hold "local" alone. */

static int
DSEnumDomains(CONST_STRPTR which)
{
    SDPrintf("Domains for %s:\n", which);
    SDPrintf("  Added     local\n");
    SDPrintf("\n(multicast DNS serves only the local domain)\n");
    return 0;
}

/*------------------------------------------------------------------------*/
/* -V : library version. */

static int
DSVersion(void)
{
    SDPrintf("%s %ld.%ld\n", DNSSDNAME,
        (LONG)DNSSDBase->lib_Version, (LONG)DNSSDBase->lib_Revision);
    return 0;
}

static UWORD
DSRecordType(CONST_STRPTR word)
{
    LONG value;

    if (SDStrEqual(word, "A")) {
        return DS_DNS_A;
    }
    if (SDStrEqual(word, "PTR")) {
        return DS_DNS_PTR;
    }
    if (SDStrEqual(word, "TXT")) {
        return DS_DNS_TXT;
    }
    if (SDStrEqual(word, "AAAA")) {
        return DS_DNS_AAAA;
    }
    if (SDStrEqual(word, "SRV")) {
        return DS_DNS_SRV;
    }
    if (SDStrEqual(word, "ANY")) {
        return DS_DNS_ANY;
    }
    if (!SDParseNum(word, &value) || value < 1 || value > 65535) {
        return 0;
    }
    return (UWORD)value;
}

/* A wire name within a record's data as text, with a final dot. */
static BOOL
DSDataName(const UBYTE *data, LONG length, char *out, LONG outmax)
{
    if (DNSSD_NameToText(data, (ULONG)length, (STRPTR)out, (ULONG)outmax - 1) < 0) {
        return FALSE;
    }
    SDStrAppend(out, ".", outmax);
    return TRUE;
}

static void
DSPrintAddress(const struct DNSSDAddress *a)
{
    char text[64];

    SDAddressText(a, text, (LONG)sizeof(text));
    SDPrintf("%s", text);
}

static void
DSPrintRecord(const struct DNSSDEvent *e)
{
    const UBYTE *data = SDEventPart(e, &e->de_RData);
    LONG length = (LONG)e->de_RData.dr_Size;
    char name[512];

    SDEventName(e, &e->de_Name, name, (LONG)sizeof(name));
    SDPrintf("%s%s.  type %ld  ttl %ld  ", (CONST_STRPTR)
        ((e->de_Flags & DNSSD_EVENTF_REMOVED) ? "(gone) " : ""), name,
        (LONG)e->de_RRType, (LONG)e->de_TTL);
    if (data == NULL) {
        length = 0;
    }
    if (e->de_RRType == DS_DNS_A && length == 4) {
        SDPrintf("%ld.%ld.%ld.%ld", (LONG)data[0], (LONG)data[1],
            (LONG)data[2], (LONG)data[3]);
    } else if (e->de_RRType == DS_DNS_PTR &&
               DSDataName(data, length, name, (LONG)sizeof(name))) {
        SDPrintf("%s", name);
    } else if (e->de_RRType == DS_DNS_SRV && length > 6 &&
               DSDataName(data + 6, length - 6, name, (LONG)sizeof(name))) {
        SDPrintf("%ld %ld %ld %s",
            (LONG)((data[0] << 8) | data[1]), (LONG)((data[2] << 8) | data[3]),
            (LONG)((data[4] << 8) | data[5]), name);
    } else if (e->de_RRType == DS_DNS_AAAA && length == 16) {
        struct DNSSDAddress a;

        a.da_Family = DNSSD_FAMILY_IPV6;
        a.da_Length = 16;
        a.da_Interface = 0;
        CopyMem((APTR)data, a.da_Bytes, 16);
        DSPrintAddress(&a);
    } else if (e->de_RRType == DS_DNS_TXT) {
        SDPrintTXT(data, length);
    } else {
        LONG i;

        for (i = 0; i < length && i < 32; i++) {
            SDPrintf("%02lx", (LONG)data[i]);
        }
    }
    SDPrintf("\n");
}

/*------------------------------------------------------------------------*/
/* -B / -Z : browse, and resolve each instance found. */

static void
DSPrintZone(const struct SDService *sv)
{
    SDPrintf("%s.local.\tPTR\t%s.%s.local.\n", sv->sv_Type, sv->sv_Name,
        sv->sv_Type);
    SDPrintf("%s.%s.local.\tSRV\t0 0 %ld %s.\n", sv->sv_Name, sv->sv_Type,
        (LONG)sv->sv_Port, sv->sv_Target);
    if (sv->sv_TXTLength > 0) {
        SDPrintf("%s.%s.local.\tTXT\t", sv->sv_Name, sv->sv_Type);
        SDPrintTXT(sv->sv_TXT, sv->sv_TXTLength);
        SDPrintf("\n");
    }
    {
        UWORD i;

        for (i = 0; i < sv->sv_AddressCount; i++) {
            char text[64];
            struct DNSSDAddress a = sv->sv_Addresses[i];

            a.da_Interface = 0;             /* a zone is no part of the record */
            SDAddressText(&a, text, (LONG)sizeof(text));
            SDPrintf("%s.\t%s\t%s\n", sv->sv_Target, (CONST_STRPTR)
                (a.da_Family == DNSSD_FAMILY_IPV4 ? "A" : "AAAA"), text);
        }
    }
}

static int
DSBrowse(CONST_STRPTR type, CONST_STRPTR domain, int zone)
{
    struct SDSession session;
    struct SDFinder finder;
    struct SDService *sv;
    LONG rc, why;
    int result = -1;

    DSDomainNote(domain);
    sv = AllocVec(sizeof(*sv), MEMF_ANY | MEMF_CLEAR);
    if (sv == NULL) {
        SDAppPrintError("browse", DNSSD_ERR_NOMEM);
        return 20;
    }
    if ((rc = SDSessionOpen(&session, 0)) != DNSSD_OK) {
        FreeVec(sv);
        SDAppPrintError("browse", rc);
        return 20;
    }
    if ((rc = SDFinderStart(&finder, &session, type)) != DNSSD_OK) {
        SDSessionClose(&session);
        FreeVec(sv);
        SDAppPrintError("browse", rc);
        return 20;
    }
    if (zone) {
        SDPrintf("; browsing %s (Ctrl-C to stop)\n\n", type);
    } else {
        SDPrintf("Browsing %s (Ctrl-C to stop)...\n\n", type);
        SDPrintf("%-8s %-28s %-22s %-15s %5s  TXT\n",
            "Event", "Instance", "Type", "Address", "Port");
    }
    while (result < 0 && (why = SDSessionWait(&session)) == SD_WAIT_EVENTS) {
        const struct DNSSDEvent *e;

        while (result < 0 && (e = SDSessionRead(&session, &rc)) != NULL) {
            switch (SDFinderEvent(&finder, e, sv, &rc)) {
            case SD_FOUND_TYPE:
                SDPrintf("%-8s %s\n", "TYPE", sv->sv_Type);
                break;
            case SD_FOUND_NOTYPE:
                SDPrintf("%-8s %s\n", "NO TYPE", sv->sv_Type);
                break;
            case SD_FOUND_ADD:
                if (zone) {
                    DSPrintZone(sv);
                } else {
                    DSPrintService((CONST_STRPTR)"ADD", sv);
                }
                break;
            case SD_FOUND_UPDATE:
                /* what it says changed: addresses, port, host or TXT */
                if (zone) {
                    SDPrintf("; update %s.%s.local.\n", sv->sv_Name, sv->sv_Type);
                    DSPrintZone(sv);
                } else {
                    DSPrintService((CONST_STRPTR)"UPDATE", sv);
                }
                break;
            case SD_FOUND_REMOVE:
                if (zone) {
                    SDPrintf("; remove %s.%s.local.\n", sv->sv_Name, sv->sv_Type);
                } else {
                    DSPrintService((CONST_STRPTR)"REMOVE", sv);
                }
                break;
            case SD_FOUND_END:
                result = DSEnded((CONST_STRPTR)"browse", rc);
                break;
            }
        }
        if (result < 0 && rc != DNSSD_ERR_EMPTY) {
            result = DSEnded((CONST_STRPTR)"browse", rc);
        }
    }
    if (result < 0) {
        SDPrintf("\n(Ctrl-C)\n");
        result = 0;
    }
    SDFinderStop(&finder);
    SDSessionClose(&session);
    FreeVec(sv);
    return result;
}

/*------------------------------------------------------------------------*/
/* -L : resolve one instance. mdnsd asks for its SRV, TXT and address
 * records and reports it resolved, or gives up after five seconds. */

static int
DSResolve(CONST_STRPTR name, CONST_STRPTR type, CONST_STRPTR domain)
{
    struct SDSession session;
    DNSSDOperationRef op;
    LONG rc, why = SD_WAIT_EVENTS;
    int result = -1;

    DSDomainNote(domain);
    if ((rc = SDSessionOpen(&session, 0)) != DNSSD_OK) {
        SDAppPrintError("resolve", rc);
        return 20;
    }
    rc = DNSSD_Resolve(session.bs_Session, name, type, &op, TAG_DONE);
    if (rc != DNSSD_OK) {
        SDSessionClose(&session);
        SDAppPrintError("resolve", rc);
        return 20;
    }
    SDPrintf("Resolving \"%s\" %s (Ctrl-C to stop)...\n", name, type);
    while (result < 0 && (why = SDSessionWait(&session)) == SD_WAIT_EVENTS) {
        const struct DNSSDEvent *e;

        while (result < 0 && (e = SDSessionRead(&session, &rc)) != NULL) {
            if (e->de_Kind == DNSSD_EVENT_RESOLVED) {
                const struct DNSSDAddress *a = (const struct DNSSDAddress *)
                    SDEventPart(e, &e->de_Addresses);
                char target[256];
                ULONG i;

                SDEventName(e, &e->de_Target, target, (LONG)sizeof(target));
                SDPrintf("%-8s %-28s %s:%ld\n", "RESOLVED", name, target,
                    (LONG)e->de_Port);
                for (i = 0; a != NULL && i < e->de_AddressCount; i++) {
                    SDPrintf("         address ");
                    DSPrintAddress(&a[i]);
                    SDPrintf("\n");
                }
                if (e->de_TXT.dr_Size != 0) {
                    SDPrintf("         TXT     ");
                    SDPrintTXT(SDEventPart(e, &e->de_TXT), (LONG)e->de_TXT.dr_Size);
                    SDPrintf("\n");
                }
                result = 0;
            } else if (e->de_Kind == DNSSD_EVENT_END) {
                SDPrintf("Not resolved.\n");
                result = 5;
            }
        }
        if (result < 0 && rc != DNSSD_ERR_EMPTY) {
            result = DSEnded((CONST_STRPTR)"resolve", rc);
        }
    }
    if (result < 0) {
        result = 5;
    }
    DNSSD_Cancel(op);
    SDSessionClose(&session);
    return result;
}

/*------------------------------------------------------------------------*/
/* -G : a bounded host lookup. */

static int
DSLookup(CONST_STRPTR family, CONST_STRPTR host)
{
    struct DNSSDAddress addresses[16];
    ULONG count = 16, i, which;
    char name[256];
    LONG rc;

    if (SDStrEqual(family, "v4")) {
        which = DNSSD_FAMILY_IPV4;
    } else if (SDStrEqual(family, "v6")) {
        which = DNSSD_FAMILY_IPV6;
    } else if (SDStrEqual(family, "v4v6")) {
        which = DNSSD_FAMILY_ANY;
    } else {
        SDPrintf("dnssd -G: v4, v6 or v4v6\n");
        return 5;
    }
    DSLocalName(host, name, (LONG)sizeof(name));
    rc = DNSSD_LookupAddressSync((CONST_STRPTR)name, addresses, &count,
        DNSSD_Family, which, TAG_DONE);
    if (rc != DNSSD_OK) {
        SDAppPrintError("lookup", rc);
        return rc == DNSSD_ERR_TIMEOUT ? 5 : 20;
    }
    for (i = 0; i < count; i++) {
        SDPrintf("%s  ", host);
        DSPrintAddress(&addresses[i]);
        SDPrintf("\n");
    }
    return 0;
}

/*------------------------------------------------------------------------*/
/* -q : a standing record query, stopped by Ctrl-C. */

static int
DSQuery(CONST_STRPTR name, CONST_STRPTR typeWord)
{
    struct SDSession session;
    DNSSDOperationRef op;
    UWORD type = DSRecordType(typeWord);
    char owner[256];
    LONG rc, why;
    int result = -1;

    if (type == 0) {
        SDPrintf("dnssd -q: unknown record type \"%s\"\n", typeWord);
        return 5;
    }
    DSLocalName(name, owner, (LONG)sizeof(owner));
    if ((rc = SDSessionOpen(&session, 0)) != DNSSD_OK) {
        SDAppPrintError("query", rc);
        return 20;
    }
    rc = DNSSD_QueryRecord(session.bs_Session, (CONST_STRPTR)owner, type, &op,
        TAG_DONE);
    if (rc != DNSSD_OK) {
        SDSessionClose(&session);
        SDAppPrintError("query", rc);
        return 20;
    }
    SDPrintf("Asking for %s %s (Ctrl-C to stop)...\n\n", owner, typeWord);
    while (result < 0 && (why = SDSessionWait(&session)) == SD_WAIT_EVENTS) {
        const struct DNSSDEvent *e;

        while (result < 0 && (e = SDSessionRead(&session, &rc)) != NULL) {
            if (e->de_Kind == DNSSD_EVENT_RECORD) {
                DSPrintRecord(e);
            } else if (e->de_Kind == DNSSD_EVENT_END ||
                       e->de_Kind == DNSSD_EVENT_LOST) {
                result = DSEnded((CONST_STRPTR)"query",
                    e->de_Kind == DNSSD_EVENT_LOST ? DNSSD_ERR_BUSY : e->de_Result);
            }
        }
        if (result < 0 && rc != DNSSD_ERR_EMPTY) {
            result = DSEnded((CONST_STRPTR)"query", rc);
        }
    }
    if (result < 0) {
        SDPrintf("\n(Ctrl-C)\n");
        result = 0;
    }
    DNSSD_Cancel(op);
    SDSessionClose(&session);
    return result;
}

/*------------------------------------------------------------------------*/
/* -R / -P : register a service, for this host or for another one. */

static int
DSRegister(CONST_STRPTR name, CONST_STRPTR type, CONST_STRPTR domain,
    UWORD port, CONST_STRPTR proxyHost, const struct DNSSDAddress *proxyAddr,
    int txtArgc, char **txtArgv)
{
    struct SDSession session;
    struct TagItem tags[6];
    DNSSDOperationRef op;
    UBYTE txt[1300];
    char items[400];
    LONG txtLength, rc, why, ttl = 0;
    int nt = 0, result = -1;

    /* "-t seconds" before the text: how long the records may live */
    if (txtArgc >= 2 && SDStrEqual(txtArgv[0], "-t")) {
        if (!SDParseNum(txtArgv[1], &ttl) || ttl < 1 || ttl > 4500) {
            SDPrintf("dnssd: TTL must be 1-4500 seconds.\n");
            return 5;
        }
        txtArgv += 2;
    }
    if (!DSDomainLocal(domain)) {
        SDPrintf("(note: domain \"%s\" is not served; registering in local)\n",
            domain);
    }
    txtLength = SDTXTFromItems(txt, (LONG)sizeof(txt), txtArgv);
    if (txtLength < 0) {
        SDPrintf("dnssd: TXT record is too long, or an escape is not \\xNN.\n");
        return 5;
    }
    SDItemsText(items, (LONG)sizeof(items), txtArgv);

    if (txtLength != 0) {
        tags[nt].ti_Tag = DNSSD_TXT;
        tags[nt++].ti_Data = (IPTR)txt;
        tags[nt].ti_Tag = DNSSD_TXTLength;
        tags[nt++].ti_Data = (IPTR)txtLength;
    }
    if (proxyHost != NULL) {
        tags[nt].ti_Tag = DNSSD_Host;
        tags[nt++].ti_Data = (IPTR)proxyHost;
        tags[nt].ti_Tag = DNSSD_HostAddress;
        tags[nt++].ti_Data = (IPTR)proxyAddr;
    }
    if (ttl != 0) {
        tags[nt].ti_Tag = DNSSD_TimeToLive;
        tags[nt++].ti_Data = (IPTR)ttl;
    }
    tags[nt].ti_Tag = TAG_DONE;
    tags[nt].ti_Data = 0;

    if ((rc = SDSessionOpen(&session, 0)) != DNSSD_OK) {
        SDAppPrintError("register", rc);
        return 20;
    }
    rc = DNSSD_RegisterA(session.bs_Session, name, type, port, &op, tags);
    if (rc != DNSSD_OK) {
        SDSessionClose(&session);
        SDAppPrintError("register", rc);
        return 20;
    }
    while (result < 0 && (why = SDSessionWait(&session)) == SD_WAIT_EVENTS) {
        const struct DNSSDEvent *e;

        while (result < 0 && (e = SDSessionRead(&session, &rc)) != NULL) {
            if (e->de_Kind == DNSSD_EVENT_REGISTERED) {
                char instance[64];
                const UBYTE *label = SDEventPart(e, &e->de_Instance);
                ULONG i;

                for (i = 0; label != NULL && i < e->de_Instance.dr_Size && i < 63; i++) {
                    instance[i] = (char)label[i];
                }
                instance[i] = '\0';
                /* the name it has: another one ("Name (2)") if another
                 * host had it */
                SDPrintf("Registered \"%s\" %s port %ld", instance, type,
                    (LONG)port);
                if (items[0] != '\0') {
                    SDPrintf(" txt=\"%s\"", items);
                }
                SDPrintf("\nPress Ctrl-C to deregister...\n");
            } else if (e->de_Kind == DNSSD_EVENT_END) {
                result = DSEnded((CONST_STRPTR)"register", e->de_Result);
            }
        }
        if (result < 0 && rc != DNSSD_ERR_EMPTY) {
            result = DSEnded((CONST_STRPTR)"register", rc);
        }
    }
    DNSSD_Cancel(op);
    SDSessionClose(&session);
    if (result < 0) {
        SDPrintf("Deregistered.\n");
        result = 0;
    }
    return result;
}

static void
DSUsage(void)
{
    SDPrintf("dnssd - DNS-SD tool (dnssd.library)\n\n");
    SDPrintf("  dnssd -E                      list registration domains\n");
    SDPrintf("  dnssd -F                      list browsing domains\n");
    SDPrintf("  dnssd -R name type dom port [-t seconds] [k=v ...]  register a service\n");
    SDPrintf("  dnssd -P name type dom port host ip [k=v ...]  register for another host\n");
    SDPrintf("                                (ip: IPv4, or IPv6 - fe80::1%%2 with its interface)\n");
    SDPrintf("  dnssd -B type [domain]        browse a service type\n");
    SDPrintf("  dnssd -L name type [domain]   look up (resolve) an instance\n");
    SDPrintf("  dnssd -Z type [domain]        browse, complete zone records\n");
    SDPrintf("  dnssd -q name rrtype          ask for any DNS record\n");
    SDPrintf("  dnssd -G v4|v6|v4v6 hostname  look up a host's addresses\n");
    SDPrintf("  dnssd -V                      show library version\n");
    SDPrintf("  dnssd -h                      this help\n\n");
    SDPrintf("Examples:\n");
    SDPrintf("  dnssd -B _http._tcp\n");
    SDPrintf("  dnssd -B _services._dns-sd._udp     (list service types)\n");
    SDPrintf("  dnssd -L \"My Mac\" _companion-link._tcp\n");
    SDPrintf("  dnssd -q _http._tcp.local PTR\n");
    SDPrintf("  dnssd -G v4 printer.local\n");
    SDPrintf("  dnssd -R AmigaShare _http._tcp . 80 path=/ vers=1\n");
}

int
main(int argc, char **argv)
{
    struct DNSSDAddress proxyAddr;
    LONG port = 0;
    int result;

    if (argc < 2) {
        DSUsage();
        return 5;
    }
    if (SDStrEqual(argv[1], "-h") || SDStrEqual(argv[1], "-?")) {
        DSUsage();
        return 0;
    }
    if ((SDStrEqual(argv[1], "-B") || SDStrEqual(argv[1], "-Z")) &&
        argc < 3) {
        SDPrintf("dnssd %s needs a <type>\n", argv[1]);
        return 5;
    }
    if (SDStrEqual(argv[1], "-L") && argc < 4) {
        SDPrintf("dnssd -L needs <name> <type> [domain]\n");
        return 5;
    }
    if (SDStrEqual(argv[1], "-q") && argc < 4) {
        SDPrintf("dnssd -q needs <name> <rrtype>\n");
        return 5;
    }
    if (SDStrEqual(argv[1], "-G") && argc < 4) {
        SDPrintf("dnssd -G needs v4|v6|v4v6 <hostname>\n");
        return 5;
    }
    if (SDStrEqual(argv[1], "-R") && argc < 6) {
        SDPrintf("dnssd -R needs <name> <type> <domain> <port>\n");
        return 5;
    }
    if (SDStrEqual(argv[1], "-P") &&
        (argc < 8 || !SDParseAddress(argv[7], &proxyAddr))) {
        SDPrintf("dnssd -P needs <name> <type> <domain> <port> <host> <ip>\n");
        return 5;
    }
    if ((SDStrEqual(argv[1], "-R") || SDStrEqual(argv[1], "-P")) &&
        (!SDParseNum(argv[5], &port) || port < 1 || port > 65535)) {
        SDPrintf("dnssd: port must be 1-65535.\n");
        return 5;
    }
    if (!SDStrEqual(argv[1], "-B") && !SDStrEqual(argv[1], "-Z") &&
        !SDStrEqual(argv[1], "-L") && !SDStrEqual(argv[1], "-R") &&
        !SDStrEqual(argv[1], "-P") && !SDStrEqual(argv[1], "-E") &&
        !SDStrEqual(argv[1], "-F") && !SDStrEqual(argv[1], "-V") &&
        !SDStrEqual(argv[1], "-q") && !SDStrEqual(argv[1], "-G")) {
        SDPrintf("Unknown mode: %s\n", argv[1]);
        DSUsage();
        return 5;
    }

    if (!SDAppOpen()) {
        SDPrintf("Cannot open %s version %ld\n", DNSSDNAME, (LONG)DNSSD_VERSION);
        return 20;
    }
    if (SDStrEqual(argv[1], "-V")) {
        result = DSVersion();
    } else if (SDStrEqual(argv[1], "-E")) {
        result = DSEnumDomains((CONST_STRPTR)"registration");
    } else if (SDStrEqual(argv[1], "-F")) {
        result = DSEnumDomains((CONST_STRPTR)"browsing");
    } else if (SDStrEqual(argv[1], "-B") || SDStrEqual(argv[1], "-Z")) {
        result = DSBrowse((CONST_STRPTR)argv[2],
            argc > 3 ? (CONST_STRPTR)argv[3] : NULL,
            SDStrEqual(argv[1], "-Z"));
    } else if (SDStrEqual(argv[1], "-L")) {
        result = DSResolve((CONST_STRPTR)argv[2], (CONST_STRPTR)argv[3],
            argc > 4 ? (CONST_STRPTR)argv[4] : NULL);
    } else if (SDStrEqual(argv[1], "-q")) {
        result = DSQuery((CONST_STRPTR)argv[2], (CONST_STRPTR)argv[3]);
    } else if (SDStrEqual(argv[1], "-G")) {
        result = DSLookup((CONST_STRPTR)argv[2], (CONST_STRPTR)argv[3]);
    } else if (SDStrEqual(argv[1], "-P")) {
        result = DSRegister((CONST_STRPTR)argv[2], (CONST_STRPTR)argv[3],
            (CONST_STRPTR)argv[4], (UWORD)port,
            (CONST_STRPTR)argv[6], &proxyAddr,
            argc - 8, &argv[8]);
    } else {
        result = DSRegister((CONST_STRPTR)argv[2], (CONST_STRPTR)argv[3],
            (CONST_STRPTR)argv[4], (UWORD)port, NULL, NULL,
            argc - 6, &argv[6]);
    }
    SDAppClose();
    return result;
}
