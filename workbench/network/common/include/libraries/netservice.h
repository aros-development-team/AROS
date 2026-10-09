#ifndef LIBRARIES_NETSERVICE_H
#define LIBRARIES_NETSERVICE_H

/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * netservices.library - the AROSTCP managed-service registry.
 *
 * A "network service" is a lifecycle unit the TCP/IP stack brings up, tears
 * down and reloads as one ordered operation: the interface/config loader, the
 * DHCP client, the resolver's dynamic DNS, rc-launched daemons, and EXTERNAL
 * daemons that run in their own process (a DHCP manager, a control/UX daemon,
 * Envoy's NIPC services, ...).  This library OWNS the registry - the service
 * list, its lock and the start/stop/signal logic; it is pure management and
 * holds no SocketBase, so it does not depend on bsdsocket.library.  The stack
 * registers its own (internal, op-driven) services through it and drives the
 * reload through it; external daemons enlist themselves with
 * RegisterNetService() and react, in their own task, to the signals they
 * asked for.  The stack never calls back into a service under its locks.
 *
 * Conventional single-base library (OpenLibrary("netservices.library", 0)
 * shares one base), exposed by the stack like miami.library.
 */

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>

#define NETSERVICESNAME     "netservices.library"
#define NETSERVICES_VERSION   1
#define NETSERVICES_REVISION  0

/*
 * Library base.  Deliberately no SocketBase - this library does not depend on
 * bsdsocket.  The registry itself (the service list and its lock) is private
 * to the library implementation, not exposed in the base.
 */
struct NetServicesBase {
    struct Library          nsb_Lib;
};

/* ln_Type for a service node (kept distinct from the Exec standard types). */
#define NT_NETSERVICE   0x90

/* Bring-up priorities (higher starts earlier / stops later). */
#define NSPRI_RESOLVER      100     /* dynamic DNS; started first, stopped last */
#define NSPRI_INTERFACES     80     /* interface/config loader */
#define NSPRI_AUTOIP         60     /* IPv4LL */
#define NSPRI_DHCP           50     /* DHCPv4/v6 client manager */
#define NSPRI_RCDAEMONS      20     /* rc-launched external daemons */
#define NSPRI_SVCMGR         10     /* external service-daemon launcher: starts last, stops first */

/* ns_State */
#define NSSTATE_STOPPED     0
#define NSSTATE_STARTING    1
#define NSSTATE_RUNNING     2
#define NSSTATE_STOPPING    3
#define NSSTATE_FAILED      4

/* ns_Flags */
#define NSF_EXTERNAL        (1 << 0)    /* registered by an external daemon (signal-driven, no ops) */
#define NSF_IGNORERELOAD    (1 << 1)    /* leave untouched across a reload (NSRP_IGNORE) */

struct NetService;

struct NetServiceOps {
    /* Return 0 on success, else an error code.  May be NULL (no-op). */
    LONG (*nso_start)(struct NetService *svc);
    /* force==TRUE: tear down unconditionally (shutdown), else graceful. */
    LONG (*nso_stop)(struct NetService *svc, BOOL force);
};

struct NetService {
    struct Node                 ns_Node;    /* ln_Name=name, ln_Pri=order, ln_Type=NT_NETSERVICE */
    UWORD                       ns_State;    /* NSSTATE_* */
    UWORD                       ns_Pad;
    ULONG                       ns_Flags;    /* NSF_* */
    const struct NetServiceOps *ns_Ops;      /* internal services: direct start/stop (NULL for external) */
    struct Task                *ns_Task;     /* external services: owner task to signal */
    ULONG                       ns_SigBegin; /* external: reconfigure-begin (quiesce) signal mask */
    ULONG                       ns_SigEnd;   /* external: reconfigure-end (resume) signal mask */
    ULONG                       ns_SigStop;  /* external: stack-shutdown signal mask */
    APTR                        ns_Data;     /* service private data */
};

/*
 * Tags for RegisterNetService().  A signal tag takes a signal NUMBER (0..31,
 * as returned by AllocSignal()); -1 or an omitted tag means "do not signal for
 * this phase".
 */
#define NETSERVICE_TagBase          (TAG_USER | 0x4E530000)   /* 'NS' */

#define NETSERVICE_Name             (NETSERVICE_TagBase + 1)  /* (STRPTR)  service name (required) */
#define NETSERVICE_Order            (NETSERVICE_TagBase + 2)  /* (LONG)    bring-up priority; higher starts earlier, stops later */
#define NETSERVICE_Task             (NETSERVICE_TagBase + 3)  /* (struct Task *) task to signal (default: caller) */
#define NETSERVICE_ReconfigBeginSig (NETSERVICE_TagBase + 4)  /* (LONG)    signal no. for "reconfigure beginning - quiesce" */
#define NETSERVICE_ReconfigEndSig   (NETSERVICE_TagBase + 5)  /* (LONG)    signal no. for "reconfigure done - resume" */
#define NETSERVICE_StopSig          (NETSERVICE_TagBase + 6)  /* (LONG)    signal no. for "stack shutting down - stop" */
#define NETSERVICE_ReloadPolicy     (NETSERVICE_TagBase + 7)  /* (ULONG)   NSRP_* */

/* NETSERVICE_ReloadPolicy values */
#define NSRP_SIGNAL     0   /* signal the service across a reload (default) */
#define NSRP_IGNORE     1   /* leave the service untouched across a reload */
#define NSRP_RESTART    2   /* stop and relaunch the service across a reload */

/*
 * Tags for QueryNetServices().  Set NETSERVICE_QueryIndex to the 0-based index
 * of the service to describe; the GET tags name pointers the call fills in.
 * The call returns the total number of registered services, so a caller
 * iterates 0..count-1.
 */
#define NETSERVICE_QueryIndex       (NETSERVICE_TagBase + 16) /* (LONG)     which service to describe */
#define NETSERVICE_QueryName        (NETSERVICE_TagBase + 17) /* (STRPTR *) <- service name */
#define NETSERVICE_QueryState       (NETSERVICE_TagBase + 18) /* (ULONG *)  <- NSSTATE_* */
#define NETSERVICE_QueryOrder       (NETSERVICE_TagBase + 19) /* (LONG *)   <- bring-up priority */
#define NETSERVICE_QueryFlags       (NETSERVICE_TagBase + 20) /* (ULONG *)  <- NSF_* */

#endif /* LIBRARIES_NETSERVICE_H */
