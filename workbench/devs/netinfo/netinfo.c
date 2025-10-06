/*
 * netinfo.c --- netinfo.device main functions
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

/****** netinfo.device/--background-- **************************************

    PURPOSE

        The netinfo.device is intended to access various information from
        network database.  It provides an uniform and well-defined interface
        to databases, so it is easy to adopt clients to different
        environments.  There is a separate unit for each database.

        Currently the netinfo.device supports access to local password and
        group files.  As a new feature since last release it supports file
        notification.

        The device file is located to "AmiTCP:devs/netinfo.device".  It is
        recommended that the OpenDevice() call is given the macro NETINFONAME
        as device name.

    OVERVIEW
        Short overview of implemented IO commands is as follows:

    CMD_RESET - reset sequential access
        This command restarts the sequential database reading.
        Inputs: none
        Results:
          * io_Error contains possaible error code or 0 if command was
            successfull

    CMD_READ - read next entry
        Inputs:
          * io_Data contains buffer for database entry. The buffer will
            be filled wtih entry data upon successfull execution.
          * io_Length contains database len
        Results:
          * io_Actual contains length of the database entry
          * io_Error contains possaible error code or 0 if command was
            successfull

    CMD_WRITE - alter the database
        This command adds a new database entry or replace an old entry.
        If a matching entry with same name is found, it is replaced with
        the new entry.

        Inputs:
          * io_Data contains pointer for database entry structure
        Results:
          * io_Actual contains length of the database entry
          * io_Error contains possible error code or 0 if command was
            successfull

    CMD_UPDATE - copy the previous changes to permanent store

        This command updates the permanent copies of database.  The
        netinfo.device does not ensure that entries updated with
        CMD_WRITE will be available by CMD_READ or written to permanent
        store before the CMD_UPDATE is executed.

        Inputs: none
        Results:
          * io_Error contains possible error code or 0 if command was
            successfull

    NI_BYNAME - search by name
        Inputs:
          * io_Data contains buffer for database entry. The buffer will
            be filled wtih entry data upon successfull execution. The name
            member of structure should be pointer to a C string containing
            the name of desired entry
          * io_Length contains database len
        Results:
          * io_Actual contains length of the database entry
          * io_Error contains possaible error code or 0 if command was
            successfull

    NI_BYUID - search by ID
        Inputs:
          * io_Data contains buffer for database entry. The buffer will
            be filled wtih entry data upon successfull execution. The ID
            member of structure should be set to desired value.
          * io_Length contains database len
        Results:
          * io_Actual contains length of the database entry
          * io_Error contains possaible error code or 0 if command was
            successfull

    NI_MEMBERS - find out memberships
        This command is implemented only for group unit.  It collects
        the IDs of groups which have a given user as member.

        Inputs:
          * io_Data is address to LONG array which is filled with group
            IDs
          * io_Length is the length of the array in bytes
          * io_Offset is a pointer to C string containing the name of
            user whose memberships are desired
        Results:
          * io_Actual contains length of the group ID array in bytes
          * io_Error contains possaible error code or 0 if command was
            successfull.

****************************************************************************
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "base.h"
#include <dos/dostags.h>

#if defined(LC_LIBDEFS_FILE)
#include LC_LIBDEFS_FILE

/*
 * Device initialization routine
 * Do only things that won't Wait()
 */
static int NetInfo__DevInit(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[NetInfo] %s()\n", __func__));

    LIBBASE->nid_Task = NULL;
    InitSemaphore(LIBBASE->nid_Lock);
    return TRUE;
}

static int NetInfo__DevOpen(LIBBASETYPEPTR LIBBASE,
    struct IORequest *req,
    ULONG unit,
    ULONG flags
)
{
    D(bug("[NetInfo] %s()\n", __func__));

    req->io_Error  = IOERR_OPENFAIL;

    /* Enforce single threading so we can Wait() */
    ObtainSemaphore(LIBBASE->nid_Lock);
    if (unit >= NETINFO_UNITS)
        goto zap;

    /*
     * Create device task if necessary
     */
    while (LIBBASE->nid_Task == NULL) {
        struct MsgPort *mp;
        struct Message *msg;
        struct Process *task;

        LIBBASE->nid_DOSBase = OpenLibrary("dos.library",37L);
        if (LIBBASE->nid_DOSBase == NULL)
            break;

        mp = CreateMsgPort();
        if (mp == NULL)
            break;
        msg = CreateIORequest(mp, sizeof(*msg));
        if (msg) {
            /* Create new process, wait for the message */
            msg->mn_Node.ln_Type = NT_MESSAGE;
            msg->mn_Node.ln_Name = (void *)LIBBASE;
            task = CreateNewProcTags(NP_Entry, NetInfoStartup,
                                     NP_Priority, NID_PRIORITY,
                                     NP_Name, MOD_NAME_STRING,
                                     NP_ExitData, msg,
                                     TAG_END);
            do {
                Wait(1L << mp->mp_SigBit);
            } while (GetMsg(mp) != msg);

            DeleteIORequest(msg);
        }
        DeleteMsgPort(mp);
        break;
    }

    if (LIBBASE->nid_Task == NULL)
        goto zap;

    D(bug("[NetInfo] %s: initializing unit...\n", __func__));

    /* OK, create a new unit structure, fill in a request and return */
    req->io_Device = (struct Device *) LIBBASE;
    if ((req->io_Unit = CreateNewUnit(LIBBASE, unit)) != NULL) {
        D(bug("[NetInfo] %s: unit @ 0x%p\n", __func__, req->io_Unit));
        req->io_Error = 0;
    }
zap:
    if (req->io_Error != 0) {
        req->io_Unit = (struct Unit *) -1;
        req->io_Device = (struct Device *) -1;
    }

    ReleaseSemaphore(LIBBASE->nid_Lock);

    D(bug("[NetInfo] %s: returning %s\n", __func__, req->io_Error ? "FALSE" : "TRUE"));

    return req->io_Error ? FALSE : TRUE;
}

static int NetInfo__DevClose(LIBBASETYPEPTR LIBBASE,
    struct IORequest *req
)
{
    int retval = FALSE;

    D(bug("[NetInfo] %s()\n", __func__));

    ObtainSemaphore(LIBBASE->nid_Lock);
    {
        ExpungeUnit(LIBBASE, req->io_Unit);
        retval = TRUE;
    }
    ReleaseSemaphore(LIBBASE->nid_Lock);

    return retval;
}

/*
 * Device expunge
 */
static int NetInfo__DevExpunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[NetInfo] %s()\n", __func__));

    if (AttemptSemaphore(LIBBASE->nid_Lock)) {
        if (LIBBASE->nid_Task) {
            if (LIBBASE->nid_Death && LIBBASE->nid_Death->mn_Node.ln_Type == NT_MESSAGE)
                ReplyMsg(LIBBASE->nid_Death);
        }
        ReleaseSemaphore(LIBBASE->nid_Lock);
    }
    return TRUE;
}

ADD2OPENDEV(NetInfo__DevOpen, 0)
ADD2CLOSEDEV(NetInfo__DevClose, 0)

ADD2INITLIB(NetInfo__DevInit, 0)
ADD2EXPUNGELIB(NetInfo__DevExpunge, 0)
#endif

/*
 * Queue requests
 */
#define IMMEDIATE_CMDS 0L
AROS_LH1(void, BeginIO,
         AROS_LHA(struct NetInfoReq *, req, A1),
         struct NetInfoDevice *, nid, 5, NetInfo)
{
    AROS_LIBFUNC_INIT

    D(bug("[NetInfo] %s()\n", __func__));

    req->io_Message.mn_Node.ln_Type = NT_MESSAGE;

    if (req->io_Command >= NI_END) {
        req->io_Error = IOERR_NOCMD;
        TermIO(req);
#if IMMEDIATE_CMDS
    } else if ((req->io_Flags & IOF_QUICK) &&
               (1L << req->io_Command) & IMMEDIATE_CMDS) {
        PerformIO(nid, req);
#endif
    } else {
        req->io_Flags &= ~IOF_QUICK;
        PutMsg(nid->nid_Port, (struct Message *)req);
    }

    AROS_LIBFUNC_EXIT
}


/*
 * The device AbortIO() entry point.
 *
 * A1 - The IO request to be aborted.
 * A6 - The device base.
 */
AROS_LH1(LONG, AbortIO,
         AROS_LHA(struct NetInfoReq *, ni, A1),
         struct NetInfoDevice *, nid, 6, NetInfo)
{
    AROS_LIBFUNC_INIT

    ULONG result = 0L;
    struct NetInfoMap *nim;

    D(bug("[NetInfo] %s()\n", __func__));
    
    if (ni->io_Unit == NULL)
        return NIERR_NULL_POINTER;

    nim = CheckUnit(nid, ni->io_Unit);
    if (nim == NULL)
        return IOERR_BADADDRESS;

    ObtainSemaphore(nim->nim_ReqLock);

    if (ni->io_Message.mn_Node.ln_Type != NT_REPLYMSG) {
        switch (ni->io_Command) {
        case CMD_READ:
            result = AbortReq(nid, nim->nim_Rx, ni);
            break;
        case CMD_WRITE:
            result = AbortReq(nid, nim->nim_Wx, ni);
            break;
        default:
            result = IOERR_NOCMD;
            break;
        }
    }

    ReleaseSemaphore(nim->nim_ReqLock);

    return(result);

    AROS_LIBFUNC_EXIT
}
