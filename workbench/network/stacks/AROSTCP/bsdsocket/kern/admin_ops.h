/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef KERN_ADMIN_OPS_H
#define KERN_ADMIN_OPS_H

/*
 * THE ADMINISTRATION MODULE ("admin", admin.atcp): the routing
 * socket (PF_ROUTE, net/rtsock.c), the route, interface and name-server
 * calls of bsdsocket.library (api/rs_admin.c) and the glue of the control
 * panel (kern/amiga_gui.c). It is loaded the first time somebody looks at
 * or changes the running stack, not when the stack starts.
 *
 * The core keeps everything that is called by name from elsewhere, in
 * kern/admin_stubs.c: the library vectors (their names and places in the
 * vector table do not change), the three routing messages the rest of the
 * stack sends, the panel calls of NETTRACE, and the panel's variables that
 * NETTRACE and the configuration read. A stub finds the module's functions
 * through the demand slot `admin_demand' (net/netdemand.h): the table
 * below, put there by the module when it starts and taken away when it
 * stops.
 *
 * What the stubs answer while the module cannot be had (its library is
 * not installed, or ENV:AROSTCP/Without_admin is set): what each call
 * answers today when it cannot do its work - the failure value, and the
 * reason (ENOENT) in errno. socket(PF_ROUTE) fails with EPROTONOSUPPORT,
 * as for any family the stack does not have. The route and interface
 * ioctls, the static routes of the configuration and the DHCP client
 * need nothing of the module: they are the core's.
 */

#include <exec/types.h>

struct SocketBase;
struct TagItem;
struct List;
struct rt_msghdr;
struct sockaddr;
struct ifnet;
struct ifaddr;
struct rtentry;
struct SysLogPacket;

#define ADMIN_OPS_VERSION   1

struct admin_ops {
    unsigned short   ao_size;       /* sizeof(struct admin_ops) */
    unsigned short   ao_version;    /* ADMIN_OPS_VERSION */

    /* bsdsocket.library: vectors 69-71, 73, 77, 86, 87, 89, 117 */
    long              (*ao_AddRouteTagList)(struct TagItem *tags, struct SocketBase *libPtr);
    long              (*ao_DeleteRouteTagList)(struct TagItem *tags, struct SocketBase *libPtr);
    long              (*ao_ChangeRouteTagList)(struct TagItem *tags, struct SocketBase *libPtr);
    struct rt_msghdr *(*ao_GetRouteInfo)(LONG address_family, LONG flags, struct SocketBase *libPtr);
    struct List      *(*ao_ObtainInterfaceList)(struct SocketBase *libPtr);
    LONG              (*ao_AddDomainNameServer)(STRPTR address, struct SocketBase *libPtr);
    LONG              (*ao_RemoveDomainNameServer)(STRPTR address, struct SocketBase *libPtr);
    struct List      *(*ao_ObtainDomainNameServerList)(struct SocketBase *libPtr);
    LONG              (*ao_GetDefaultDomainName)(STRPTR buffer, LONG buffer_size, struct SocketBase *libPtr);

    /* the routing messages (the stubs call these only while a routing socket exists) */
    void (*ao_rt_missmsg)(int type, struct sockaddr *dst, struct sockaddr *gate,
                          struct sockaddr *mask, struct sockaddr *src, int flags, int error);
    void (*ao_rt_ifmsg)(struct ifnet *ifp);
    void (*ao_rt_newaddrmsg)(int cmd, struct ifaddr *ifa, int error, struct rtentry *rt);

    /* the panel, in the NETTRACE task (kern/amiga_log.c) */
    void (*ao_gui_open)(void);
    void (*ao_gui_close)(void);
    void (*ao_gui_process_refresh)(void);
    void (*ao_gui_process_msg)(struct SysLogPacket *msg);    /* only while GUI_Running */
};

/*
 * A panel command of NETTRACE's message queue besides those of
 * kern/amiga_gui.h (GUICMD_SET_INTERFACE_STATE 1, GUICMD_CLOSE 2): open the
 * panel, now that the module is loaded. Sent by the core's panel loader
 * (kern/admin_stubs.c), never by the panel library.
 */
#define ADMIN_GUICMD_OPEN   0x0003

/*
 * Two variables of the core whose names are also structure tags: a module
 * names them by these aliases (as `ifnet_list' stands for `ifnet'), which
 * the core's services table can map.
 */
struct route_cb;
struct radix_node_head;
extern struct route_cb route_cb;        /* net/route.c: the routing sockets, counted */
#define route_counts    route_cb
#define rnh_list        radix_node_head /* net/radix.c: the routing tables */

#ifdef KERNEL
struct netdemand;
/* the core's slot (kern/admin_stubs.c) */
extern struct netdemand admin_demand;

/*
 * The module's functions behind the table (net/rtsock.c, api/rs_admin.c,
 * kern/amiga_gui.c). The vectors' functions keep the arguments of the
 * vector, the base last.
 */
long  rsadm_AddRouteTagList(struct TagItem *tags, struct SocketBase *libPtr);
long  rsadm_DeleteRouteTagList(struct TagItem *tags, struct SocketBase *libPtr);
long  rsadm_ChangeRouteTagList(struct TagItem *tags, struct SocketBase *libPtr);
struct rt_msghdr *rsadm_GetRouteInfo(LONG address_family, LONG flags, struct SocketBase *libPtr);
struct List *rsadm_ObtainInterfaceList(struct SocketBase *libPtr);
LONG  rsadm_AddDomainNameServer(STRPTR address, struct SocketBase *libPtr);
LONG  rsadm_RemoveDomainNameServer(STRPTR address, struct SocketBase *libPtr);
struct List *rsadm_ObtainDomainNameServerList(struct SocketBase *libPtr);
LONG  rsadm_GetDefaultDomainName(STRPTR buffer, LONG buffer_size, struct SocketBase *libPtr);

struct domain;
extern struct domain routedomain;   /* net/rtsock.c */
int   rts_attach(void);              /* the routing domain into the stack's list */
int   rts_detach(void);              /* ... and out: EBUSY while a routing socket exists */
void  rts_missmsg(int type, struct sockaddr *dst, struct sockaddr *gate,
                  struct sockaddr *mask, struct sockaddr *src, int flags, int error);
void  rts_ifmsg(struct ifnet *ifp);
void  rts_newaddrmsg(int cmd, struct ifaddr *ifa, int error, struct rtentry *rt);

void  admin_gui_open(void);
void  admin_gui_close(void);
void  admin_gui_process_refresh(void);
void  admin_gui_process_msg(struct SysLogPacket *msg);
/* the panel's helper processes that run the module's code now (kern/amiga_gui.c) */
extern volatile LONG admin_gui_procs;

/*
 * The panel's variables, the core's (kern/admin_stubs.c) because NETTRACE
 * and the configuration read them by name; the module sets them while it
 * opens and closes the panel. (Declared here as kern/amiga_gui.h does,
 * which net/netcore.h cannot include: it has no guard against a second
 * inclusion.)
 */
struct timerequest;
extern UBYTE GUI_Running;               /* the panel is open */
extern ULONG guimask;                   /* its signals, which NETTRACE waits for */
extern struct timerequest *gui_timerio; /* its refresh timer */
extern ULONG InternalProc;              /* the stack's helper processes that run now */
struct gui_cnf;
extern struct gui_cnf gui_cnf;          /* GUI PANEL=, FONT= */
#define gui_config      gui_cnf         /* the alias a module names it by: also a structure tag */
extern LONG gui_show[6];                /* GUI SHOW= */
extern ULONG gui_refresh;               /* the panel's refresh, in seconds */
struct Library;
extern struct Library *MiamiPanelBase;  /* the panel library while it is open */
#endif /* KERNEL */

#endif /* !KERN_ADMIN_OPS_H */
