/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * mdnsd_service.c - registers mdnsd with netservices.library.
 *
 * The stack's service manager starts mdnsd from db/services.d/mdns. Here
 * mdnsd asks to be signalled when the stack stops, and around a
 * reconfiguration, when it has to let go of bsdsocket.library and take it
 * up again.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/netservice.h>
#include <proto/exec.h>
#include <proto/netservice.h>

#include "mdnsd.h"

struct Library *NetServicesBase;

static APTR handle;
static BYTE sigStop = -1, sigBegin = -1, sigEnd = -1;

BOOL
mdnsdServiceOpen(struct MDNSDSignals *sig)
{
    struct TagItem tags[6];

    sig->ms_Stop = sig->ms_Begin = sig->ms_End = 0;

    NetServicesBase = OpenLibrary(NETSERVICESNAME, 0);
    if (NetServicesBase == NULL) {
        return FALSE;
    }
    sigStop = AllocSignal(-1);
    sigBegin = AllocSignal(-1);
    sigEnd = AllocSignal(-1);

    tags[0].ti_Tag = NETSERVICE_Name;
    tags[0].ti_Data = (IPTR)"mdns";
    tags[1].ti_Tag = NETSERVICE_Order;
    tags[1].ti_Data = (IPTR)NSPRI_RCDAEMONS;
    tags[2].ti_Tag = NETSERVICE_ReconfigBeginSig;
    tags[2].ti_Data = (IPTR)(LONG)sigBegin;
    tags[3].ti_Tag = NETSERVICE_ReconfigEndSig;
    tags[3].ti_Data = (IPTR)(LONG)sigEnd;
    tags[4].ti_Tag = NETSERVICE_StopSig;
    tags[4].ti_Data = (IPTR)(LONG)sigStop;
    tags[5].ti_Tag = TAG_DONE;
    tags[5].ti_Data = 0;
    handle = RegisterNetService(tags);
    if (handle == NULL) {
        mdnsdServiceClose();
        return FALSE;
    }

    sig->ms_Stop = sigStop >= 0 ? 1UL << sigStop : 0;
    sig->ms_Begin = sigBegin >= 0 ? 1UL << sigBegin : 0;
    sig->ms_End = sigEnd >= 0 ? 1UL << sigEnd : 0;
    return TRUE;
}

void
mdnsdServiceClose(void)
{
    if (handle != NULL) {
        UnregisterNetService(handle);
        handle = NULL;
    }
    if (sigStop >= 0) {
        FreeSignal(sigStop);
    }
    if (sigBegin >= 0) {
        FreeSignal(sigBegin);
    }
    if (sigEnd >= 0) {
        FreeSignal(sigEnd);
    }
    sigStop = sigBegin = sigEnd = -1;
    if (NetServicesBase != NULL) {
        CloseLibrary(NetServicesBase);
        NetServicesBase = NULL;
    }
}
