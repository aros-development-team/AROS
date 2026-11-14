/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * Nearby - see what services are on your local network, in plain English,
 *          on dnssd.library.
 */

/******************************************************************************

    NAME

        Nearby [TYPE] [ALL] [DOMAIN <d>] [TIME <secs>] [RAW] [PORTS]

    SYNOPSIS

        TYPE,ALL/S,DOMAIN/K,TIME/N/K,RAW/S,PORTS/S

    LOCATION

        C:

    FUNCTION

        Lists the services of a kind on the local network as they come, change
        (shown again) and go, or, without a TYPE, the kinds of service there are (a browse of
        "_services._dns-sd._udp", RFC 6763 section 9). A plain word stands
        for the service type, so that nobody has to type
        "_afpovertcp._tcp".

    INPUTS

        TYPE    a plain word ("web", "printer", "files", "ssh", ...) or a
                real DNS-SD type ("_http._tcp")
        ALL     list the kinds of service on the network, not the services
        DOMAIN  network domain to search (default: your local network)
        TIME    how many seconds to look; without it, until Ctrl-C
        RAW     also show the DNS-SD type strings
        PORTS   show each service's port, addresses (IPv4 and IPv6) and
                extra information

    RESULT

        0, or 20 for an error.

    EXAMPLE

        Nearby                  ; what kinds of things are out there?
        Nearby web              ; who has a web server?
        Nearby printer PORTS    ; printers, with address and port
        Nearby ssh TIME 10      ; look for SSH logins for 10 seconds

    SEE ALSO

        Share, Handshake, dnssd

******************************************************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dnssd.h>
#include <utility/tagitem.h>

#include "sd_common.h"
#include "sd_types.h"

#define ARG_TYPE    0
#define ARG_ALL     1
#define ARG_DOMAIN  2
#define ARG_TIME    3
#define ARG_RAW     4
#define ARG_PORTS   5
#define ARG_COUNT   6

static const char *NearbyTemplate = "TYPE,ALL/S,DOMAIN/K,TIME/N/K,RAW/S,PORTS/S";
static const char version_tag[] __attribute__((used)) =
    "\0$VER: Nearby 42.0 (13.11.2026)";

static void
NearbyPrintKind(const struct SDService *sv, BOOL gone, BOOL raw)
{
    if (gone) {
        SDPrintf("  gone: %s\n", SDTypeFriendly((CONST_STRPTR)sv->sv_Type));
    } else if (raw) {
        SDPrintf("  %-26s  %s\n", SDTypeFriendly((CONST_STRPTR)sv->sv_Type),
            sv->sv_Type);
    } else {
        SDPrintf("  %s\n", SDTypeFriendly((CONST_STRPTR)sv->sv_Type));
    }
}

static void
NearbyPrint(const struct SDService *sv, BOOL gone, BOOL raw, BOOL ports)
{
    if (gone) {
        SDPrintf("  gone: %s\n", sv->sv_Name);
        return;
    }
    if (!ports) {
        if (raw) {
            SDPrintf("  %-28s  (%s)\n", sv->sv_Name, sv->sv_Type);
        } else {
            SDPrintf("  %s\n", sv->sv_Name);
        }
        return;
    }
    SDPrintf("  %-26s port %ld", sv->sv_Name, (LONG)sv->sv_Port);
    if (sv->sv_AddressCount != 0) {
        UWORD i;

        /* every address it has, IPv4 and IPv6 */
        SDPrintf("  (");
        for (i = 0; i < sv->sv_AddressCount; i++) {
            char text[64];

            SDAddressText(&sv->sv_Addresses[i], text, (LONG)sizeof(text));
            SDPrintf("%s%s", (CONST_STRPTR)(i != 0 ? ", " : ""), text);
        }
        SDPrintf(")");
    }
    SDPrintf("\n");
    if (sv->sv_TXTLength > 1) {
        SDPrintf("      info: ");
        SDPrintTXT(sv->sv_TXT, sv->sv_TXTLength);
        SDPrintf("\n");
    }
}

static int
NearbyRun(CONST_STRPTR word, BOOL all, LONG timeSecs, BOOL raw, BOOL ports)
{
    BOOL kinds = all || word == NULL;
    CONST_STRPTR type = kinds ? (CONST_STRPTR)SD_TYPE_SERVICES :
        SDTypeFromAlias(word);
    struct SDSession session;
    struct SDFinder finder;
    struct SDService *sv;
    LONG rc;
    int result = -1;

    sv = AllocVec(sizeof(*sv), MEMF_ANY | MEMF_CLEAR);
    if (sv == NULL) {
        SDAppPrintError("browse", DNSSD_ERR_NOMEM);
        return 20;
    }
    if ((rc = SDSessionOpen(&session, timeSecs)) != DNSSD_OK) {
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
    if (kinds) {
        SDPrintf("Kinds of service on your local network");
    } else {
        SDPrintf("Looking for %s", SDTypeFriendly(type));
    }
    if (timeSecs > 0) {
        SDPrintf(" (%ld seconds)...\n\n", timeSecs);
    } else {
        SDPrintf(" (Ctrl-C to stop)...\n\n");
    }
    while (result < 0 && SDSessionWait(&session) == SD_WAIT_EVENTS) {
        const struct DNSSDEvent *e;

        while (result < 0 && (e = SDSessionRead(&session, &rc)) != NULL) {
            switch (SDFinderEvent(&finder, e, sv, &rc)) {
            case SD_FOUND_TYPE:
                NearbyPrintKind(sv, FALSE, raw);
                break;
            case SD_FOUND_NOTYPE:
                NearbyPrintKind(sv, TRUE, raw);
                break;
            case SD_FOUND_ADD:
            case SD_FOUND_UPDATE:           /* shown again as it is now */
                NearbyPrint(sv, FALSE, raw, ports);
                break;
            case SD_FOUND_REMOVE:
                NearbyPrint(sv, TRUE, raw, ports);
                break;
            case SD_FOUND_END:
                SDAppPrintError("browse", rc);
                result = 20;
                break;
            }
        }
        if (result < 0 && rc != DNSSD_ERR_EMPTY) {
            SDAppPrintError("browse", rc);
            result = 20;
        }
    }
    if (result < 0) {
        SDPrintf("\nDone.\n");
        result = 0;
    }
    SDFinderStop(&finder);
    SDSessionClose(&session);
    FreeVec(sv);
    return result;
}

int
main(void)
{
    struct RDArgs *rda;
    IPTR args[ARG_COUNT];
    LONG timeSecs = 0;
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
    if (args[ARG_TIME] != 0) {
        timeSecs = *((LONG *)args[ARG_TIME]);
    }
    if (!SDAppOpen()) {
        SDPrintf("Cannot open %s version %ld.\n", DNSSDNAME, (LONG)DNSSD_VERSION);
        FreeArgs(rda);
        return 20;
    }
    result = NearbyRun((CONST_STRPTR)args[ARG_TYPE], args[ARG_ALL] != 0,
        timeSecs, args[ARG_RAW] != 0, args[ARG_PORTS] != 0);
    SDAppClose();
    FreeArgs(rda);
    return result;
}
