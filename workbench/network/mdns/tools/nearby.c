/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * Nearby - see what services are on your local network, in plain English.
 *
 * A friendly Amiga-style front end to bonami.library's mDNS/DNS-SD browse.
 * Uses a standard ReadArgs template and a service-type alias table so nobody
 * has to type "_afpovertcp._tcp".
 *
 *   Nearby [TYPE] [ALL] [DOMAIN <d>] [TIME <secs>] [RAW] [PORTS]
 *
 *   TYPE    a plain word ("web", "printer", "files", "ssh", ...) or a real
 *           DNS-SD type ("_http._tcp").  Omit it (or say ALL) to list the
 *           kinds of service present on the network instead of instances.
 *   ALL     list the service kinds on the network (not individual services).
 *   DOMAIN  network domain to search (default: your local network).
 *   TIME    how many seconds to look; omit to keep looking until Ctrl-C.
 *   RAW     also show the underlying DNS-SD type strings.
 *   PORTS   show each service's host and port (and any extra info).
 *
 * Examples:
 *   Nearby                 ; what kinds of things are out there?
 *   Nearby web             ; who has a web server?
 *   Nearby printer PORTS   ; printers, with host and port
 *   Nearby ssh TIME 10     ; look for SSH logins for 10 seconds
 *
 * Without a TYPE, or with ALL, it lists the kinds of service present: a
 * browse of _services._dns-sd._udp, which mdnsd answers with the types on
 * the link (RFC 6763 section 9).
 *
 * The browse is queued (DISCOVERY.md): Nearby sleeps on a request lent to
 * mdnsd and reads the events in its own task.
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <bonami/mdns.h>
#include <proto/bonami.h>

#include "ba_common.h"
#include "ba_types.h"

#define ARG_TYPE    0
#define ARG_ALL     1
#define ARG_DOMAIN  2
#define ARG_TIME    3
#define ARG_RAW     4
#define ARG_PORTS   5
#define ARG_COUNT   6

static const char *NearbyTemplate = "TYPE,ALL/S,DOMAIN/K,TIME/N/K,RAW/S,PORTS/S";
static const char version_tag[] __attribute__((used)) = "\0$VER: Nearby 41.1 (22.10.2026)";

static void
NearbyPrintKind(const struct MDNSDiscoveryEvent *ev, BOOL raw)
{
    if (ev->de_Code == MEVENT_REMOVE) {
        BAPrintf("  gone: %s\n", BATypeFriendly((CONST_STRPTR)ev->de_Type));
    } else if (raw) {
        BAPrintf("  %-26s  %s\n", BATypeFriendly((CONST_STRPTR)ev->de_Type),
            ev->de_Type);
    } else {
        BAPrintf("  %s\n", BATypeFriendly((CONST_STRPTR)ev->de_Type));
    }
}

static void
NearbyPrint(const struct MDNSDiscoveryEvent *ev, BOOL raw, BOOL ports)
{
    LONG length = ev->de_TXTLength;

    if (ev->de_Code == MEVENT_REMOVE) {
        BAPrintf("  gone: %s\n", ev->de_Name);
        return;
    }
    if (!ports) {
        if (raw) {
            BAPrintf("  %-28s  (%s)\n", ev->de_Name, ev->de_Type);
        } else {
            BAPrintf("  %s\n", ev->de_Name);
        }
        return;
    }
    BAPrintf("  %-26s port %ld", ev->de_Name, (LONG)ev->de_Port);
    if (ev->de_Address != 0) {
        BAPrintf("  (%ld.%ld.%ld.%ld)", BA_IP_ARGS(ev->de_Address));
    }
    BAPrintf("\n");
    if (length > 0) {
        if (length > MDNS_DISCOVERY_TXT_MAX) {
            length = MDNS_DISCOVERY_TXT_MAX;
        }
        BAPrintf("      info: ");
        BAPrintTXT(ev->de_TXT, length);
        BAPrintf("\n");
    }
}

static int
NearbyRun(CONST_STRPTR word, BOOL all, LONG timeSecs, BOOL raw, BOOL ports)
{
    MDNSBrowseRef ref = NULL;
    struct TagItem tags[] = {
        { MDNS_EventMode, MDNS_EVENT_QUEUE },
        { MDNS_BrowseRef, (IPTR)&ref },
        { TAG_END, 0 }
    };
    struct BAWatch watch;
    struct MDNSDiscoveryEvent ev;
    BOOL kinds = all || word == NULL;
    CONST_STRPTR type = kinds ? (CONST_STRPTR)MDNS_TYPE_SERVICES_DNS_SD :
        BATypeFromAlias(word);
    LONG rc;
    LONG why;
    int result = 0;

    rc = BrowseServiceTypeA((STRPTR)type, NULL, tags);
    if (rc != MDNS_OK) {
        BAAppPrintMDNSError("browse", rc);
        return 20;
    }
    rc = BAWatchStart(&watch, ref, timeSecs);
    if (rc != MDNS_OK) {
        StopBrowse(ref);
        BAAppPrintMDNSError("browse", rc);
        return 20;
    }
    if (kinds) {
        BAPrintf("Kinds of service on your local network");
    } else {
        BAPrintf("Looking for %s", BATypeFriendly(type));
    }
    if (timeSecs > 0) {
        BAPrintf(" (%ld seconds)...\n\n", timeSecs);
    } else {
        BAPrintf(" (Ctrl-C to stop)...\n\n");
    }
    while ((why = BAWatchWait(&watch)) == BA_WAIT_EVENTS) {
        while (ReadMDNSEvent(ref, &ev) == MDNS_OK) {
            if (ev.de_Code != MEVENT_ADD && ev.de_Code != MEVENT_REMOVE) {
                continue;
            }
            if (kinds) {
                NearbyPrintKind(&ev, raw);
            } else {
                NearbyPrint(&ev, raw, ports);
            }
        }
    }
    if (why == BA_WAIT_END) {
        BAAppPrintMDNSError("browse", watch.bw_Result);
        result = 20;
    } else {
        BAPrintf("\nDone.\n");
    }
    StopBrowse(ref);
    BAWatchEnd(&watch);
    return result;
}

int
main(void)
{
    struct RDArgs *rda;
    IPTR args[ARG_COUNT];
    CONST_STRPTR word;
    LONG timeSecs = 0;
    LONG rc;
    int result;
    int i;

    for (i = 0; i < ARG_COUNT; i++) {
        args[i] = 0;
    }
    rda = ReadArgs((STRPTR)NearbyTemplate, args, NULL);
    if (rda == NULL) {
        PrintFault(IoErr(), (CONST_STRPTR)"Nearby");
        return 20;
    }
    word = (CONST_STRPTR)args[ARG_TYPE];
    if (args[ARG_TIME] != 0) {
        timeSecs = *((LONG *)args[ARG_TIME]);
    }
    if (!BAAppOpen()) {
        BAPrintf("Cannot open %s version %ld.\n", BONAMINAME,
            (LONG)BONAMI_LIB_VERSION);
        FreeArgs(rda);
        return 20;
    }
    rc = BAAppOpenEngine();
    if (rc != MDNS_OK) {
        BAAppPrintMDNSError("start mDNS", rc);
        result = 20;
    } else {
        result = NearbyRun(word, args[ARG_ALL] != 0, timeSecs,
            args[ARG_RAW] != 0, args[ARG_PORTS] != 0);
    }
    BAAppClose();
    FreeArgs(rda);
    return result;
}
