/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A printer that has moved: DHCP gave it another address since the unit was
    set up. IPP Everywhere printers keep their UUID, so the unit's UUID= finds
    them again on the link (dnssd.c), and the unit follows them there.
*/

#include <aros/debug.h>

#include <exec/memory.h>
#include <devices/timer.h>
#include <proto/exec.h>

#include <string.h>

#include "ipp_intern.h"
#include "dnssd.h"

#define RESOLVE_MS      3000
#define RESOLVE_MAX     16

BOOL IPP_Resolve(struct IPPBase *IPPBase, struct IPPUnit *iu)
{
    static struct DNSSDPrinter found[RESOLVE_MAX];
    struct IPPConfig *ic = &iu->iu_Config;
    struct Library *socketbase;
    struct MsgPort *port;
    struct timerequest *tr = NULL;
    BOOL ipp = strncmp(ic->ic_URI, "ipp://", 6) == 0, moved = FALSE;
    LONG n, i, error, pick = -1;

    if (ic->ic_UUID[0] == 0)
        return FALSE;
    /* Sockets and the timer request belong to the opener's task, as in ipp_net.c. */
    if (!(socketbase = OpenLibrary("bsdsocket.library", 4)))
        return FALSE;
    if (!(port = CreateMsgPort())) {
        CloseLibrary(socketbase);
        return FALSE;
    }
    if (!(tr = (struct timerequest *)CreateIORequest(port, sizeof(*tr)))
        || OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0) != 0) {
        if (tr)
            DeleteIORequest((struct IORequest *)tr);
        DeleteMsgPort(port);
        CloseLibrary(socketbase);
        return FALSE;
    }

    ObtainSemaphore(&IPPBase->ib_ResolveLock);
    n = DNSSD_FindPrinters(socketbase, tr->tr_node.io_Device, RESOLVE_MS, found, RESOLVE_MAX, &error);
    for (i = 0; i < n; i++)
        if (strcasecmp(found[i].dp_UUID, ic->ic_UUID) == 0 && (pick < 0 || found[i].dp_IPP == ipp))
            pick = i;
    if (pick >= 0 && strcmp(found[pick].dp_URI, ic->ic_URI) != 0
        && strlen(found[pick].dp_URI) < sizeof(ic->ic_URI)) {
        D(bug("[ipp] unit %lu: %s moved to %s\n", (unsigned long)iu->iu_Number, ic->ic_URI, found[pick].dp_URI));
        strcpy(ic->ic_URI, found[pick].dp_URI);
        moved = TRUE;
    }
    ReleaseSemaphore(&IPPBase->ib_ResolveLock);

    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(port);
    CloseLibrary(socketbase);

    if (moved) {
        struct IPPKey key = { "URI=", ic->ic_URI };
        IPP_UpdateConfig(IPPBase, iu->iu_Number, &key, 1, IPP_SAVE_IF_THERE);
    }
    return moved;
}
