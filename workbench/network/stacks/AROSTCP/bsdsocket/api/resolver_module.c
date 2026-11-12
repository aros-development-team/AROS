/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The resolver as a module (api/resolver_ops.h): module "resolver", in
 * resolver.atcp, or - in a monolithic program, or while
 * conf/group-resolver.cfg builds it into the program - part of the stack
 * program, found through the symbol set NETMODULES. Either way it is not
 * started with the stack: the first name look-up has it started (the
 * core's demand slot "resolver"), and with MODULES=RESOLVER it is started
 * at start-up.
 *
 * It needs no protocol family: it asks a name server with the core's
 * socket calls, of whatever family the server's address is.
 *
 * What it owns: the DNS cache (static data, its entries from the core's
 * pool) and its table in the core's slot. No timer, task, port, socket or
 * exec memory outlives a call: a look-up's buffers are freed before it
 * returns, a getaddrinfo() list is exec memory the core frees, and the
 * resolver state and hostent buffers are the library bases' (the core's).
 *
 * Stopping is refused while a task is inside a look-up - possibly asleep
 * waiting for a name server - which the slot counts (EBUSY from
 * netdemand_set()). After a stop the next look-up starts it again, and
 * after an unload loads it again.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <net/netmodule.h>
#include <net/netdemand.h>
#include <api/resolver_ops.h>

#include <net/netcore_map.h>        /* always the last include */

static const struct resolver_ops resolver_ops = {
    sizeof(struct resolver_ops),
    res_gethostbyname2,
    res_gethostbyaddr,
    res_gethostbyname_r,
    res_gethostbyaddr_r,
    res_getaddrinfo,
    res_getnameinfo
};

static int
resolver_start(struct netmodule *nm)
{
    /* the cache first: once the table is in the slot, look-ups come */
    dns_cache_start();
    return (netdemand_set(RESOLVER_MODULE, &resolver_ops));
}

static int
resolver_stop(struct netmodule *nm)
{
    int error;

    /* refused, with nothing changed, while a task is inside a look-up */
    if((error = netdemand_set(RESOLVER_MODULE, NULL)) != 0)
        return (error);
    dns_cache_stop();
    return (0);
}

/*
 * nm_shutdown is NULL: what the module holds besides static data is the
 * cache's entries, the core's pool memory, freed in one piece by the core.
 * The generic test switch AROSTCP/Without_resolver keeps it out
 * (netmodule_leaveout_name()); it needs no variable of its own.
 */
NETMODULE_DESCRIPTOR(resolver_module) = {
    sizeof(struct netmodule), NETMODULE_VERSION, RESOLVER_MODULE, 0,
    resolver_start, resolver_stop, NULL,
    0, NULL,
    NULL,           /* no domain */
    NULL,           /* needs no other module */
    NULL,           /* nm_leaveout: the generic switch */
    NULL,           /* nm_library: the loader's */
    NULL            /* nm_shutdown */
};
NETMODULE_BUILTIN(resolver_module, 40)
