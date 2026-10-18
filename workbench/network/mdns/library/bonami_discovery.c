/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * Explicitly polled discovery. No hook or client event pointer reaches mdnsd.
 * Local references outlive daemon restarts, but cannot address a new daemon's
 * objects: every read/stop checks the daemon's stable Exec task ID.
 */
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <aros/libcall.h>
#include <string.h>
#include "bonami_intern.h"

#define BONAMI_DISCOVERY_MAX 32

struct BonamiDiscovery
{
    struct Node node;
    APTR ref;
    APTR remote;
    ULONG owner;
    ULONG daemon;
    BOOL resolve;
};

static struct BonamiDiscovery *find_ref(struct BonamiIntern *base, APTR ref)
{
    struct BonamiDiscovery *op;
    ULONG owner = GetETaskID(FindTask(NULL));

    for (op = (struct BonamiDiscovery *)base->bi_Discovery.lh_Head;
         op->node.ln_Succ != NULL; op = (struct BonamiDiscovery *)op->node.ln_Succ) {
        if (op->ref == ref && op->owner == owner) {
            return op;
        }
    }
    return NULL;
}

/* Caller holds bi_Lock. Neither IPC waits nor hooks happen under it. */
static LONG prepare(struct BonamiIntern *base, APTR ref,
    struct MDNSIPCMsg *msg, BOOL stop, BOOL resolve)
{
    struct BonamiDiscovery *op = find_ref(base, ref);

    if (op == NULL || (stop && op->resolve != resolve)) {
        return MDNSERR_BADPARAM;
    }
    msg->mi_Handle = op->remote;
    msg->mi_DaemonID = op->daemon;
    if (stop) {
        Remove(&op->node);
        FreeMem(op, sizeof(*op));
    }
    return MDNS_OK;
}

static LONG copy_input(char *to, LONG size, const char *from)
{
    LONG i = 0;

    if (from == NULL) {
        return MDNSERR_BADPARAM;
    }
    while (i < size && from[i] != '\0') {
        to[i] = from[i];
        i++;
    }
    if (i == 0 || i == size) {
        return MDNSERR_BADPARAM;
    }
    to[i] = '\0';
    return MDNS_OK;
}

static LONG start(struct BonamiIntern *base, STRPTR name, STRPTR type,
    struct Hook *hook, struct TagItem *tags, BOOL resolve)
{
    struct MDNSIPCMsg msg;
    struct BonamiDiscovery *op, *next;
    struct TagItem *output = FindTagItem(resolve ? MDNS_ResolveRef : MDNS_BrowseRef, tags);
    ULONG count = 0;
    LONG rc;

    if (hook != NULL || GetTagData(MDNS_EventMode, 0, tags) != MDNS_EVENT_POLL) {
        return MDNSERR_NOTREADY;   /* callback delivery is not implemented */
    }
    if (output == NULL || output->ti_Data == 0) {
        return MDNSERR_BADPARAM;
    }
    memset(&msg, 0, sizeof(msg));
    rc = copy_input(msg.mi_ServiceType, sizeof(msg.mi_ServiceType), type);
    if (rc == MDNS_OK && resolve) {
        rc = copy_input(msg.mi_Name, 64, name);
    }
    if (rc != MDNS_OK) {
        return rc;
    }
    op = AllocMem(sizeof(*op), MEMF_CLEAR);
    if (op == NULL) {
        return MDNSERR_NOMEM;
    }
    op->owner = GetETaskID(FindTask(NULL));
    op->resolve = resolve;
    ObtainSemaphore(&base->bi_Lock);
    /* Reclaim records left by vanished tasks without reading their memory. */
    for (next = (struct BonamiDiscovery *)base->bi_Discovery.lh_Head;
         next->node.ln_Succ != NULL;) {
        struct BonamiDiscovery *current = next;

        next = (struct BonamiDiscovery *)next->node.ln_Succ;
        if (FindTaskByPID(current->owner) == NULL) {
            Remove(&current->node);
            FreeMem(current, sizeof(*current));
        } else {
            count++;
        }
    }
    if (count == BONAMI_DISCOVERY_MAX || base->bi_NextDiscovery == 0) {
        ReleaseSemaphore(&base->bi_Lock);
        FreeMem(op, sizeof(*op));
        return MDNSERR_BUSY;
    }
    op->ref = (APTR)base->bi_NextDiscovery++;
    AddTail(&base->bi_Discovery, &op->node);
    ReleaseSemaphore(&base->bi_Lock);
    msg.mi_Command = resolve ? MDNS_IPC_RESOLVE : MDNS_IPC_BROWSE;
    rc = bonamiSend(&msg);
    ObtainSemaphore(&base->bi_Lock);
    if (rc == MDNS_OK) {
        op->remote = msg.mi_Handle;
        op->daemon = msg.mi_DaemonID;
        *((APTR *)output->ti_Data) = op->ref;
    } else {
        Remove(&op->node);
        FreeMem(op, sizeof(*op));
    }
    ReleaseSemaphore(&base->bi_Lock);
    return rc;
}

static void stop(struct BonamiIntern *base, APTR ref, BOOL resolve)
{
    struct MDNSIPCMsg msg;
    LONG rc;

    memset(&msg, 0, sizeof(msg));
    msg.mi_Command = resolve ? MDNS_IPC_STOPRESOLVE : MDNS_IPC_STOPBROWSE;
    ObtainSemaphore(&base->bi_Lock);
    rc = prepare(base, ref, &msg, TRUE, resolve);
    ReleaseSemaphore(&base->bi_Lock);
    if (rc == MDNS_OK) {
        bonamiSend(&msg);
    }
}

void bonamiDiscoveryRelease(struct BonamiIntern *base)
{
    struct BonamiDiscovery *op, *next;
    ULONG owner = GetETaskID(FindTask(NULL));

    ObtainSemaphore(&base->bi_Lock);
    for (op = (struct BonamiDiscovery *)base->bi_Discovery.lh_Head;
         op->node.ln_Succ != NULL; op = next) {
        next = (struct BonamiDiscovery *)op->node.ln_Succ;
        if (op->owner == owner) {
            Remove(&op->node);
            FreeMem(op, sizeof(*op));
        }
    }
    ReleaseSemaphore(&base->bi_Lock);
}

AROS_LH3(LONG, BrowseServiceTypeA,
    AROS_LHA(STRPTR, type, A0), AROS_LHA(struct Hook *, hook, A1),
    AROS_LHA(struct TagItem *, tags, A2),
    struct BonamiIntern *, BonamiBase, 10, Bonami)
{
    AROS_LIBFUNC_INIT
    return start(BonamiBase, NULL, type, hook, tags, FALSE);
    AROS_LIBFUNC_EXIT
}
AROS_LH1(void, StopBrowse, AROS_LHA(MDNSBrowseRef, ref, A0),
    struct BonamiIntern *, BonamiBase, 11, Bonami)
{
    AROS_LIBFUNC_INIT
    stop(BonamiBase, ref, FALSE);
    AROS_LIBFUNC_EXIT
}
AROS_LH4(LONG, ResolveServiceA,
    AROS_LHA(STRPTR, name, A0), AROS_LHA(STRPTR, type, A1),
    AROS_LHA(struct Hook *, hook, A2), AROS_LHA(struct TagItem *, tags, A3),
    struct BonamiIntern *, BonamiBase, 12, Bonami)
{
    AROS_LIBFUNC_INIT
    return start(BonamiBase, name, type, hook, tags, TRUE);
    AROS_LIBFUNC_EXIT
}
AROS_LH1(void, StopResolve, AROS_LHA(MDNSResolveRef, ref, A0),
    struct BonamiIntern *, BonamiBase, 13, Bonami)
{
    AROS_LIBFUNC_INIT
    stop(BonamiBase, ref, TRUE);
    AROS_LIBFUNC_EXIT
}
AROS_LH2(LONG, ReadMDNSEvent,
    AROS_LHA(APTR, ref, A0), AROS_LHA(struct MDNSDiscoveryEvent *, event, A1),
    struct BonamiIntern *, BonamiBase, 20, Bonami)
{
    AROS_LIBFUNC_INIT
    struct MDNSIPCMsg msg;
    LONG rc;

    if (event == NULL) {
        return MDNSERR_BADPARAM;
    }
    memset(&msg, 0, sizeof(msg));
    msg.mi_Command = MDNS_IPC_READ;
    ObtainSemaphore(&BonamiBase->bi_Lock);
    rc = prepare(BonamiBase, ref, &msg, FALSE, FALSE);
    ReleaseSemaphore(&BonamiBase->bi_Lock);
    if (rc == MDNS_OK) {
        rc = bonamiSend(&msg);
        if (rc == MDNS_OK) {
            *event = msg.mi_Event;
        }
    }
    return rc;
    AROS_LIBFUNC_EXIT
}
