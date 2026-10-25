/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * dnssd - a DNS-SD command-line tool in the spirit of Apple's dns-sd and
 *         Avahi's avahi-browse, on bonami.library.
 *
 *   dnssd -E                              list domains for registering
 *   dnssd -F                              list domains for browsing
 *   dnssd -R name type domain port [-t seconds] [k=v ...]
 *                                         register (advertise) a service
 *   dnssd -P name type domain port host ip [k=v ...]
 *                                         register for another host
 *   dnssd -B type [domain]                browse for instances of a type
 *   dnssd -L name type [domain]           look up (resolve) one instance
 *   dnssd -Z type [domain]                browse, as available zone records
 *   dnssd -V                              show the bonami.library version
 *   dnssd -h                              this help
 *
 * The domain may be omitted or given as "." for the default, which is the
 * only one bonami.library serves: the multicast "local" domain.
 *
 * -R, -P, -B and -Z run until Ctrl-C; -L until the instance is resolved or
 * mdnsd gives up on it, five seconds later. Browse and resolve use queued
 * discovery (DISCOVERY.md): the events are read in this task, which sleeps
 * on a request lent to mdnsd until there are some.
 *
 *   dnssd -B _http._tcp
 *   dnssd -B _services._dns-sd._udp     (the service types on the link)
 *   dnssd -L "My Mac" _companion-link._tcp
 *   dnssd -R AmigaShare _http._tcp . 80 path=/ vers=1
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <libraries/bonamibase.h>
#include <bonami/mdns.h>
#include <proto/bonami.h>

#include "ba_common.h"

static const char version_tag[] __attribute__((used)) =
    "\0$VER: dnssd 41.1 (22.10.2026)";

/*------------------------------------------------------------------------*/
/* Helpers */

/*
 * bonami.library serves only the multicast "local" domain. dns-sd's
 * spellings of the default ("", ".") and "local"/"local." are taken; for
 * anything else a note is printed and "local" is used, so that a script is
 * not misled.
 */
static int
DSDomainLocal(CONST_STRPTR domain)
{
    if (domain == NULL) {
        return 1;
    }
    return BAStrEqual((const char *)domain, "") ||
           BAStrEqual((const char *)domain, ".") ||
           BAStrEqual((const char *)domain, "local") ||
           BAStrEqual((const char *)domain, "local.");
}

static void
DSDomainNote(CONST_STRPTR domain)
{
    if (!DSDomainLocal(domain)) {
        BAPrintf("(note: domain \"%s\" is not served; using local)\n", domain);
    }
}

/* What a watch ended with, said the way a user can act on. */
static int
DSWatchEnded(struct BAWatch *watch, CONST_STRPTR what)
{
    if (watch->bw_Result == MDNSERR_BUSY) {
        BAPrintf("\n%s: more answers came than could be kept; run it again.\n",
            what);
    } else if (watch->bw_Result == MDNSERR_NOENGINE) {
        BAPrintf("\n%s: mdnsd ended.\n", what);
    } else {
        BAAppPrintMDNSError(what, watch->bw_Result);
    }
    return 20;
}

/*------------------------------------------------------------------------*/
/* -E / -F : bonami.library serves only the link-local mDNS domain, so both
 * lists hold "local" alone. */

static int
DSEnumDomains(CONST_STRPTR which)
{
    BAPrintf("Domains for %s:\n", which);
    BAPrintf("  Added     local\n");
    BAPrintf("\n(bonami.library serves only the multicast local domain)\n");
    return 0;
}

/*------------------------------------------------------------------------*/
/* -V : library version. */

static int
DSVersion(void)
{
    struct Library *lib = (struct Library *)BonamiBase;

    BAPrintf("%s %ld.%ld\n", BONAMINAME,
        (LONG)lib->lib_Version, (LONG)lib->lib_Revision);
    return 0;
}

/*------------------------------------------------------------------------*/
/* -B / -Z : browse. Zone output includes only records the queued event can
 * represent faithfully. It has the instance address and port but not the SRV
 * target host name, so inventing an SRV target and an A owner would produce
 * incorrect DNS-SD data. Those values are reported in a comment instead. */

static void
DSPrintZone(const struct MDNSDiscoveryEvent *ev)
{
    LONG length = ev->de_TXTLength;

    BAPrintf("%s.local.\tPTR\t%s.%s.local.\n", ev->de_Type, ev->de_Name,
        ev->de_Type);
    BAPrintf("; %s.%s.local.: port %ld", ev->de_Name, ev->de_Type,
        (LONG)ev->de_Port);
    if (ev->de_Address != 0) {
        BAPrintf(", address %ld.%ld.%ld.%ld", BA_IP_ARGS(ev->de_Address));
    }
    BAPrintf("; SRV target unavailable\n");
    if (length > 0) {
        if (length > MDNS_DISCOVERY_TXT_MAX) {
            length = MDNS_DISCOVERY_TXT_MAX;
        }
        BAPrintf("%s.%s.local.\tTXT\t", ev->de_Name, ev->de_Type);
        BAPrintTXT(ev->de_TXT, length);
        BAPrintf("\n");
    }
}

static int
DSBrowse(CONST_STRPTR type, CONST_STRPTR domain, int zone)
{
    MDNSBrowseRef ref = NULL;
    struct TagItem tags[] = {
        { MDNS_EventMode, MDNS_EVENT_QUEUE },
        { MDNS_BrowseRef, (IPTR)&ref },
        { TAG_END, 0 }
    };
    struct BAWatch watch;
    struct MDNSDiscoveryEvent ev;
    LONG rc;
    LONG why;
    int result = 0;

    DSDomainNote(domain);
    rc = BrowseServiceTypeA((STRPTR)type, NULL, tags);
    if (rc != MDNS_OK) {
        BAAppPrintMDNSError("BrowseServiceTypeA", rc);
        return 20;
    }
    rc = BAWatchStart(&watch, ref, 0);
    if (rc != MDNS_OK) {
        StopBrowse(ref);
        BAAppPrintMDNSError("RequestMDNSEvent", rc);
        return 20;
    }
    if (zone) {
        BAPrintf("; browsing %s (Ctrl-C to stop)\n\n", type);
    } else {
        BAPrintf("Browsing %s (Ctrl-C to stop)...\n\n", type);
        BAPrintf("%-8s %-28s %-22s %-15s %5s  TXT\n",
            "Event", "Instance", "Type", "Address", "Port");
    }
    while ((why = BAWatchWait(&watch)) == BA_WAIT_EVENTS) {
        while (ReadMDNSEvent(ref, &ev) == MDNS_OK) {
            if (ev.de_Name[0] == '\0') {
                /* type enumeration: a type, not an instance */
                if (ev.de_Code == MEVENT_ADD || ev.de_Code == MEVENT_REMOVE) {
                    BAPrintf("%-8s %s\n", (CONST_STRPTR)(ev.de_Code ==
                        MEVENT_ADD ? "TYPE" : "NO TYPE"), ev.de_Type);
                }
            } else if (ev.de_Code == MEVENT_ADD) {
                if (zone) {
                    DSPrintZone(&ev);
                } else {
                    BAPrintEvent((CONST_STRPTR)"ADD", &ev);
                }
            } else if (ev.de_Code == MEVENT_REMOVE) {
                if (zone) {
                    BAPrintf("; remove %s.%s.local.\n", ev.de_Name,
                        ev.de_Type);
                } else {
                    BAPrintEvent((CONST_STRPTR)"REMOVE", &ev);
                }
            }
        }
    }
    if (why == BA_WAIT_END) {
        result = DSWatchEnded(&watch, (CONST_STRPTR)"browse");
    } else {
        BAPrintf("\n(Ctrl-C)\n");
    }
    StopBrowse(ref);
    BAWatchEnd(&watch);
    return result;
}

/*------------------------------------------------------------------------*/
/* -L : resolve one instance. mdnsd asks for its SRV, TXT and address
 * records and reports it resolved, or failed after five seconds. */

static int
DSResolve(CONST_STRPTR name, CONST_STRPTR type, CONST_STRPTR domain)
{
    MDNSResolveRef ref = NULL;
    struct TagItem tags[] = {
        { MDNS_EventMode, MDNS_EVENT_QUEUE },
        { MDNS_ResolveRef, (IPTR)&ref },
        { TAG_END, 0 }
    };
    struct BAWatch watch;
    struct MDNSDiscoveryEvent ev;
    LONG rc;
    LONG why = BA_WAIT_EVENTS;
    int result = -1;

    DSDomainNote(domain);
    rc = ResolveServiceA((STRPTR)name, (STRPTR)type, NULL, tags);
    if (rc != MDNS_OK) {
        BAAppPrintMDNSError("ResolveServiceA", rc);
        return 20;
    }
    rc = BAWatchStart(&watch, ref, 0);
    if (rc != MDNS_OK) {
        StopResolve(ref);
        BAAppPrintMDNSError("RequestMDNSEvent", rc);
        return 20;
    }
    BAPrintf("Resolving \"%s\" %s (Ctrl-C to stop)...\n", name, type);
    while (result < 0 && (why = BAWatchWait(&watch)) == BA_WAIT_EVENTS) {
        while (result < 0 && ReadMDNSEvent(ref, &ev) == MDNS_OK) {
            if (ev.de_Code == MEVENT_RESOLVED) {
                BAPrintEvent((CONST_STRPTR)"RESOLVED", &ev);
                result = 0;
            } else if (ev.de_Code == MEVENT_RESOLVE_FAILED) {
                BAPrintf("Not resolved.\n");
                result = 5;
            }
        }
    }
    if (result < 0) {
        result = why == BA_WAIT_END ?
            DSWatchEnded(&watch, (CONST_STRPTR)"resolve") : 5;
    }
    StopResolve(ref);
    BAWatchEnd(&watch);
    return result;
}

/*------------------------------------------------------------------------*/
/* -R / -P : register a service, for this host or for another one. */

static int
DSRegister(CONST_STRPTR name, CONST_STRPTR type, CONST_STRPTR domain,
    UWORD port, CONST_STRPTR proxyHost, ULONG proxyAddr,
    int txtArgc, char **txtArgv)
{
    struct TagItem tags[10];
    char txt[MDNS_MAX_TXT + 1];
    MDNSRegisterRef ref = NULL;
    LONG rc;
    LONG ttl = 0;
    BOOL haveTtl = FALSE;
    int nt = 0;

    /* "-t seconds" before the text: how long the records may live */
    if (txtArgc >= 2 && BAStrEqual(txtArgv[0], "-t")) {
        if (!BAParseNum(txtArgv[1], &ttl) || ttl < 1 || ttl > 4500) {
            BAPrintf("dnssd: TTL must be 1-4500 seconds.\n");
            return 5;
        }
        haveTtl = TRUE;
        txtArgv += 2;
    }
    if (!DSDomainLocal(domain)) {
        BAPrintf("(note: domain \"%s\" is not served; registering in local)\n",
            domain);
    }

    if (!BAEncodeTXT(txt, (LONG)sizeof(txt), txtArgv)) {
        BAPrintf("dnssd: TXT record is too long.\n");
        return 5;
    }

    tags[nt].ti_Tag = MDNS_ServiceName;
    tags[nt++].ti_Data = (IPTR)name;
    tags[nt].ti_Tag = MDNS_ServiceType;
    tags[nt++].ti_Data = (IPTR)type;
    tags[nt].ti_Tag = MDNS_Port;
    tags[nt++].ti_Data = (IPTR)port;
    if (txt[0] != '\0') {
        tags[nt].ti_Tag = MDNS_TXTRecord;
        tags[nt++].ti_Data = (IPTR)txt;
    }
    if (proxyHost != NULL) {
        tags[nt].ti_Tag = MDNS_HostName;
        tags[nt++].ti_Data = (IPTR)proxyHost;
        tags[nt].ti_Tag = MDNS_HostAddr;
        tags[nt++].ti_Data = (IPTR)proxyAddr;
    }
    if (haveTtl) {
        tags[nt].ti_Tag = MDNS_TimeToLive;
        tags[nt++].ti_Data = (IPTR)ttl;
    }
    tags[nt].ti_Tag = MDNS_RegisterRef;
    tags[nt++].ti_Data = (IPTR)&ref;
    tags[nt].ti_Tag = TAG_END;
    tags[nt].ti_Data = 0;

    rc = RegisterServiceA(tags);
    if (rc != MDNS_OK) {
        BAAppPrintMDNSError("RegisterServiceA", rc);
        return 20;
    }
    BAPrintf("Registered \"%s\" %s port %ld", name, type, (LONG)port);
    if (txt[0] != '\0') {
        BAPrintf(" txt=\"%s\"", txt);
    }
    BAPrintf("\nPress Ctrl-C to deregister...\n");
    Wait(SIGBREAKF_CTRL_C);
    DeregisterService(ref);
    BAPrintf("Deregistered.\n");
    return 0;
}

/* "192.168.1.20" as an address in network byte order; 0 if it is not one. */
static ULONG
DSParseAddr(const char *s)
{
    ULONG addr = 0;
    UBYTE *b = (UBYTE *)&addr;
    LONG part;

    for (part = 0; part < 4; part++) {
        LONG value = 0;

        if (*s < '0' || *s > '9') {
            return 0;
        }
        for (; *s >= '0' && *s <= '9'; s++) {
            value = value * 10 + (*s - '0');
            if (value > 255) {
                return 0;
            }
        }
        if (part < 3 && *s++ != '.') {
            return 0;
        }
        b[part] = (UBYTE)value;
    }
    return *s == '\0' ? addr : 0;
}

static void
DSUsage(void)
{
    BAPrintf("dnssd - DNS-SD tool (bonami.library)\n\n");
    BAPrintf("  dnssd -E                      list registration domains\n");
    BAPrintf("  dnssd -F                      list browsing domains\n");
    BAPrintf("  dnssd -R name type dom port [-t seconds] [k=v ...]  register a service\n");
    BAPrintf("  dnssd -P name type dom port host ip [k=v ...]  register for another host\n");
    BAPrintf("  dnssd -B type [domain]        browse a service type\n");
    BAPrintf("  dnssd -L name type [domain]   look up (resolve) an instance\n");
    BAPrintf("  dnssd -Z type [domain]        browse, available zone records\n");
    BAPrintf("  dnssd -V                      show library version\n");
    BAPrintf("  dnssd -h                      this help\n\n");
    BAPrintf("Examples:\n");
    BAPrintf("  dnssd -B _http._tcp\n");
    BAPrintf("  dnssd -B _services._dns-sd._udp     (list service types)\n");
    BAPrintf("  dnssd -L \"My Mac\" _companion-link._tcp\n");
    BAPrintf("  dnssd -R AmigaShare _http._tcp . 80 path=/ vers=1\n");
}

/*
 * The modes that need no engine (-E, -F, -V, -h) are answered before it is
 * opened; the others open the shared engine.
 */
int
main(int argc, char **argv)
{
    LONG rc;
    LONG port = 0;
    int result;

    if (argc < 2) {
        DSUsage();
        return 5;
    }
    if (BAStrEqual(argv[1], "-h") || BAStrEqual(argv[1], "-?")) {
        DSUsage();
        return 0;
    }
    if ((BAStrEqual(argv[1], "-B") || BAStrEqual(argv[1], "-Z")) &&
        argc < 3) {
        BAPrintf("dnssd %s needs a <type>\n", argv[1]);
        return 5;
    }
    if (BAStrEqual(argv[1], "-L") && argc < 4) {
        BAPrintf("dnssd -L needs <name> <type> [domain]\n");
        return 5;
    }
    if (BAStrEqual(argv[1], "-R") && argc < 6) {
        BAPrintf("dnssd -R needs <name> <type> <domain> <port>\n");
        return 5;
    }
    if (BAStrEqual(argv[1], "-P") &&
        (argc < 8 || DSParseAddr(argv[7]) == 0)) {
        BAPrintf("dnssd -P needs <name> <type> <domain> <port> <host> <ip>\n");
        return 5;
    }
    if ((BAStrEqual(argv[1], "-R") || BAStrEqual(argv[1], "-P")) &&
        (!BAParseNum(argv[5], &port) || port < 1 || port > 65535)) {
        BAPrintf("dnssd: port must be 1-65535.\n");
        return 5;
    }
    if (!BAStrEqual(argv[1], "-B") && !BAStrEqual(argv[1], "-Z") &&
        !BAStrEqual(argv[1], "-L") && !BAStrEqual(argv[1], "-R") &&
        !BAStrEqual(argv[1], "-P") && !BAStrEqual(argv[1], "-E") &&
        !BAStrEqual(argv[1], "-F") && !BAStrEqual(argv[1], "-V")) {
        BAPrintf("Unknown mode: %s\n", argv[1]);
        DSUsage();
        return 5;
    }

    if (!BAAppOpen()) {
        BAPrintf("Cannot open %s version %ld\n", BONAMINAME,
            (LONG)BONAMI_LIB_VERSION);
        return 20;
    }
    if (BAStrEqual(argv[1], "-V")) {
        result = DSVersion();
    } else if (BAStrEqual(argv[1], "-E")) {
        result = DSEnumDomains((CONST_STRPTR)"registration");
    } else if (BAStrEqual(argv[1], "-F")) {
        result = DSEnumDomains((CONST_STRPTR)"browsing");
    } else if ((rc = BAAppOpenEngine()) != MDNS_OK) {
        BAAppPrintMDNSError("OpenMDNSEngine", rc);
        result = 20;
    } else if (BAStrEqual(argv[1], "-B") || BAStrEqual(argv[1], "-Z")) {
        result = DSBrowse((CONST_STRPTR)argv[2],
            argc > 3 ? (CONST_STRPTR)argv[3] : NULL,
            BAStrEqual(argv[1], "-Z"));
    } else if (BAStrEqual(argv[1], "-L")) {
        result = DSResolve((CONST_STRPTR)argv[2], (CONST_STRPTR)argv[3],
            argc > 4 ? (CONST_STRPTR)argv[4] : NULL);
    } else if (BAStrEqual(argv[1], "-P")) {
        result = DSRegister((CONST_STRPTR)argv[2], (CONST_STRPTR)argv[3],
            (CONST_STRPTR)argv[4], (UWORD)port,
            (CONST_STRPTR)argv[6], DSParseAddr(argv[7]),
            argc - 8, &argv[8]);
    } else {
        result = DSRegister((CONST_STRPTR)argv[2], (CONST_STRPTR)argv[3],
            (CONST_STRPTR)argv[4], (UWORD)port, NULL, 0,
            argc - 6, &argv[6]);
    }
    BAAppClose();
    return result;
}
