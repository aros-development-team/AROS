/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * Handshake - find out how to reach one named service on the local network,
 *             on dnssd.library.
 */

/******************************************************************************

    NAME

        Handshake NAME/A TYPE/A [DOMAIN <d>] [TIME <secs>]

    SYNOPSIS

        NAME/A,TYPE/A,DOMAIN/K,TIME/N/K

    LOCATION

        C:

    FUNCTION

        You already know a service's name (from Nearby, say): Handshake
        resolves it to the host, its IP addresses (IPv4 and IPv6) and the
        port you need to connect, and any extra information the service
        advertises.

    INPUTS

        NAME    the service's name, e.g. "Tim's Mac" (quote names with
                spaces)
        TYPE    a plain word ("web", "files", "ssh", ...) or a real DNS-SD
                type ("_http._tcp")
        DOMAIN  network domain (default: your local network)
        TIME    how many seconds to keep trying, 1 to 60 (default 30)

    RESULT

        0 when it was reached, 5 when not, 20 for an error.

    EXAMPLE

        Handshake "Tim's Mac" web
        Handshake AmigaShare files

    SEE ALSO

        Nearby, Share, dnssd

******************************************************************************/

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dnssd.h>
#include <utility/tagitem.h>

#include "sd_common.h"
#include "sd_types.h"

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#include "../strings.h"

#define ARG_NAME    0
#define ARG_TYPE    1
#define ARG_DOMAIN  2
#define ARG_TIME    3
#define ARG_COUNT   4

static const char *HandshakeTemplate = "NAME/A,TYPE/A,DOMAIN/K,TIME/N/K";
static const char version_tag[] __attribute__((used)) =
    "\0$VER: Handshake 42.2 (21.11.2026)";

static void
HandshakeReport(CONST_STRPTR name, const struct DNSSDEvent *e)
{
    struct DNSSDAddress addresses[SD_ADDRESSES];
    ULONG count = SDEventAddresses(e, addresses, SD_ADDRESSES), i;
    char target[256];

    SDEventName(e, &e->de_Target, target, (LONG)sizeof(target));
    SDPrintf(SDMsg(MSG_HANDSHAKE_REACHED), name);
    SDPrintf(SDMsg(MSG_HANDSHAKE_HOST), target);
    for (i = 0; i < count; i++) {
        char text[64];

        SDAddressText(&addresses[i], text, (LONG)sizeof(text));
        SDPrintf(SDMsg(MSG_HANDSHAKE_IP), text);
    }
    SDPrintf(SDMsg(MSG_HANDSHAKE_PORT), (LONG)e->de_Port);
    if (e->de_TXT.dr_Size > 1) {
        SDPrintf(SDMsg(MSG_HANDSHAKE_INFO));
        SDPrintTXT(SDEventPart(e, &e->de_TXT), (LONG)e->de_TXT.dr_Size);
        SDPrintf("\n");
    }
}

/* 0 reached, 5 not, 20 an error. */
static int
HandshakeRun(CONST_STRPTR name, CONST_STRPTR word, LONG timeSecs)
{
    CONST_STRPTR type = SDTypeFromAlias(word);
    struct SDSession session;
    DNSSDOperationRef op;
    LONG rc, why;
    int result = -1;

    if (timeSecs <= 0) {
        timeSecs = 30;
    } else if (timeSecs > 60) {
        timeSecs = 60;
    }
    if ((rc = SDSessionOpen(&session, 0)) != DNSSD_OK) {
        SDAppPrintError(SDMsg(MSG_OP_CONTACT), rc);
        return 20;
    }
    rc = DNSSD_Resolve(session.bs_Session, name, type, &op,
        DNSSD_Timeout, timeSecs, TAG_DONE);
    if (rc != DNSSD_OK) {
        SDSessionClose(&session);
        SDAppPrintError(SDMsg(MSG_OP_CONTACT), rc);
        return 20;
    }
    SDPrintf(SDMsg(MSG_HANDSHAKE_CONTACTING), name, SDTypeFriendly(type));
    while (result < 0 && (why = SDSessionWait(&session)) == SD_WAIT_EVENTS) {
        const struct DNSSDEvent *e;

        while (result < 0 && (e = SDSessionRead(&session, &rc)) != NULL) {
            if (e->de_Kind == DNSSD_EVENT_RESOLVED) {
                HandshakeReport(name, e);
                result = 0;
            } else if (e->de_Kind == DNSSD_EVENT_END) {
                result = 5;
            }
        }
        if (result < 0 && rc != DNSSD_ERR_EMPTY) {
            SDAppPrintError(SDMsg(MSG_OP_CONTACT), rc);
            result = 20;
        }
    }
    if (result == 5) {
        SDPrintf(SDMsg(MSG_HANDSHAKE_NOT_REACHED), name);
    }
    DNSSD_Cancel(op);
    SDSessionClose(&session);
    return result < 0 ? 5 : result;
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
    /* the catalog first: what is said about the arguments is translated */
    SDLocaleOpen();
    rda = ReadArgs((STRPTR)HandshakeTemplate, args, NULL);
    if (rda == NULL) {
        SDLocaleClose();
        PrintFault(IoErr(), (CONST_STRPTR)"Handshake");
        return 20;
    }
    if (args[ARG_TIME] != 0) {
        timeSecs = *((LONG *)args[ARG_TIME]);
    }
    if (!SDAppOpen()) {
        SDAppPrintNoLibrary();
        FreeArgs(rda);
        SDLocaleClose();
        return 20;
    }
    result = HandshakeRun((CONST_STRPTR)args[ARG_NAME],
        (CONST_STRPTR)args[ARG_TYPE], timeSecs);
    SDAppClose();
    FreeArgs(rda);
    SDLocaleClose();
    return result;
}
