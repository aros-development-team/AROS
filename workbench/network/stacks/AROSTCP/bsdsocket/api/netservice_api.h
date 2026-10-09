/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * netservices.library - stack-internal interface.
 *
 * The public interface (the library base, the NetService structures and the
 * RegisterNetService/UnregisterNetService/QueryNetServices LVO calls) lives in
 * <libraries/netservice.h>.  The registry data and logic live in this library
 * (netservice_api.c), NOT in bsdsocket; the stack drives boot/shutdown/reload
 * through the plain C entry points declared here (same binary, no LVO), while
 * external daemons use the LVO calls.
 */

#ifndef API_NETSERVICE_API_H
#define API_NETSERVICE_API_H

#include <exec/types.h>
#include <libraries/netservice.h>

/* Reconfigure phases for netservice_signal_external(). */
#define NSPHASE_BEGIN   0       /* reconfigure starting: signal services to quiesce (teardown order) */
#define NSPHASE_END     1       /* reconfigure done: signal services to resume (bring-up order) */
#define NSPHASE_STOP    2       /* stack shutting down: signal services to stop (teardown order) */

/* Registry lifecycle, called once when the library base is created/destroyed. */
void netservice_registry_init(void);
void netservice_registry_deinit(void);

/*
 * Low-level (un)register of a caller-owned node - used by the stack for its own
 * internal, op-driven services.  The caller fills ln_Name, ln_Pri and ns_Ops
 * before registering, and owns the storage (no free on unregister).
 */
void netservice_register(struct NetService *svc);
void netservice_unregister(struct NetService *svc);

/* Look a service up by name (ln_Name), or NULL. */
struct NetService *netservice_find(CONST_STRPTR name);

/*
 * Start every registered (op-driven) service in descending-priority order, or
 * stop every one in reverse.  External (signal-driven) services carry no ops
 * and are skipped here - they are driven by netservice_signal_external().
 * start_all stops at the first failing start and returns its error; stop_all
 * always walks the whole list.
 */
LONG netservice_start_all(void);
void netservice_stop_all(BOOL force);

/*
 * Signal every EXTERNAL (signal-driven) service for a reconfigure phase, in the
 * correct order (NSPHASE_BEGIN/STOP: teardown order; NSPHASE_END: bring-up
 * order).  A service that did not ask for a signal in the given phase, or whose
 * policy is NSRP_IGNORE, is skipped.
 */
void netservice_signal_external(ULONG phase);

#endif /* API_NETSERVICE_API_H */
