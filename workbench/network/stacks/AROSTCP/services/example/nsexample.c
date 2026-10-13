/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Example external AROSTCP service daemon - the template for real services.
 *
 * It demonstrates the full contract:
 *   - obtain its OWN bsdsocket.library base on launch (and release it on stop),
 *   - register with netservices.library, asking for reconfigure-begin/end and
 *     stop signals,
 *   - react to those signals in its own task (quiesce / resume / exit),
 *   - unregister and close its bases cleanly before exiting, so the stack's
 *     open-count drops and a shutdown/restart completes.
 *
 * The service manager (db/services) launches it; it writes a small log so a
 * test can see what happened.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/netservice.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/netservice.h>

struct Library *NetServicesBase;
struct Library *SocketBase;     /* our OWN bsdsocket base */

static void logln(CONST_STRPTR s)
{
    BPTR f = Open("SYS:nssvc.log", MODE_READWRITE);
    if(f) {
        Seek(f, 0, OFFSET_END);
        FPuts(f, (STRPTR)s);
        FPuts(f, (STRPTR)"\n");
        Close(f);
    }
}

int main(void)
{
    APTR handle;
    BYTE sStop, sBegin, sEnd;
    ULONG mStop, mBegin, mEnd, got;
    int running = 1;

    NetServicesBase = OpenLibrary(NETSERVICESNAME, 0);
    if(!NetServicesBase) {
        logln("example: cannot open netservices.library");
        return 20;
    }

    /* Obtain our own stack binding. */
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if(!SocketBase) {
        logln("example: cannot open bsdsocket.library");
        CloseLibrary(NetServicesBase);
        return 20;
    }

    sStop  = AllocSignal(-1);
    sBegin = AllocSignal(-1);
    sEnd   = AllocSignal(-1);
    mStop  = (sStop  >= 0) ? (1UL << sStop)  : 0;
    mBegin = (sBegin >= 0) ? (1UL << sBegin) : 0;
    mEnd   = (sEnd   >= 0) ? (1UL << sEnd)   : 0;

    {
        struct TagItem t[] = {
            { NETSERVICE_Name,             (IPTR)"example" },
            { NETSERVICE_Order,            (IPTR)10 },
            { NETSERVICE_ReconfigBeginSig, (IPTR)sBegin },
            { NETSERVICE_ReconfigEndSig,   (IPTR)sEnd },
            { NETSERVICE_StopSig,          (IPTR)sStop },
            { TAG_DONE, 0 }
        };
        handle = RegisterNetService(t);
    }
    if(!handle) {
        logln("example: RegisterNetService failed");
        CloseLibrary(SocketBase);
        CloseLibrary(NetServicesBase);
        return 20;
    }

    logln("example: started, registered, bsdsocket base held");

    while(running) {
        got = Wait(mStop | mBegin | mEnd | SIGBREAKF_CTRL_C);
        if(got & mBegin)
            logln("example: reconfigure-begin (dropping stack bindings)");
        if(got & mEnd)
            logln("example: reconfigure-end (reacquiring)");
        if(got & (mStop | SIGBREAKF_CTRL_C)) {
            logln("example: stop received");
            running = 0;
        }
    }

    /* Clean release: unregister, drop our stack base, close netservices. */
    UnregisterNetService(handle);
    CloseLibrary(SocketBase);
    CloseLibrary(NetServicesBase);
    if(sStop  >= 0) FreeSignal(sStop);
    if(sBegin >= 0) FreeSignal(sBegin);
    if(sEnd   >= 0) FreeSignal(sEnd);
    logln("example: unregistered, base released, exiting");
    return 0;
}
