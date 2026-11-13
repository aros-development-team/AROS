/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * Share - advertise a service running on this Amiga to the local network.
 *
 * Other machines running Bonjour/Avahi/mDNS (Macs, PCs, phones, other Amigas)
 * will see the service appear.  Share keeps advertising until you press Ctrl-C,
 * then withdraws it cleanly.
 *
 *   Share NAME/A TYPE/A PORT/N/A [DOMAIN <d>] [TEXT <k=v> ...] [QUIET]
 *
 *   NAME    the name others will see, e.g. "AmigaShare" (quote if it has
 *           spaces).
 *   TYPE    a plain word ("web", "files", "ftp", ...) or a real DNS-SD type.
 *   PORT    the TCP/UDP port your service listens on (1-65535).
 *   DOMAIN  network domain (default: your local network).
 *   TEXT    optional key=value pairs describing the service, each in
 *           quotes: ReadArgs() would take the '=' of a bare one as its own.
 *   QUIET   suppress the running banner.
 *
 * Examples:
 *   Share AmigaShare web 80 "path=/" "version=1"
 *   Share "Tim's FTP" ftp 21
 *
 * Native AmigaOS only.  You must actually be running a server on that port for
 * the advert to be useful.
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
#define ARG_PORT    2
#define ARG_DOMAIN  3
#define ARG_TEXT    4
#define ARG_QUIET   5
#define ARG_COUNT   6

static const char *ShareTemplate =
    "NAME/A,TYPE/A,PORT/N/A,DOMAIN/K,TEXT/M,QUIET/S";

static int
ShareRun(CONST_STRPTR name, CONST_STRPTR word, UWORD port,
    STRPTR *textv, BOOL quiet)
{
    struct TagItem tags[8];
    char txt[MDNS_MAX_TXT + 1];
    CONST_STRPTR type;
    MDNSRegisterRef ref;
    LONG rc;
    LONG nt;

    type = BATypeFromAlias(word);

    if (!BAEncodeTXT(txt, (LONG)sizeof(txt), (char **)textv)) {
        BAPrintf("Share: TEXT is too long.\n");
        return 20;
    }

    ref = NULL;
    nt = 0;
    tags[nt].ti_Tag = MDNS_ServiceName;
    tags[nt].ti_Data = (IPTR)name;
    nt++;
    tags[nt].ti_Tag = MDNS_ServiceType;
    tags[nt].ti_Data = (IPTR)type;
    nt++;
    tags[nt].ti_Tag = MDNS_Port;
    tags[nt].ti_Data = (IPTR)port;
    nt++;
    if (txt[0] != '\0') {
        tags[nt].ti_Tag = MDNS_TXTRecord;
        tags[nt].ti_Data = (IPTR)txt;
        nt++;
    }
    tags[nt].ti_Tag = MDNS_RegisterRef;
    tags[nt].ti_Data = (IPTR)&ref;
    nt++;
    tags[nt].ti_Tag = TAG_END;
    tags[nt].ti_Data = 0;

    rc = RegisterServiceA(tags);
    if (rc != MDNS_OK) {
        BAAppPrintMDNSError("advertise", rc);
        return 20;
    }

    if (!quiet) {
        BAPrintf("Now sharing \"%s\" (%s) on port %ld.\n",
            name, BATypeFriendly(type), (LONG)port);
        BAPrintf("Others see it as: %s.%s.local\n", name, type);
        if (txt[0] != '\0') {
            BAPrintf("Info advertised:  %s\n", txt);
        }
        BAPrintf("\nSharing... press Ctrl-C to stop.\n");
    }

    Wait(SIGBREAKF_CTRL_C);

    /* RegisterServiceA() wrote the handle; DeregisterService(NULL) would
       do nothing */
    DeregisterService(ref);
    if (!quiet) {
        BAPrintf("Stopped sharing \"%s\".\n", name);
    }
    return 0;
}

int
main(void)
{
    struct RDArgs *rda;
    IPTR args[ARG_COUNT];
    CONST_STRPTR name;
    CONST_STRPTR word;
    STRPTR *textv;
    LONG portNum;
    BOOL quiet;
    int result;
    int i;

    for (i = 0; i < ARG_COUNT; i++) {
        args[i] = 0;
    }

    rda = ReadArgs((STRPTR)ShareTemplate, args, NULL);
    if (rda == NULL) {
        PrintFault(IoErr(), "Share");
        return 20;
    }

    name = (CONST_STRPTR)args[ARG_NAME];
    word = (CONST_STRPTR)args[ARG_TYPE];
    portNum = (args[ARG_PORT] != 0) ? *((LONG *)args[ARG_PORT]) : 0;
    textv = (STRPTR *)args[ARG_TEXT];
    quiet = (args[ARG_QUIET] != 0) ? TRUE : FALSE;

    if (portNum < 1 || portNum > 65535) {
        BAPrintf("Share: PORT must be 1-65535.\n");
        FreeArgs(rda);
        return 20;
    }

    if (!BAAppOpen()) {
        BAPrintf("Cannot open %s (v%ld+).\n", BONAMINAME,
            (LONG)BONAMI_LIB_VERSION);
        FreeArgs(rda);
        return 20;
    }

    if (BAAppOpenEngine() != MDNS_OK) {
        BAPrintf("Cannot start the mDNS engine.\n");
        BAAppClose();
        FreeArgs(rda);
        return 20;
    }

    result = ShareRun(name, word, (UWORD)portNum, textv, quiet);

    BAAppClose();
    FreeArgs(rda);
    return result;
}
