/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoyfs - the Envoy filesystem exports server (EFS) as a standalone
          AROSTCP network service daemon.

          Launched by the stack's service manager (db/services.d/envoyfs) or
          by hand from a shell.  Registers with netservices.library for the
          reconfigure begin/end and stop signals (optional: without the
          library it still runs, only unsupervised), then runs the server
          body (fs_server.c) in this process.

          Discovery: the server owns the public nipc entity "Filesystem" and,
          when Envoy's ServicesManager is not running, the "Services Manager"
          entity too, answering FindService() requests itself.  The thin
          filesystem.service stub library keeps compatibility when the real
          manager owns that entity.

          NOTE: with security.library configured, client actions run under
          the mount user's identity only when this daemon's owner is root
          (fs_server.c ServerImpersonate).

          envoyfs QUIT/S
          Only one instance runs; QUIT (or Ctrl-C) stops the running one.
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/netservice.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/netservice.h>

#include "fs_intern.h"

const char version[] = "$VER: envoyfs 50.1 (10.10.2026)";

struct Library *NetServicesBase;

int main(void)
{
    struct MsgPort *port;
    struct RDArgs *rda;
    IPTR args[1] = { 0 };
    APTR handle = NULL;
    BYTE sStop = -1, sBegin = -1, sEnd = -1;
    ULONG mStop = 0, mBegin = 0, mEnd = 0;
    int rc;

    if (!(rda = ReadArgs("QUIT/S", args, NULL)))
    {
        PrintFault(IoErr(), "envoyfs");
        return RETURN_FAIL;
    }
    FreeArgs(rda);

    /* a public port marks the running daemon */
    Forbid();
    if ((port = FindPort(FS_DAEMON_PORT)))
    {
        if (args[0])
            Signal(port->mp_SigTask, SIGBREAKF_CTRL_C);
        Permit();
        PutStr(args[0] ? "envoyfs: told the running server to quit\n"
                       : "envoyfs: already running\n");
        return args[0] ? RETURN_OK : RETURN_WARN;
    }
    Permit();
    if (args[0])
    {
        PutStr("envoyfs: not running\n");
        return RETURN_WARN;
    }

    if (!(port = CreateMsgPort()))
        return RETURN_FAIL;
    port->mp_Node.ln_Name = (char *)FS_DAEMON_PORT;
    port->mp_Node.ln_Pri = 0;
    AddPort(port);

    /* Optional: supervised by the stack's service framework when present */
    if ((NetServicesBase = OpenLibrary(NETSERVICESNAME, 0)))
    {
        sStop  = AllocSignal(-1);
        sBegin = AllocSignal(-1);
        sEnd   = AllocSignal(-1);
        mStop  = (sStop  >= 0) ? (1UL << sStop)  : 0;
        mBegin = (sBegin >= 0) ? (1UL << sBegin) : 0;
        mEnd   = (sEnd   >= 0) ? (1UL << sEnd)   : 0;

        {
            struct TagItem t[] =
            {
                { NETSERVICE_Name,             (IPTR)"envoyfs" },
                { NETSERVICE_Order,            (IPTR)20 },
                { NETSERVICE_ReconfigBeginSig, (IPTR)sBegin },
                { NETSERVICE_ReconfigEndSig,   (IPTR)sEnd },
                { NETSERVICE_StopSig,          (IPTR)sStop },
                { TAG_DONE, 0 }
            };
            handle = RegisterNetService(t);
        }
        if (!handle)
        {
            /* the stack may not be up yet: run unsupervised */
            if (sStop  >= 0) { FreeSignal(sStop);  sStop  = -1; }
            if (sBegin >= 0) { FreeSignal(sBegin); sBegin = -1; }
            if (sEnd   >= 0) { FreeSignal(sEnd);   sEnd   = -1; }
            mStop = mBegin = mEnd = 0;
            CloseLibrary(NetServicesBase);
            NetServicesBase = NULL;
        }
    }

    rc = ServerMain(mStop, mBegin, mEnd);

    if (handle)
        UnregisterNetService(handle);
    if (NetServicesBase)
        CloseLibrary(NetServicesBase);
    if (sStop  >= 0) FreeSignal(sStop);
    if (sBegin >= 0) FreeSignal(sBegin);
    if (sEnd   >= 0) FreeSignal(sEnd);

    RemPort(port);
    DeleteMsgPort(port);
    return rc;
}
