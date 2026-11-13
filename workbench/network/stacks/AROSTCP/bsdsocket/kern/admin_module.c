/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The administration module, "admin" (kern/admin_ops.h): its descriptor
 * and its table. Built into the stack program while the group's switch
 * says so (conf/group-admin.cfg) or the program is monolithic, else into
 * admin.atcp. Either way it is started on first use, not with
 * the stack: by the first routing socket, the first call of one of its
 * vectors, or the panel (kern/admin_stubs.c).
 *
 * It replaces the core's module "route", which attached the routing
 * domain at every start and could not be stopped.
 *
 * WHAT KEEPS IT FROM STOPPING (refused with EBUSY, never forced):
 *  - a routing socket (counted by the registry: nm_domain);
 *  - a task inside one of its functions through the core's stubs, or a
 *    list it handed out (GetRouteInfo(), ObtainInterfaceList(),
 *    ObtainDomainNameServerList()) not yet given back: the core's slot
 *    counts both, and netdemand_set() refuses to take the table away;
 *  - the panel open, or a helper process of the panel running.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/synch.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <syslog.h>
#include <net/netmodule.h>
#include <kern/amiga_log.h>
#include <kern/amiga_gui.h>
#include <kern/admin_ops.h>

#include <net/netcore.h>            /* the core's names, as the module's table has them */
#include <net/netcore_map.h>        /* always the last include */

#ifdef NETMODULE
/*
 * A library file of its own: the libraries the code calls that the core
 * does not hand over (the tag parser of the route calls, dos.library of
 * the panel), opened by the module itself and closed when it stops. In
 * the stack program the program's own bases serve.
 */
struct UtilityBase *UtilityBase = NULL;
struct DosLibrary *DOSBase = NULL;

static int
admin_libs_open(void)
{
    if(UtilityBase == NULL)
        UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37);
    if(DOSBase == NULL)
        DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    return (UtilityBase != NULL && DOSBase != NULL);
}

static void
admin_libs_close(void)
{
    if(DOSBase != NULL)
        CloseLibrary((struct Library *)DOSBase);
    if(UtilityBase != NULL)
        CloseLibrary((struct Library *)UtilityBase);
    DOSBase = NULL;
    UtilityBase = NULL;
}
#else
#define admin_libs_open()   1
#define admin_libs_close()
#endif

static const struct admin_ops admin_ops = {
    sizeof(struct admin_ops), ADMIN_OPS_VERSION,
    rsadm_AddRouteTagList,
    rsadm_DeleteRouteTagList,
    rsadm_ChangeRouteTagList,
    rsadm_GetRouteInfo,
    rsadm_ObtainInterfaceList,
    rsadm_AddDomainNameServer,
    rsadm_RemoveDomainNameServer,
    rsadm_ObtainDomainNameServerList,
    rsadm_GetDefaultDomainName,
    rts_missmsg,
    rts_ifmsg,
    rts_newaddrmsg,
    admin_gui_open,
    admin_gui_close,
    admin_gui_process_refresh,
    admin_gui_process_msg,
};

/* the panel is open, or code of the panel runs in a process of its own */
static int
admin_gui_busy(void)
{
    return (GUI_Running || gui_timerio != NULL || MiamiPanelBase != NULL || admin_gui_procs != 0);
}

static int
admin_start(struct netmodule *nm)
{
    int error;

    if(!admin_libs_open()) {
        admin_libs_close();
        return (ENOENT);
    }
    if((error = rts_attach()) != 0) {
        admin_libs_close();
        return (error);
    }
    /* last: from here the core's stubs call in */
    (void)netdemand_set(nm->nm_name, &admin_ops);
    return (0);
}

static int
admin_can_stop(struct netmodule *nm)
{
    return (admin_gui_busy() ? EBUSY : 0);
}

static int
admin_stop(struct netmodule *nm)
{
    int error;

    /*
     * The table first: refused while a task is inside or a list is out.
     * Then the panel once more - NETTRACE may have opened it through the
     * table just before it was taken away; with the table gone it cannot
     * any more, so what is seen now stays true.
     */
    if((error = netdemand_set(nm->nm_name, NULL)) != 0)
        return (error);
    if(admin_gui_busy() || (error = rts_detach()) != 0) {
        (void)netdemand_set(nm->nm_name, &admin_ops);
        return (error != 0 ? error : EBUSY);
    }
    admin_libs_close();
    return (0);
}

/*
 * The stack is ending and the module could not be stopped - the panel is
 * open (its routing sockets are gone by now). The panel is NETTRACE's:
 * have NETTRACE close it, which it does with this module's code, and wait
 * for that while the code is still in memory; then for the helper
 * processes. NETTRACE does not wait for this task, so the wait ends; it is
 * bounded all the same (5 s each).
 */
static void
admin_shutdown(struct netmodule *nm)
{
    struct SysLogPacket *msg;
    int i;

    if(gui_timerio != NULL && (msg = GetLogMsg(&logReplyPort)) != NULL) {
        msg->Level = LOG_GUIMSG | GUICMD_CLOSE;
        PutMsg(logPort, (struct Message *)msg);
        for(i = 0; i < 250 && gui_timerio != NULL; i++)
            Delay(1);
    }
    for(i = 0; i < 250 && admin_gui_procs != 0; i++)
        Delay(1);
    if(gui_timerio != NULL || admin_gui_procs != 0)
        log(LOG_ERR, "module %s: the control panel did not close at shutdown.", nm->nm_name);
    admin_libs_close();
}

NETMODULE_DESCRIPTOR(admin_module) = {
    sizeof(struct netmodule), NETMODULE_VERSION, "admin", 0,
    admin_start, admin_stop, admin_can_stop,
    0, NULL,
    &routedomain,           /* its routing sockets count as users */
    NULL,                   /* needs no other module */
    NULL, NULL,
    admin_shutdown
};
NETMODULE_BUILTIN(admin_module, 90)
