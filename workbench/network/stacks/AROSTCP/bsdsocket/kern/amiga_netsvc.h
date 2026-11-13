/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef AMIGA_NETSVC_H
#define AMIGA_NETSVC_H

/*
 * Built-in network services and the in-place reload orchestration.
 *
 * net_services_register() registers the stack's built-in lifecycle units
 * (resolver / interfaces / dhcp / rc-daemons) with the service registry; call
 * it once from init_all() after the subsystems are up.  net_reload() performs
 * an in-place reconfigure (stop all services in reverse order, re-read the
 * config, start them again) without exiting the stack task.
 */

void net_services_register(void);
LONG net_reload(void);

#endif /* AMIGA_NETSVC_H */
