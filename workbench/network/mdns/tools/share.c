/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * Share - advertise a service running on this Amiga to the local network,
 *         on dnssd.library.
 */

/******************************************************************************

    NAME

        Share NAME/A TYPE/A PORT/N/A [DOMAIN <d>] [TEXT <k=v> ...] [QUIET]

    SYNOPSIS

        NAME/A,TYPE/A,PORT/N/A,DOMAIN/K,TEXT/M,QUIET/S

    LOCATION

        C:

    FUNCTION

        Advertises a service of this computer: other machines running
        Bonjour, Avahi or mDNS (Macs, PCs, phones, other Amigas) see it
        appear. Share keeps advertising until Ctrl-C, then withdraws it
        cleanly. If another machine has the name already, the service is
        advertised as "Name (2)", and Share says so.

    INPUTS

        NAME    the name others will see, e.g. "AmigaShare" (quote it if it
                has spaces)
        TYPE    a plain word ("web", "files", "ftp", ...) or a real DNS-SD
                type ("_http._tcp")
        PORT    the TCP or UDP port the service listens on, 1-65535
        DOMAIN  network domain (default: your local network)
        TEXT    key=value items describing the service, each in quotes:
                ReadArgs() would take the '=' of a bare one as its own. A
                value may be binary: "\xNN" is the byte NN, "\\" a
                backslash
        QUIET   no messages while it runs

    RESULT

        0, or 20 when the service could not be advertised.

    NOTES

        A server must actually listen on that port for the advert to be of
        use.

    EXAMPLE

        Share AmigaShare web 80 "path=/" "version=1"
        Share "Tim's FTP" ftp 21

    SEE ALSO

        Nearby, Handshake, dnssd

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

#define ARG_NAME    0
#define ARG_TYPE    1
#define ARG_PORT    2
#define ARG_DOMAIN  3
#define ARG_TEXT    4
#define ARG_QUIET   5
#define ARG_COUNT   6

static const char *ShareTemplate =
    "NAME/A,TYPE/A,PORT/N/A,DOMAIN/K,TEXT/M,QUIET/S";
static const char version_tag[] __attribute__((used)) =
    "\0$VER: Share 42.0 (13.11.2026)";

static int
ShareRun(CONST_STRPTR name, CONST_STRPTR word, UWORD port,
    STRPTR *textv, BOOL quiet)
{
    CONST_STRPTR type = SDTypeFromAlias(word);
    struct SDSession session;
    DNSSDOperationRef op;
    UBYTE txt[1300];
    char items[400];
    LONG txtLength, rc, why;
    int result = -1;

    txtLength = SDTXTFromItems(txt, (LONG)sizeof(txt), (char **)textv);
    if (txtLength < 0) {
        SDPrintf("Share: TEXT is too long, or an escape is not \\xNN.\n");
        return 20;
    }
    SDItemsText(items, (LONG)sizeof(items), (char **)textv);
    if ((rc = SDSessionOpen(&session, 0)) != DNSSD_OK) {
        SDAppPrintError("advertise", rc);
        return 20;
    }
    if (txtLength != 0) {
        rc = DNSSD_Register(session.bs_Session, name, type, port, &op,
            DNSSD_TXT, (IPTR)txt, DNSSD_TXTLength, txtLength, TAG_DONE);
    } else {
        rc = DNSSD_Register(session.bs_Session, name, type, port, &op, TAG_DONE);
    }
    if (rc != DNSSD_OK) {
        SDSessionClose(&session);
        SDAppPrintError("advertise", rc);
        return 20;
    }
    while (result < 0 && (why = SDSessionWait(&session)) == SD_WAIT_EVENTS) {
        const struct DNSSDEvent *e;

        while (result < 0 && (e = SDSessionRead(&session, &rc)) != NULL) {
            if (e->de_Kind == DNSSD_EVENT_REGISTERED && !quiet) {
                const UBYTE *label = SDEventPart(e, &e->de_Instance);
                char as[64];
                ULONG i;

                for (i = 0; label != NULL && i < e->de_Instance.dr_Size && i < 63; i++) {
                    as[i] = (char)label[i];
                }
                as[i] = '\0';
                SDPrintf("Now sharing \"%s\" (%s) on port %ld.\n",
                    as, SDTypeFriendly(type), (LONG)port);
                SDPrintf("Others see it as: %s.%s.local\n", as, type);
                if (items[0] != '\0') {
                    SDPrintf("Info advertised:  %s\n", items);
                }
                SDPrintf("\nSharing... press Ctrl-C to stop.\n");
            } else if (e->de_Kind == DNSSD_EVENT_END) {
                SDAppPrintError("advertise", e->de_Result);
                result = 20;
            }
        }
        if (result < 0 && rc != DNSSD_ERR_EMPTY) {
            SDAppPrintError("advertise", rc);
            result = 20;
        }
    }
    DNSSD_Cancel(op);
    SDSessionClose(&session);
    if (result < 0) {
        if (!quiet) {
            SDPrintf("Stopped sharing \"%s\".\n", name);
        }
        result = 0;
    }
    return result;
}

int
main(void)
{
    struct RDArgs *rda;
    IPTR args[ARG_COUNT];
    LONG portNum;
    int result;
    int i;

    for (i = 0; i < ARG_COUNT; i++) {
        args[i] = 0;
    }
    rda = ReadArgs((STRPTR)ShareTemplate, args, NULL);
    if (rda == NULL) {
        PrintFault(IoErr(), (CONST_STRPTR)"Share");
        return 20;
    }
    portNum = (args[ARG_PORT] != 0) ? *((LONG *)args[ARG_PORT]) : 0;
    if (portNum < 1 || portNum > 65535) {
        SDPrintf("Share: PORT must be 1-65535.\n");
        FreeArgs(rda);
        return 20;
    }
    if (!SDAppOpen()) {
        SDPrintf("Cannot open %s version %ld.\n", DNSSDNAME, (LONG)DNSSD_VERSION);
        FreeArgs(rda);
        return 20;
    }
    result = ShareRun((CONST_STRPTR)args[ARG_NAME], (CONST_STRPTR)args[ARG_TYPE],
        (UWORD)portNum, (STRPTR *)args[ARG_TEXT], args[ARG_QUIET] != 0);
    SDAppClose();
    FreeArgs(rda);
    return result;
}
