/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * Handshake - find out how to reach one named service on the local network.
 *
 * You already know a service's name (e.g. from Nearby); Handshake resolves it
 * to the host, IP address and port you need to connect, plus any extra info the
 * service advertises.
 *
 *   Handshake NAME/A TYPE/A [DOMAIN <d>] [TIME <secs>]
 *
 *   NAME    the service's name, e.g. "Tim's Mac" (quote names with spaces).
 *   TYPE    a plain word ("web", "files", "ssh", ...) or a real DNS-SD type.
 *   DOMAIN  network domain (default: your local network).
 *   TIME    how many seconds to keep trying (default 30).
 *
 * Examples:
 *   Handshake "Tim's Mac" web
 *   Handshake AmigaShare files
 *
 * The resolve is queued (DISCOVERY.md): mdnsd reports the instance resolved,
 * or failed after five seconds, and Handshake asks again until TIME is up.
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

#define ARG_NAME    0
#define ARG_TYPE    1
#define ARG_DOMAIN  2
#define ARG_TIME    3
#define ARG_COUNT   4

static const char *HandshakeTemplate = "NAME/A,TYPE/A,DOMAIN/K,TIME/N/K";

static void
HandshakeReport(const struct MDNSDiscoveryEvent *ev)
{
    LONG length = ev->de_TXTLength;

    BAPrintf("\nReached \"%s\":\n", ev->de_Name);
    if (ev->de_Address != 0) {
        BAPrintf("  IP:    %ld.%ld.%ld.%ld\n", BA_IP_ARGS(ev->de_Address));
    }
    BAPrintf("  Port:  %ld\n", (LONG)ev->de_Port);
    if (length > 0) {
        if (length > MDNS_DISCOVERY_TXT_MAX) {
            length = MDNS_DISCOVERY_TXT_MAX;
        }
        BAPrintf("  Info:  ");
        BAPrintTXT(ev->de_TXT, length);
        BAPrintf("\n");
    }
}

/* One resolve: 0 resolved, 5 not, 10 stopped by Ctrl-C, 20 an error. */
static int
HandshakeOnce(CONST_STRPTR name, CONST_STRPTR type)
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

    rc = ResolveServiceA((STRPTR)name, (STRPTR)type, NULL, tags);
    if (rc != MDNS_OK) {
        BAAppPrintMDNSError("contact", rc);
        return 20;
    }
    rc = BAWatchStart(&watch, ref, 0);
    if (rc != MDNS_OK) {
        StopResolve(ref);
        BAAppPrintMDNSError("contact", rc);
        return 20;
    }
    while (result < 0 && (why = BAWatchWait(&watch)) == BA_WAIT_EVENTS) {
        while (result < 0 && ReadMDNSEvent(ref, &ev) == MDNS_OK) {
            if (ev.de_Code == MEVENT_RESOLVED) {
                HandshakeReport(&ev);
                result = 0;
            } else if (ev.de_Code == MEVENT_RESOLVE_FAILED) {
                result = 5;
            }
        }
    }
    if (result < 0) {
        if (why == BA_WAIT_BREAK) {
            result = 10;
        } else {
            BAAppPrintMDNSError("contact", watch.bw_Result);
            result = 20;
        }
    }
    StopResolve(ref);
    BAWatchEnd(&watch);
    return result;
}

static int
HandshakeRun(CONST_STRPTR name, CONST_STRPTR word, LONG timeSecs)
{
    CONST_STRPTR type = BATypeFromAlias(word);
    LONG attempts = ((timeSecs > 0 ? timeSecs : 30) + 4) / 5;
    int result = 5;

    BAPrintf("Contacting \"%s\" (%s)...\n", name, BATypeFriendly(type));
    while (attempts-- > 0 && (result = HandshakeOnce(name, type)) == 5) {
        ;
    }
    if (result == 5) {
        BAPrintf("\nCould not reach \"%s\".  Is it switched on and "
            "advertising?\n", name);
    }
    return result == 10 ? 5 : result;
}

int
main(void)
{
    struct RDArgs *rda;
    IPTR args[ARG_COUNT];
    LONG timeSecs = 0;
    LONG rc;
    int result;
    int i;

    for (i = 0; i < ARG_COUNT; i++) {
        args[i] = 0;
    }
    rda = ReadArgs((STRPTR)HandshakeTemplate, args, NULL);
    if (rda == NULL) {
        PrintFault(IoErr(), (CONST_STRPTR)"Handshake");
        return 20;
    }
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
        result = HandshakeRun((CONST_STRPTR)args[ARG_NAME],
            (CONST_STRPTR)args[ARG_TYPE], timeSecs);
    }
    BAAppClose();
    FreeArgs(rda);
    return result;
}
