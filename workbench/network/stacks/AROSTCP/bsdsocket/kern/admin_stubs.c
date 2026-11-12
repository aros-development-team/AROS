/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * THE CORE'S SIDE OF THE ADMINISTRATION MODULE (kern/admin_ops.h): what
 * the rest of the stack and the applications call by name, whether the
 * module is loaded or not.
 *
 *  - The library vectors of the route, interface and name-server calls.
 *    Those that do work call the module through the demand slot, which
 *    loads it on the first call; the answer when it cannot be had is the
 *    call's failure value with ENOENT in errno. Those that give a list
 *    back, and those that are not implemented, are here whole: they need
 *    nothing of the module, and an answer of "not implemented" is not
 *    worth reading a file from disk for.
 *  - The routing messages the stack sends (rt_missmsg(), rt_ifmsg(),
 *    rt_newaddrmsg()): one test of the count of routing sockets while
 *    there is none - which is always, while the module is not loaded -
 *    and the signal to the applications that asked for address changes,
 *    which does not depend on the module.
 *  - The control panel's calls from NETTRACE, the panel's variables that
 *    NETTRACE and the configuration read, gui_set_interface_state() of
 *    the interfaces, and the part of a panel message that is not about
 *    the panel.
 *
 * NETTRACE NEVER WAITS FOR THE MODULE. A module is loaded by the stack's
 * own task (net/netdemand.h); a task that needs one loaded waits for the
 * stack's task. The stack's task, in turn, can wait for NETTRACE: for a
 * free log message (GetLogMsg(), kern/amiga_log.c), which NETTRACE hands
 * back. So NETTRACE must not wait for a load, or the two could wait for
 * each other. When the panel is to be opened and the module does not run,
 * gui_open() starts a small process of the core instead, which has the
 * module loaded and then asks NETTRACE, through its message port, to open
 * the panel (ADMIN_GUICMD_OPEN); NETTRACE goes on with its work
 * meanwhile. Every other panel call of NETTRACE calls the module only
 * while it runs, and loads nothing. (For the same reason the ARexx
 * reports of kern/amiga_cstat.c, which NETTRACE serves and must answer at
 * once, stay in the core.)
 */

#include <conf.h>

#include <aros/libcall.h>
#include <aros/atomic.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <devices/timer.h>
#include <libraries/bsdsocket.h>
#include <libraries/miamipanel.h>
#include <utility/tagitem.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/synch.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <net/route.h>
#include <net/if.h>
#include <net/netmodule.h>
#include <protos/net/rtsock_protos.h>
#include <syslog.h>
#include <api/amiga_api.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_log.h>
#include <kern/amiga_gui.h>
#include <kern/admin_ops.h>

/* the slot through which the stubs find the module (net/netdemand.h) */
struct netdemand admin_demand = { "admin" };
NETDEMAND_SLOT(admin_demand);

/*
 * THE PANEL'S VARIABLES (kern/admin_ops.h): read by NETTRACE and by the
 * configuration, set by the module while it opens and closes the panel.
 */
struct gui_cnf gui_cnf = {NULL};
LONG gui_show[6] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};
ULONG gui_refresh = 1;

UBYTE GUI_Running = 0;
ULONG guimask = 0;
struct Library *MiamiPanelBase = NULL;
struct timerequest *gui_timerio = NULL;
ULONG InternalProc = 0;         /* the stack's helper processes that run now */

/*
 * The module's table if it runs, counted as a call in progress (give it
 * back with netdemand_leave()); NULL if it does not run. Never loads it.
 */
static const struct admin_ops *
admin_peek(void)
{
    const struct admin_ops *ao;

    AROS_ATOMIC_INC(admin_demand.nd_busy);
    if((ao = (const struct admin_ops *)admin_demand.nd_ops) == NULL)
        AROS_ATOMIC_DEC(admin_demand.nd_busy);
    return (ao);
}

/* The module's table, loading it first if need be; NULL with errno set when it cannot be had. */
static const struct admin_ops *
admin_enter(struct SocketBase *libPtr)
{
    const void *ops;
    int error;

    if((error = netdemand_enter_error(&admin_demand, &ops)) != 0) {
        writeErrnoValue(libPtr, error);
        return (NULL);
    }
    return ((const struct admin_ops *)ops);
}

#define admin_leave()   netdemand_leave(&admin_demand)

/*
 * A list the module handed to an application (a route table, the list of
 * interfaces, of name servers) keeps the module from being stopped until
 * it is given back: it stays counted in the slot as a call in progress.
 * The lists are the core's memory and are freed here; the count is kept
 * beside the slot's so that a list given back twice cannot take away the
 * count of a task that is inside the module.
 */
static volatile LONG admin_lists = 0;

static void
admin_list_out(const void *list)
{
    if(list != NULL)
        AROS_ATOMIC_INC(admin_lists);
    else
        admin_leave();
}

static void
admin_list_back(void)
{
    BOOL counted = FALSE;

    Forbid();
    if(admin_lists > 0) {
        admin_lists--;
        counted = TRUE;
    }
    Permit();
    if(counted)
        admin_leave();
}

/*
 * THE ROUTING MESSAGES. The count of routing sockets is the core's
 * (net/route.c); it is not 0 only while the module runs, since the module
 * attaches the domain those sockets are of and is not stopped while one
 * exists. The table is tested all the same: it costs one load.
 */
void
rt_missmsg(int type, struct sockaddr *dst, struct sockaddr *gate,
           struct sockaddr *mask, struct sockaddr *src, int flags, int error)
{
    const struct admin_ops *ao;

    if(route_counts.any_count == 0 ||
            (ao = (const struct admin_ops *)admin_demand.nd_ops) == NULL)
        return;
    ao->ao_rt_missmsg(type, dst, gate, mask, src, flags, error);
}

void
rt_ifmsg(struct ifnet *ifp)
{
    const struct admin_ops *ao;

    if(route_counts.any_count == 0 ||
            (ao = (const struct admin_ops *)admin_demand.nd_ops) == NULL)
        return;
    ao->ao_rt_ifmsg(ifp);
}

void
rt_newaddrmsg(int cmd, struct ifaddr *ifa, int error, struct rtentry *rt)
{
    const struct admin_ops *ao;

    /*
     * Wake SBTC_SIG_ADDRESS_CHANGE_MASK subscribers on every address
     * add/remove, whether a routing socket is listening or not - and
     * whether the module is loaded or not.
     */
    api_sendaddrchange();

    if(route_counts.any_count == 0 ||
            (ao = (const struct admin_ops *)admin_demand.nd_ops) == NULL)
        return;
    ao->ao_rt_newaddrmsg(cmd, ifa, error, rt);
}

/*
 * THE CONTROL PANEL, in the NETTRACE task.
 */

/* the panel's library is there to be opened (as admin_gui_open() names it) */
static BOOL
admin_panel_installed(void)
{
    TEXT path[128];
    BPTR lock;
    int len;

    path[0] = '\0';
    if(!AddPart(path, "LIBS:", sizeof(path)) || !AddPart(path, gui_cnf.PanelName, sizeof(path)))
        return (FALSE);
    len = strnlen(path, sizeof(path));
    if(len + sizeof(".MiamiPanel") > sizeof(path))
        return (FALSE);
    memcpy(path + len, ".MiamiPanel", sizeof(".MiamiPanel"));
    if((lock = Lock(path, ACCESS_READ)) == BNULL)
        return (FALSE);
    UnLock(lock);
    return (TRUE);
}

static volatile UBYTE admin_panel_loading = 0;

/*
 * The panel loader: a process of the core's code. It waits for the stack's
 * task to load the module - which NETTRACE must not do - and then asks
 * NETTRACE to open the panel. It ends in Forbid(), as the stack's other
 * helper processes do.
 */
static void SAVEDS
admin_panel_loader(void)
{
    struct SysLogPacket *msg;

    if(netdemand_load(admin_demand.nd_module) == 0 &&
            (msg = (struct SysLogPacket *)GetLogMsg(&logReplyPort)) != NULL) {
        msg->Level = LOG_GUIMSG | ADMIN_GUICMD_OPEN;
        PutMsg(logPort, (struct Message *)msg);
    }
    Forbid();
    admin_panel_loading = 0;
    InternalProc--;
}

void
gui_open(void)
{
    const struct admin_ops *ao;
    BOOL started;

#if defined(__AROS__)
    D(bug("[AROSTCP](admin_stubs.c) %s()\n", __func__));
#endif

    if(GUI_Running || gui_cnf.PanelName == NULL)
        return;
    if((ao = admin_peek()) != NULL) {
        ao->ao_gui_open();
        admin_leave();
        return;
    }
    /*
     * The module is not running: have it loaded without waiting here -
     * unless it could not be had before, or there is no panel to open
     * with it (then it is not loaded for nothing).
     */
    if(admin_panel_loading || admin_demand.nd_failed || !admin_panel_installed())
        return;
    admin_panel_loading = 1;
    Forbid();
    InternalProc++;
    Permit();
    started = CreateNewProcTags(NP_Entry, (IPTR)&admin_panel_loader,
                                NP_Name, (IPTR)"AROSTCP panel loader",
#ifdef __MORPHOS__
                                NP_CodeType, CODETYPE_PPC,
#endif
                                TAG_DONE) != NULL;
    if(!started) {
        Forbid();
        InternalProc--;
        Permit();
        admin_panel_loading = 0;
    }
}

void
gui_close(void)
{
    const struct admin_ops *ao;

#if defined(__AROS__)
    D(bug("[AROSTCP](admin_stubs.c) %s()\n", __func__));
#endif

    /* nothing is open without gui_timerio; and while it is set, the module is not stopped */
    if(gui_timerio == NULL || (ao = admin_peek()) == NULL)
        return;
    ao->ao_gui_close();
    admin_leave();
}

void
gui_snapshot(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](admin_stubs.c) %s()\n", __func__));
#endif
    /* TODO */
}

void
gui_process_refresh(void)
{
    const struct admin_ops *ao;

    if(!GUI_Running || (ao = admin_peek()) == NULL)
        return;
    ao->ao_gui_process_refresh();
    admin_leave();
}

void
gui_process_msg(struct SysLogPacket *msg)
{
    const struct admin_ops *ao;
    struct ifnet *ifp;
    long state;

#if defined(__AROS__)
    D(bug("[AROSTCP](admin_stubs.c) %s()\n", __func__));
#endif

    /* the panel loader has had the module loaded: open the panel (once; no new load from here) */
    if((msg->Level & GUICMD_MASK) == ADMIN_GUICMD_OPEN) {
        if(!GUI_Running && gui_cnf.PanelName != NULL && (ao = admin_peek()) != NULL) {
            ao->ao_gui_open();
            admin_leave();
        }
        return;
    }
    if(GUI_Running && (ao = admin_peek()) != NULL) {
        ao->ao_gui_process_msg(msg);
        admin_leave();
    }
    if((msg->Level & GUICMD_MASK) == GUICMD_SET_INTERFACE_STATE) {
        /* the DHCP handling runs whether the panel is up or not */
        ifp = (struct ifnet *)msg->Time;
        state = (long)msg->Tag;
        /* No run_dhclient() on Online: dhclient's own ifup sends it, and it
         * is handled late, after a reload's end. sana_connect() covers a
         * link that came up. */
        if(ifp && state == MIAMIPANELV_AddInterface_State_GoingOffline) {
            kill_dhclient(ifp);
#if INET6 && DHCP6
            kill_dhclient6(ifp);
#endif
        }
    }
}

void
gui_set_interface_state(struct ifnet *ifp, long state)
{
    struct SysLogPacket *msg;

#if defined(__AROS__)
    D(bug("[AROSTCP](admin_stubs.c) %s()\n", __func__));
#endif

    if(GUI_Running) {
        if(msg = (struct SysLogPacket *)GetLogMsg(&logReplyPort)) {
            msg->Level = LOG_GUIMSG | GUICMD_SET_INTERFACE_STATE;
            msg->Time = (IPTR)ifp;
            msg->Tag = (STRPTR)state;
            PutMsg(logPort, (struct Message *)msg);
        }
    }
}

#ifdef __CONFIG_ROADSHOW__
/*
 * THE LIBRARY VECTORS (bsdsocket.library, the Roadshow interface). The
 * module's are called with the socket calls not fenced: they fence what
 * they need themselves, and netdemand_enter() must never be called with
 * the fence held (net/netdemand.h).
 */
AROS_LH1(long, AddRouteTagList,
         AROS_LHA(struct TagItem *, tags, A0),
         struct SocketBase *, libPtr, 69, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    long r;

    if((ao = admin_enter(libPtr)) == NULL)
        return -1;
    r = ao->ao_AddRouteTagList(tags, libPtr);
    admin_leave();
    return r;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, DeleteRouteTagList,
         AROS_LHA(struct TagItem *, tags, A0),
         struct SocketBase *, libPtr, 70, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    long r;

    if((ao = admin_enter(libPtr)) == NULL)
        return -1;
    r = ao->ao_DeleteRouteTagList(tags, libPtr);
    admin_leave();
    return r;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, ChangeRouteTagList,
         AROS_LHA(struct TagItem *, tags, A0),
         struct SocketBase *, libPtr, 71, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    long r;

    if((ao = admin_enter(libPtr)) == NULL)
        return -1;
    r = ao->ao_ChangeRouteTagList(tags, libPtr);
    admin_leave();
    return r;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct rt_msghdr *, GetRouteInfo,
         AROS_LHA(LONG, address_family, D0),
         AROS_LHA(LONG, flags, D1),
         struct SocketBase *, libPtr, 73, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    struct rt_msghdr *table;

    if((ao = admin_enter(libPtr)) == NULL)
        return NULL;
    table = ao->ao_GetRouteInfo(address_family, flags, libPtr);
    admin_list_out(table);
    return table;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct List *, ObtainInterfaceList,
         struct SocketBase *, libPtr, 77, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    struct List *list;

    if((ao = admin_enter(libPtr)) == NULL)
        return NULL;
    list = ao->ao_ObtainInterfaceList(libPtr);
    admin_list_out(list);
    return list;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AddDomainNameServer,
         AROS_LHA(STRPTR, address, A0),
         struct SocketBase *, libPtr, 86, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    LONG r;

    if((ao = admin_enter(libPtr)) == NULL)
        return -1;
    r = ao->ao_AddDomainNameServer(address, libPtr);
    admin_leave();
    return r;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, RemoveDomainNameServer,
         AROS_LHA(STRPTR, address, A0),
         struct SocketBase *, libPtr, 87, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    LONG r;

    if((ao = admin_enter(libPtr)) == NULL)
        return -1;
    r = ao->ao_RemoveDomainNameServer(address, libPtr);
    admin_leave();
    return r;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct List *, ObtainDomainNameServerList,
         struct SocketBase *, libPtr, 89, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    struct List *list;

    if((ao = admin_enter(libPtr)) == NULL)
        return NULL;
    list = ao->ao_ObtainDomainNameServerList(libPtr);
    admin_list_out(list);
    return list;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 117: GetDefaultDomainName                                     */
/* ------------------------------------------------------------------ */
AROS_LH2(LONG, GetDefaultDomainName,
         AROS_LHA(STRPTR, buffer, A0),
         AROS_LHA(LONG, buffer_size, D0),
         struct SocketBase *, libPtr, 117, UL)
{
    AROS_LIBFUNC_INIT
    const struct admin_ops *ao;
    LONG r;

    if (buffer == NULL || buffer_size <= 0)
        return 0;
    if((ao = admin_enter(libPtr)) == NULL) {
        buffer[0] = '\0';
        return 0; /* FALSE */
    }
    r = ao->ao_GetDefaultDomainName(buffer, buffer_size, libPtr);
    admin_leave();
    return r;
    AROS_LIBFUNC_EXIT
}

/*
 * What gives a list back: the lists are the core's memory (bsd_malloc(),
 * AllocVec()), freed here whether the module runs or not.
 */
AROS_LH1(void, FreeRouteInfo,
         AROS_LHA(struct rt_msghdr *, table, A0),
         struct SocketBase *, libPtr, 72, UL)
{
    AROS_LIBFUNC_INIT

    if(table != NULL) {
        bsd_free(table, NULL);
        admin_list_back();
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, ReleaseInterfaceList,
         AROS_LHA(struct List *, list, A0),
         struct SocketBase *, libPtr, 76, UL)
{
    AROS_LIBFUNC_INIT
    struct Node *n;

    if(list == NULL)
        return;
    while((n = RemHead(list)) != NULL)
        FreeVec(n);
    FreeVec(list);
    admin_list_back();
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, ReleaseDomainNameServerList,
         AROS_LHA(struct List *, list, A0),
         struct SocketBase *, libPtr, 88, UL)
{
    AROS_LIBFUNC_INIT

    struct Node *node, *nnode;

    if(list == NULL)
        return;

    for(node = list->lh_Head; node->ln_Succ;) {
        nnode = node->ln_Succ;
        Remove(node);
        bsd_free(node, NULL);
        node = nnode;
    }
    bsd_free(list, NULL);
    admin_list_back();

    AROS_LIBFUNC_EXIT
}

/*
 * Not implemented: answered here, without the module.
 */
AROS_LH4(long, AddInterfaceTagList,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(STRPTR, device, A1),
         AROS_LHA(long, unit, D0),
         AROS_LHA(struct TagItem *, tags, A2),
         struct SocketBase *, libPtr, 74, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "AddInterfaceTagList() is not implemented\n");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(long, ConfigureInterfaceTagList,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct SocketBase *, libPtr, 75, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "ConfigureInterfaceTagList() is not implemented\n");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH5(LONG, CreateAddrAllocMessageA,
         AROS_LHA(LONG, version, D0),
         AROS_LHA(LONG, protocol, D1),
         AROS_LHA(STRPTR, interface_name, A0),
         AROS_LHA(struct AddressAllocationMessage *, result_ptr, A1),
         AROS_LHA(struct TagItem *, tags, A2),
         struct SocketBase *, libPtr, 79, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "CreateAddrAllocMessageA() is not implemented\n");
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, DeleteAddrAllocMessage,
         AROS_LHA(struct AddressAllocationMessage *, message, A0),
         struct SocketBase *, libPtr, 80, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "DeleteAddrAllocMessage() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, BeginInterfaceConfig,
         AROS_LHA(struct AddressAllocationMessage *, message, A0),
         struct SocketBase *, libPtr, 81, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "BeginInterfaceConfig() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, AbortInterfaceConfig,
         AROS_LHA(struct AddressAllocationMessage *, message, A0),
         struct SocketBase *, libPtr, 82, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "AbortInterfaceConfig() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH3(long, AddNetMonitorHookTagList,
         AROS_LHA(long, type, D0),
         AROS_LHA(struct Hook *, hook, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct SocketBase *, libPtr, 83, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "AddNetMonitorHookTagList() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, RemoveNetMonitorHook,
         AROS_LHA(struct Hook *, hook, A0),
         struct SocketBase *, libPtr, 84, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "RemoveNetMonitorHook() is not implemented");
    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, GetNetworkStatistics,
         AROS_LHA(LONG, type, D0),
         AROS_LHA(LONG, version, D1),
         AROS_LHA(APTR, destination, A0),
         AROS_LHA(LONG, size, D2),
         struct SocketBase *, libPtr, 85, UL)
{
    AROS_LIBFUNC_INIT
    __log(LOG_CRIT, "GetNetworkStatistics() is not implemented");
    writeErrnoValue(libPtr, ENOSYS);
    return -1;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 118: SetDefaultDomainName                                     */
/* ------------------------------------------------------------------ */
AROS_LH1(void, SetDefaultDomainName,
         AROS_LHA(STRPTR, name, A0),
         struct SocketBase *, libPtr, 118, UL)
{
    AROS_LIBFUNC_INIT
    /* Not fully implemented -- would need to rebuild domain list */
    (void)name;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 119: ObtainRoadshowData                                       */
/* ------------------------------------------------------------------ */
AROS_LH1(struct List *, ObtainRoadshowData,
         AROS_LHA(LONG, access, D0),
         struct SocketBase *, libPtr, 119, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return NULL;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 120: ReleaseRoadshowData                                      */
/* NOTE: Roadshow passes pointer in D0 (unusual)                      */
/* ------------------------------------------------------------------ */
AROS_LH1(void, ReleaseRoadshowData,
         AROS_LHA(APTR, list, D0),
         struct SocketBase *, libPtr, 120, UL)
{
    AROS_LIBFUNC_INIT
    (void)list;
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 121: ChangeRoadshowData                                       */
/* ------------------------------------------------------------------ */
AROS_LH4(LONG, ChangeRoadshowData,
         AROS_LHA(struct List *, list, A0),
         AROS_LHA(STRPTR, name, A1),
         AROS_LHA(LONG, length, D0),
         AROS_LHA(APTR, data, A2),
         struct SocketBase *, libPtr, 121, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return 0; /* FALSE */
    AROS_LIBFUNC_EXIT
}

/* ------------------------------------------------------------------ */
/* Slot 122: RemoveInterface                                          */
/* ------------------------------------------------------------------ */
AROS_LH2(LONG, RemoveInterface,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(LONG, force, D0),
         struct SocketBase *, libPtr, 122, UL)
{
    AROS_LIBFUNC_INIT
    writeErrnoValue(libPtr, ENOSYS);
    return 0; /* FALSE */
    AROS_LIBFUNC_EXIT
}

#endif /* __CONFIG_ROADSHOW__ */
