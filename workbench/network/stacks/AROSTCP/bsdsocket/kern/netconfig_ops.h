/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef KERN_NETCONFIG_OPS_H
#define KERN_NETCONFIG_OPS_H

/*
 * THE CONFIGURATION READERS AS A MODULE (module "netconfig",
 * netconfig.atcp; maintainer's decision D19).
 *
 * The code that reads and parses the configuration files - the command
 * line and db/general.config, db/netdb and the files it includes with
 * WITH (interfaces, hosts, services, protocols, networks, inet.access,
 * static-routes, netdb-myhost ...), and the service files of
 * db/services.d - is a module that is only in memory while a
 * configuration is read:
 *
 *  - at start-up: the stack's task loads it before anything else that
 *    the configuration decides (PROTOCOLS= is read by it, the interface
 *    lines say what starts), and drops it again when the start is done;
 *  - at every reload (NCMD_RELOAD): loaded before the old configuration
 *    is taken down, dropped after the new one is in place. When it cannot
 *    be had the reload is refused and the stack keeps its configuration;
 *  - for an ARexx command that reads configuration text (ADD, RESET,
 *    SET WITH=file): NETTRACE, which serves the ARexx port, has the
 *    stack's task load it (net/netdemand.h) and asks for it to be dropped
 *    after the command.
 *
 * It stays loaded only while an interface line marked DEFER waits for its
 * device: the stack's timer applies such a line through it every
 * NETDB_DEFER_SECS seconds, and must not load a library itself (it runs
 * with the stack's spl level raised). nm_can_stop refuses while a line
 * waits; when the last one is applied the reader is dropped.
 *
 * What the readers FILL IN is the core's and stays there: the network
 * database (NDB, its lists, ndb_Lock, ndb_Serial), the dynamic database
 * and its run-time updates (DHCP, Router Advertisements, DNS=), the
 * deferred interface lines, the variables of general.config and their
 * ARexx QUERY/SET (kern/amiga_config.c, kern/config_var.c), every look-up.
 *
 * The functions the core always called by these names - readconfig(),
 * init_netdb(), netdb_reload(), do_netdb(), reset_netdb(), read_sets(),
 * svcmgr_launch_all(), netdb_defer_timer() - stay in the core as stubs
 * (kern/netconfig_stubs.c and the files that own them) that call through
 * the table below.
 */

#include <exec/types.h>
#include <dos/rdargs.h>

struct CSource;
struct NetDataBase;
struct ifnet;
struct ssconfig;
struct in6_addr;

struct netconfig_ops {
    unsigned short nco_size;    /* sizeof(struct netconfig_ops) of the module */

    /* the command line and db/general.config (or WITH's file) */
    BOOL (*nco_readconfig)(void);
    /* one file of general.config's syntax: SET WITH=file (read_sets()) */
    LONG (*nco_parsefile)(UBYTE const *name, UBYTE **errstrp, struct CSource *res);
    /* db/netdb at start: NDB made and filled, interfaces made and set up */
    LONG (*nco_netdb_init)(void);
    /* db/netdb again, in place (the "interfaces" service of a reload) */
    LONG (*nco_netdb_reload)(void);
    /* ARexx ADD <line>: one line of the netdb's syntax */
    LONG (*nco_netdb_add)(struct CSource *cs, UBYTE **errstrp, struct CSource *res);
    /* ARexx RESET: the database read again, interfaces left alone */
    LONG (*nco_netdb_reset)(struct CSource *cs, UBYTE **errstrp, struct CSource *res);
    /*
     * One deferred interface line tried again (the stack's timer, with
     * NDB locked): 1 its device still does not open, 0 it is done with
     * (the interface was made, or the line turned out unusable - then
     * *errstrp says why, if it can).
     */
    int  (*nco_defer_retry)(const char *line, ULONG length, UBYTE **errstrp,
                            struct CSource *res);
    /* db/services.d: every service file read and applied (svcmgr) */
    void (*nco_services_scan)(void);
};

#define NETCONFIG_MODULE "netconfig"

#ifdef KERNEL
/*
 * THE CORE'S SIDE (kern/netconfig_stubs.c).
 *
 * netconfig_acquire(): the stack's task, before a configuration is read
 * (start-up, NCMD_RELOAD): the reader started, its library loaded first
 * when it is not part of the program. 0, or why not (ENOENT: not
 * installed, or its test switch ENV:AROSTCP/Without_netconfig is set; the
 * loader's error otherwise) - the log says which, once.
 * netconfig_release(): the stack's task, after it: the reader stopped and
 * its library unloaded - unless a deferred interface line waits for it,
 * or a task is inside it (that task has it dropped when it is done).
 *
 * netconfig_enter() / netconfig_leave(): around one call through the
 * table, from any task, never with the socket calls fenced by the caller
 * (net/netdemand.h). From a task that is not the stack's, leave asks the
 * stack's task to drop the reader again. NULL: it cannot be had, *errorp
 * says why.
 *
 * netconfig_keep_error(): an error text the reader returned may be its
 * own constant, which leaves memory with it: copied into the caller's
 * reply buffer, where the core's own error texts are written too.
 */
int  netconfig_acquire(void);
void netconfig_release(void);
const struct netconfig_ops *netconfig_enter(int *errorp);
void netconfig_leave(void);
void netconfig_keep_error(UBYTE **errstrp, struct CSource *res);
/* the reader's table when it runs, entered without loading it (the timer) */
const struct netconfig_ops *netconfig_running(void);
/* init_netdb()'s read (kern/amiga_netdb.c) */
LONG netconfig_netdb_init(void);
/* the text a caller gets when the reader cannot be had */
extern UBYTE ERR_NOCONFIG[];

/*
 * WHAT THE READER USES OF THE CORE besides what other headers declare
 * (all in the core's table, net/netcore.def). Declared here as the files
 * that define them have them, so that net/netcore.h can include this
 * header: it declares, it includes nothing of the stack.
 */
/* kern/amiga_netdb.c */
int  inet6_aton(const char *src, struct in6_addr *dst);
void netdb_access_setup(struct NetDataBase *ndb);
int  netdb_defer_add(const char *name, const char *line, ULONG length);
void netdb_defer_forget(const char *name);
int  netdb_defer_pending(void);
/* api/amiga_libcalls.c */
ULONG __inet_network(const char *cp);
/* net/sana2copybuff.c (also in net/sana2request.h, which has inline
 * functions and so is not for net/netcore.h): the buffer management a
 * device is opened with */
#include <utility/tagitem.h>
extern struct TagItem buffermanagement[];
/* net/sana2perror.c */
struct IOSana2Req;
void sana2perror(const char *banner, struct IOSana2Req *ios2);
/* kern/uipc_mbuf.c */
void mb_autosize(ULONG mtu);
/* kern/amiga_request.c (also in kern/amiga_gui.h) */
void error_request(STRPTR Text, ...);
/* kern/amiga_svcmgr.c: the service manager's own records */
struct SvcProc;
struct SvcProc *svcmgr_find(CONST_STRPTR name);
LONG svcmgr_launch(CONST_STRPTR name, CONST_STRPTR path, ULONG stopsig, ULONG policy);
void svcmgr_stop_one(struct SvcProc *sp, BOOL force);

/*
 * THE READER'S OWN, between its files (kern/netconfig_iface.c): making an
 * interface of a line, the requester when its device does not open, the
 * generation of a reload. Called only by the reader.
 */
struct ifnet *iface_make(struct ssconfig *ifc);
void sana_reload_begin(void);
void sana_mark_seen(struct ifnet *ifp);
void sana_remove_unseen(void);
LONG iface_fail_request(CONST_STRPTR name, CONST_STRPTR device, LONG unit);
#endif /* KERNEL */

#endif /* !KERN_NETCONFIG_OPS_H */
