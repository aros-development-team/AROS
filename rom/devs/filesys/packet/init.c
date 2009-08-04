/*
 * packet.handler - Proxy filesystem for DOS packet handlers
 *
 * Copyright © 2007-2009 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include "packet.h"

/* places we might find handlers */
const char *search_path[] = {
    "%s",
    "DEVS:%s",
    "DEVS:Filesystems/%s",
    "DEVS:Handlers/%s",
    NULL
};

static int GM_UNIQUENAME(init)(struct PacketBase *pb) {
    D(bug("[packet] in init\n"));

    NEWLIST(&(pb->mounts));

    return TRUE;
}

static int GM_UNIQUENAME(expunge)(struct PacketBase *pb) {
    D(bug("[packet] in expunge\n"));

    return TRUE;
}

AROS_UFH3(void, packet_startup,
          AROS_UFHA(STRPTR,            argPtr,  A0),
          AROS_UFHA(ULONG,             argSize, D0),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    struct Process *me = (struct Process *) FindTask(NULL);
    BPTR seglist = (BPTR) me->pr_Task.tc_UserData;
    struct Message *message;
    struct DosPacket *packet;
    struct MsgPort *port;

    D(bug("[packet] in packet_startup\n"));

    /* wait for startup message before running handler. We can't wait for it
       after the handler returns because we won't know whether the handler
       has swallowed it or if it's just slow coming from the init code below */
    WaitPort(&(me->pr_MsgPort));

    AROS_UFC1(void, (LONG_FUNC) ((BPTR *) BADDR(seglist) + 1),
              AROS_UFCA(struct ExecBase *, SysBase, A6));

    D(bug("[packet] handler returned\n"));

    /* if we get here, the handler probably wasn't packet-based. We reply
       to the startup message ourselves if necessary */
    message = GetMsg(&(me->pr_MsgPort));
    if (message != NULL)
    {
        packet = (struct DosPacket *)((struct Node *)message)->ln_Name;

        port = packet->dp_Port;
        message = packet->dp_Link;

        packet->dp_Port = &me->pr_MsgPort;
        packet->dp_Res1 = DOSFALSE;
        packet->dp_Res2 = IOERR_OPENFAIL;

        PutMsg(port, message);
    }

    AROS_USERFUNC_EXIT
}

void packet_reply(struct ph_mount *, APTR, struct ExecBase *);

static int GM_UNIQUENAME(open)(struct PacketBase *pb, struct IOFileSys *iofs, ULONG unitnum, ULONG flags) {
    struct ph_mount *scan, *mount;
    struct DeviceNode *dn;
    char filename[MAXFILENAMELENGTH];
    int i, n;
    BPTR seglist;
    BOOL loaded = FALSE;
    char pr_name[256];
    struct MsgPort *reply_port;
    TEXT *dos_path;
    struct DosPacket *dp;

    D(bug("[packet] in open by Task %s\n", FindTask(NULL)->tc_Node.ln_Name));

    dn = iofs->io_Union.io_OpenDevice.io_DeviceNode;

    D(bug("[packet] devicenode=%08lx devicename '%s' unit %d dosname '%s' handler '%s'\n",
            dn,
            iofs->io_Union.io_OpenDevice.io_DeviceName,
            iofs->io_Union.io_OpenDevice.io_Unit,
            iofs->io_Union.io_OpenDevice.io_DosName,
            AROS_BSTR_ADDR(dn->dn_Handler)));

    /* find this mount */
    mount = NULL;
    ForeachNode(&(pb->mounts), scan) {
        /* XXX what happens if the mount point matches but the handler doesn't? */
        if (strcmp(scan->handler_name, AROS_BSTR_ADDR(dn->dn_Handler)) == 0 &&
            strcmp(scan->mount_point, iofs->io_Union.io_OpenDevice.io_DosName) == 0) {
            mount = scan;
            break;
        }
    }

    /* if we didn't find it then we have to set it up */
    if (mount == NULL) {

        /* try to load the named handler from each dir in the search path if
         * not already loaded */
        seglist = dn->dn_SegList;
        for (i = 0; seglist == NULL && search_path[i] != NULL; i++) {
            snprintf(filename, MAXFILENAMELENGTH, search_path[i], AROS_BSTR_ADDR(dn->dn_Handler));
            seglist = LoadSeg(filename);
            if (seglist != NULL) {
                loaded = TRUE;
                D(bug("[packet] loaded %s\n", filename));
            }
            else
                D(bug("[packet] couldn't load %s\n", filename));
        }

        if (seglist == NULL) {
            kprintf("[packet] couldn't open %s\n", AROS_BSTR_ADDR(dn->dn_Handler));
            iofs->IOFS.io_Error = IOERR_OPENFAIL;
            return FALSE;
        }

        /* got it, create our mount struct */
        mount = (struct ph_mount *) AllocVec(sizeof(struct ph_mount), MEMF_PUBLIC | MEMF_CLEAR);

        strncpy(mount->handler_name, AROS_BSTR_ADDR(dn->dn_Handler), MAXFILENAMELENGTH);
        strncpy(mount->mount_point, iofs->io_Union.io_OpenDevice.io_DosName, MAXFILENAMELENGTH);

        /* only store seg list for later unloading if we loaded it ourselves */
        if (loaded)
            mount->seglist = seglist;

        D(bug("[packet] starting handler process\n"));

        /* start it up */
        snprintf(pr_name, sizeof(pr_name), "filesys process for %s", mount->mount_point);
        mount->process = CreateNewProcTags(NP_Entry,     (IPTR) packet_startup,
                                           NP_Name,      pr_name,
                                           NP_StackSize, dn->dn_StackSize,
                                           NP_Priority,  dn->dn_Priority,
                                           NP_UserData,  (IPTR) seglist,
                                           TAG_DONE);

        /* something went horribly wrong? */
        if (mount->process == NULL) {
            D(kprintf("[packet] couldn't start filesys process for %s\n", mount->mount_point));

            if (loaded)
                UnLoadSeg(seglist);
            FreeVec(mount);

            iofs->IOFS.io_Error = IOERR_OPENFAIL;
            return FALSE;
        }

        D(bug("[packet] started, process structure is 0x%08x\n", mount->process));
        reply_port = CreateMsgPort();

        /* build the startup packet */
        dp = (struct DosPacket *) AllocDosObject(DOS_STDPKT, NULL);
        n = strlen(dn->dn_Name);
        dos_path = AllocVec(n + 3, MEMF_PUBLIC);
        sprintf(dos_path + 1, "%s:", dn->dn_Name);
        dos_path[0] = n + 1;
        dp->dp_Arg1 = (SIPTR)MKBADDR(dos_path);
        dp->dp_Arg2 = (SIPTR)MKBADDR(dn->dn_Startup);
        dp->dp_Arg3 = (SIPTR)MKBADDR(dn);
        dp->dp_Port = reply_port;

        /* A handler can add volumes during startup, so we have to be fully functional before it
           replies the startup packet */

        /* setup a handler function and port for replies */
        mount->reply_int.is_Code = packet_reply;
        mount->reply_int.is_Data = mount;

        NEWLIST(&(mount->reply_port.mp_MsgList));
        mount->reply_port.mp_Flags = PA_SOFTINT;
        mount->reply_port.mp_SigTask = &(mount->reply_int);

        /* setup the root "handle" */
        mount->root_handle.mount = mount;
        mount->root_handle.is_lock = TRUE;

        /* set up device and unit in device node */
        dn->dn_Ext.dn_AROS.dn_Device = iofs->IOFS.io_Device;
        dn->dn_Ext.dn_AROS.dn_Unit = (struct Unit *) &(mount->root_handle);
        D(bug("[packet] sending startup packet\n"));

        PutMsg(&(mount->process->pr_MsgPort), dp->dp_Link);
        WaitPort(reply_port);
        GetMsg(reply_port);

        DeleteMsgPort(reply_port);

        if (dp->dp_Res1 == DOSFALSE) {
            D(bug("[packet] handler failed startup [%d]\n", dp->dp_Res2));

            iofs->IOFS.io_Error = dp->dp_Res2;
            dn->dn_Ext.dn_AROS.dn_Device = NULL;
            dn->dn_Ext.dn_AROS.dn_Unit = NULL;
        }
        else {
            /* hook the process up to the device node */
            dn->dn_Task = &(mount->process->pr_MsgPort);


            /* remember this mount */
            Disable();
            AddTail((struct List *) &(pb->mounts), (struct Node *) mount);
            Enable();

            iofs->IOFS.io_Unit = (struct Unit *) &(mount->root_handle);
            iofs->IOFS.io_Error = 0;

            D(bug("[packet] handler %s for mount %s now online\n", mount->handler_name, mount->mount_point));
        }

        FreeDosObject(DOS_STDPKT, dp);
    }

    return iofs->IOFS.io_Error != 0 ? FALSE : TRUE;
}

static int GM_UNIQUENAME(close)(struct PacketBase *pb, struct IOFileSys *iofs) {
    D(bug("[packet] in close\n"));

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(expunge),0)
ADD2OPENDEV(GM_UNIQUENAME(open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(close),0)

void packet_handle_request(struct IOFileSys *, struct PacketBase *);

AROS_LH1(void, beginio, AROS_LHA(struct IOFileSys *, iofs, A1), struct PacketBase *, pb, 5, Packet) {
    AROS_LIBFUNC_INIT

    D(bug("[packet] in begin_io\n"));

    packet_handle_request(iofs, pb);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, abortio, AROS_LHA(struct IOFileSys *, iofs, A1), struct PacketBase *, pb, 6, Packet) {
    AROS_LIBFUNC_INIT

    D(bug("[packet] in abort_io\n"));

    return 0;

    AROS_LIBFUNC_EXIT
}
