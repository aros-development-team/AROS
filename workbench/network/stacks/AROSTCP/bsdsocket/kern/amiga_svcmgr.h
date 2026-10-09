/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Service manager: the stack's launcher for EXTERNAL service daemons.
 *
 * A data-driven manifest (db/netservices) lists daemons the stack should run:
 *     <name> <path> [order] [stopsignal]
 * one per line ('#' and ';' begin comments).  svcmgr launches each with
 * LoadSeg()+CreateNewProc() (as amiga_dhcp.c already does for dhclient),
 * tracks it, and on stop signals it and waits for it to exit (via NP_ExitCode)
 * before unloading - so "stop" means the daemon is really gone, not just told
 * to go.  The daemon itself OpenLibrary()s bsdsocket + netservices and
 * RegisterNetService()s, so it participates in the reload begin/end signalling
 * through netservices.library; svcmgr only owns its PROCESS lifecycle.
 *
 * This runs as one internal netservice ("servicemgr", lowest priority: started
 * last, stopped first) so boot/reload/shutdown drive the daemons through the
 * same registry as everything else.
 */

#ifndef AMIGA_SVCMGR_H
#define AMIGA_SVCMGR_H

#include <exec/types.h>

/* Register the "servicemgr" netservice (called from net_services_register). */
void svcmgr_register(void);

/* Launch every manifest daemon not already running (service start). */
void svcmgr_launch_all(void);

/* Signal every launched daemon to stop and wait for it to exit (service stop). */
void svcmgr_stop_all(BOOL force);

#endif /* AMIGA_SVCMGR_H */
