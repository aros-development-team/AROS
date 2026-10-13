/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * netservices.library - the AROSTCP managed-service registry.
 *
 * This owns the registry (the service list, its lock and the start/stop/signal
 * logic) that used to live in bsdsocket (kern/amiga_services.c).  It holds no
 * SocketBase and does not depend on bsdsocket.  The stack drives its own
 * internal, op-driven services through the plain C entry points (netservice_*);
 * external daemons enlist through the LVO calls (RegisterNetService, ...).
 *
 * Conventional single shared base, served by the stack like miami.library: the
 * library Open() just shares the one base the stack created - there is no
 * per-opener MakeLibrary().
 */

#include <conf.h>

#include <aros/libcall.h>
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <utility/tagitem.h>
#include <proto/exec.h>

#include <libraries/netservice.h>
#include "netservice_api.h"

/*
 * The registry.  A plain Exec list sorted (by Enqueue) in descending ln_Pri,
 * guarded by a semaphore because external services may (un)register from their
 * own task while the stack orchestrates.  Private to this library.
 */
static struct List            NetServiceList;
static struct SignalSemaphore NetServiceLock;
static BOOL                   NetServicesReady = FALSE;

/* -------------------------------------------------------------------------
 * Registry lifecycle
 * ------------------------------------------------------------------------- */

void netservice_registry_init(void)
{
    D(bug("[netservices] registry_init()\n"));
    NewList(&NetServiceList);
    InitSemaphore(&NetServiceLock);
    NetServicesReady = TRUE;
}

void netservice_registry_deinit(void)
{
    D(bug("[netservices] registry_deinit()\n"));
    NetServicesReady = FALSE;
    /* Internal services own their storage; external nodes are freed on
     * UnregisterNetService.  Nothing to free here. */
}

/* -------------------------------------------------------------------------
 * Low-level (un)register of a caller-owned node (stack-internal services)
 * ------------------------------------------------------------------------- */

void netservice_register(struct NetService *svc)
{
    if(!NetServicesReady || svc == NULL)
        return;

    svc->ns_Node.ln_Type = NT_NETSERVICE;
    svc->ns_State        = NSSTATE_STOPPED;

    D(bug("[netservices] register('%s', pri %ld, %s)\n",
          svc->ns_Node.ln_Name ? svc->ns_Node.ln_Name : (STRPTR)"?",
          (long)svc->ns_Node.ln_Pri,
          (svc->ns_Flags & NSF_EXTERNAL) ? "external" : "internal"));

    ObtainSemaphore(&NetServiceLock);
    Enqueue(&NetServiceList, &svc->ns_Node);
    ReleaseSemaphore(&NetServiceLock);
}

void netservice_unregister(struct NetService *svc)
{
    if(!NetServicesReady || svc == NULL)
        return;

    D(bug("[netservices] unregister('%s')\n",
          svc->ns_Node.ln_Name ? svc->ns_Node.ln_Name : (STRPTR)"?"));

    ObtainSemaphore(&NetServiceLock);
    Remove(&svc->ns_Node);
    ReleaseSemaphore(&NetServiceLock);
}

struct NetService *netservice_find(CONST_STRPTR name)
{
    struct NetService *svc;

    if(!NetServicesReady || name == NULL)
        return NULL;

    ObtainSemaphoreShared(&NetServiceLock);
    svc = (struct NetService *)FindName(&NetServiceList, name);
    ReleaseSemaphore(&NetServiceLock);

    return svc;
}

/* -------------------------------------------------------------------------
 * Lifecycle: start/stop internal (op-driven) services, signal external ones
 * ------------------------------------------------------------------------- */

LONG netservice_start_all(void)
{
    struct Node *node;
    LONG error = 0;

    if(!NetServicesReady)
        return 0;

    D(bug("[netservices] start_all()\n"));

    /* Head->tail = descending ln_Pri = dependency bring-up order. */
    for(node = NetServiceList.lh_Head; node->ln_Succ; node = node->ln_Succ) {
        struct NetService *svc = (struct NetService *)node;

        if(svc->ns_State == NSSTATE_RUNNING || svc->ns_Ops == NULL ||
                svc->ns_Ops->nso_start == NULL)
            continue;

        svc->ns_State = NSSTATE_STARTING;
        error = svc->ns_Ops->nso_start(svc);
        if(error) {
            svc->ns_State = NSSTATE_FAILED;
            D(bug("[netservices] service '%s' start failed (%ld)\n",
                  svc->ns_Node.ln_Name, (long)error));
            break;
        }
        svc->ns_State = NSSTATE_RUNNING;
    }

    return error;
}

void netservice_stop_all(BOOL force)
{
    struct Node *node;

    if(!NetServicesReady)
        return;

    D(bug("[netservices] stop_all(force=%ld)\n", (long)force));

    /* Tail->head = reverse of bring-up = teardown order. */
    for(node = NetServiceList.lh_TailPred; node->ln_Pred; node = node->ln_Pred) {
        struct NetService *svc = (struct NetService *)node;

        if(svc->ns_State == NSSTATE_STOPPED || svc->ns_Ops == NULL ||
                svc->ns_Ops->nso_stop == NULL)
            continue;

        svc->ns_State = NSSTATE_STOPPING;
        svc->ns_Ops->nso_stop(svc, force);
        svc->ns_State = NSSTATE_STOPPED;
    }
}

void netservice_signal_external(ULONG phase)
{
    struct Node *node;

    if(!NetServicesReady)
        return;

    D(bug("[netservices] signal_external(phase=%ld)\n", (long)phase));

    ObtainSemaphoreShared(&NetServiceLock);
    if(phase == NSPHASE_END) {
        /* bring-up order: head->tail */
        for(node = NetServiceList.lh_Head; node->ln_Succ; node = node->ln_Succ) {
            struct NetService *svc = (struct NetService *)node;
            if((svc->ns_Flags & NSF_EXTERNAL) && !(svc->ns_Flags & NSF_IGNORERELOAD)
                    && svc->ns_Task && svc->ns_SigEnd)
                Signal(svc->ns_Task, svc->ns_SigEnd);
        }
    } else {
        /* teardown order: tail->head */
        for(node = NetServiceList.lh_TailPred; node->ln_Pred; node = node->ln_Pred) {
            struct NetService *svc = (struct NetService *)node;
            ULONG sig;
            if(!(svc->ns_Flags & NSF_EXTERNAL) || (svc->ns_Flags & NSF_IGNORERELOAD)
                    || svc->ns_Task == NULL)
                continue;
            sig = (phase == NSPHASE_STOP) ? svc->ns_SigStop : svc->ns_SigBegin;
            if(sig)
                Signal(svc->ns_Task, sig);
        }
    }
    ReleaseSemaphore(&NetServiceLock);
}

/* -------------------------------------------------------------------------
 * Tag helpers (manual walk - no utility.library dependency)
 * ------------------------------------------------------------------------- */

static IPTR ns_gettag(const struct TagItem *tags, Tag want, IPTR def)
{
    const struct TagItem *ti = tags;

    while(ti) {
        switch(ti->ti_Tag) {
        case TAG_DONE:
            return def;
        case TAG_IGNORE:
            break;
        case TAG_MORE:
            ti = (const struct TagItem *)ti->ti_Data;
            continue;
        case TAG_SKIP:
            ti += ti->ti_Data;
            break;
        default:
            if(ti->ti_Tag == want)
                return ti->ti_Data;
            break;
        }
        ti++;
    }
    return def;
}

/* Signal NUMBER (0..31) -> mask; -1/out-of-range -> 0 (no signal). */
static ULONG ns_sigmask(IPTR signum)
{
    return (signum >= 0 && signum < 32) ? (1UL << signum) : 0;
}

/* Local strlen - keeps this library free of a string/utility dependency. */
static ULONG ns_strlen(CONST_STRPTR s)
{
    CONST_STRPTR p = s;
    while(*p)
        p++;
    return (ULONG)(p - s);
}

/* -------------------------------------------------------------------------
 * Library base functions (single shared base, no per-opener MakeLibrary)
 * ------------------------------------------------------------------------- */

AROS_LH1(struct Library *, Open,
         AROS_LHA(ULONG, version, D0),
         struct NetServicesBase *, NetServicesBase, 1, NetServices)
{
    AROS_LIBFUNC_INIT

    D(bug("[netservices] Open() OpenCount %ld\n",
          (long)NetServicesBase->nsb_Lib.lib_OpenCnt));

    NetServicesBase->nsb_Lib.lib_OpenCnt++;
    NetServicesBase->nsb_Lib.lib_Flags &= ~LIBF_DELEXP;
    return (struct Library *)NetServicesBase;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, Close, struct NetServicesBase *, NetServicesBase, 2, NetServices)
{
    AROS_LIBFUNC_INIT

    D(bug("[netservices] Close() OpenCount %ld\n",
          (long)NetServicesBase->nsb_Lib.lib_OpenCnt));

    if(NetServicesBase->nsb_Lib.lib_OpenCnt > 0)
        NetServicesBase->nsb_Lib.lib_OpenCnt--;
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, Expunge, struct NetServicesBase *, NetServicesBase, 3, NetServices)
{
    AROS_LIBFUNC_INIT

    /* Stack-owned base: never expunge on our own.  The stack removes and frees
     * it at shutdown.  Flag the pending delete but keep the base resident. */
    NetServicesBase->nsb_Lib.lib_Flags |= LIBF_DELEXP;
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(APTR, Reserved, struct NetServicesBase *, NetServicesBase, 4, NetServices)
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

/* -------------------------------------------------------------------------
 * Public LVO API (external daemons)
 * ------------------------------------------------------------------------- */

AROS_LH1(APTR, RegisterNetService,
         AROS_LHA(struct TagItem *, tags, A0),
         struct NetServicesBase *, NetServicesBase, 5, NetServices)
{
    AROS_LIBFUNC_INIT

    struct NetService *svc;
    STRPTR name = (STRPTR)ns_gettag(tags, NETSERVICE_Name, (IPTR)NULL);
    IPTR   namelen;

    if(!NetServicesReady || name == NULL)
        return NULL;

    /* Allocate node + a private copy of the name (the caller's string may be
     * transient). */
    for(namelen = 0; name[namelen]; namelen++)
        ;
    svc = AllocMem(sizeof(struct NetService) + namelen + 1,
                   MEMF_PUBLIC | MEMF_CLEAR);
    if(svc == NULL)
        return NULL;

    {
        STRPTR dst = (STRPTR)(svc + 1);
        IPTR i;
        for(i = 0; i <= namelen; i++)
            dst[i] = name[i];
        svc->ns_Node.ln_Name = dst;
    }
    svc->ns_Node.ln_Pri = (BYTE)ns_gettag(tags, NETSERVICE_Order, 0);
    svc->ns_Flags       = NSF_EXTERNAL;
    if(ns_gettag(tags, NETSERVICE_ReloadPolicy, NSRP_SIGNAL) == NSRP_IGNORE)
        svc->ns_Flags |= NSF_IGNORERELOAD;
    svc->ns_Ops   = NULL;
    svc->ns_Task  = (struct Task *)ns_gettag(tags, NETSERVICE_Task,
                                             (IPTR)FindTask(NULL));
    svc->ns_SigBegin = ns_sigmask(ns_gettag(tags, NETSERVICE_ReconfigBeginSig, -1));
    svc->ns_SigEnd   = ns_sigmask(ns_gettag(tags, NETSERVICE_ReconfigEndSig, -1));
    svc->ns_SigStop  = ns_sigmask(ns_gettag(tags, NETSERVICE_StopSig, -1));

    netservice_register(svc);
    return (APTR)svc;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, UnregisterNetService,
         AROS_LHA(APTR, handle, A0),
         struct NetServicesBase *, NetServicesBase, 6, NetServices)
{
    AROS_LIBFUNC_INIT

    struct NetService *svc = (struct NetService *)handle;

    if(svc == NULL)
        return;

    /* Only externally-registered nodes are library-allocated and freeable. */
    if(!(svc->ns_Flags & NSF_EXTERNAL))
        return;

    netservice_unregister(svc);
    FreeMem(svc, sizeof(struct NetService) +
            (svc->ns_Node.ln_Name ? ns_strlen(svc->ns_Node.ln_Name) + 1 : 0));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, QueryNetServices,
         AROS_LHA(struct TagItem *, tags, A0),
         struct NetServicesBase *, NetServicesBase, 7, NetServices)
{
    AROS_LIBFUNC_INIT

    struct Node *node;
    LONG index = (LONG)ns_gettag(tags, NETSERVICE_QueryIndex, -1);
    LONG count = 0;
    STRPTR *pName  = (STRPTR *)ns_gettag(tags, NETSERVICE_QueryName,  (IPTR)NULL);
    ULONG  *pState = (ULONG  *)ns_gettag(tags, NETSERVICE_QueryState, (IPTR)NULL);
    LONG   *pOrder = (LONG   *)ns_gettag(tags, NETSERVICE_QueryOrder, (IPTR)NULL);
    ULONG  *pFlags = (ULONG  *)ns_gettag(tags, NETSERVICE_QueryFlags, (IPTR)NULL);

    if(!NetServicesReady)
        return 0;

    ObtainSemaphoreShared(&NetServiceLock);
    for(node = NetServiceList.lh_Head; node->ln_Succ; node = node->ln_Succ) {
        struct NetService *svc = (struct NetService *)node;
        if(count == index) {
            if(pName)  *pName  = svc->ns_Node.ln_Name;
            if(pState) *pState = svc->ns_State;
            if(pOrder) *pOrder = svc->ns_Node.ln_Pri;
            if(pFlags) *pFlags = svc->ns_Flags;
        }
        count++;
    }
    ReleaseSemaphore(&NetServiceLock);

    return count;

    AROS_LIBFUNC_EXIT
}
