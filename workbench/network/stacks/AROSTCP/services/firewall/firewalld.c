/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * firewalld - firewall lifecycle manager as an external AROSTCP service.
 *
 * The packet-filter ENGINE stays inside the stack (bsdsocket net/ipfilter.c,
 * controlled through the 'r'-group IoctlSocket codes); this daemon only
 * manages its lifecycle, reusing the existing rule parsers (C:ipf, C:ipnat)
 * instead of duplicating them:
 *
 *   - on start and on every stack reconfigure-end it applies the rules:
 *       ipf -Fa -f ENV:AROSTCP/ipf.rules -E     (or ipf -DFa with no file)
 *       ipnat -CF -f ENV:AROSTCP/ipnat.rules    (or ipnat -CF with no file)
 *     so the saved policy actually comes up with the stack (nothing called
 *     S:Start-Firewall before),
 *   - it watches both rules files and re-applies on change (the Firewall
 *     prefs editor's own shell-out becomes redundant but stays harmless),
 *   - reconfigure-begin is a no-op (no stack calls during the fence; a
 *     change notification arriving inside the fence is applied at the end),
 *   - on stop it exits WITHOUT touching the rules: a dying firewall manager
 *     must not open the firewall.
 *
 * Managed by the stack's service manager via db/services.d/firewall
 * (disabled by default; configure rules with SYS:Prefs/Firewall first).
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/netservice.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/netservice.h>

#include <string.h>

const char version[] = "$VER: firewalld 1.0 (10.10.2026)";

#define FWD_PORT        "AROSTCP Firewall Daemon"
#define FWD_IPF_RULES   "ENV:AROSTCP/ipf.rules"
#define FWD_IPNAT_RULES "ENV:AROSTCP/ipnat.rules"

struct Library *NetServicesBase;

static BOOL file_exists(CONST_STRPTR path)
{
    BPTR lock = Lock(path, SHARED_LOCK);

    if (lock == BNULL)
        return FALSE;
    UnLock(lock);
    return TRUE;
}

static void run(CONST_STRPTR cmd)
{
    SystemTags(cmd,
               SYS_Input, BNULL,
               SYS_Output, BNULL,
               TAG_DONE);
}

/* Bring the kernel filter in line with the rules files: clear old state,
 * load the saved policy, enable - or disable and flush when the policy was
 * removed.  Mirrors S:Start-Firewall, plus the actual enable. */
static void apply_rules(void)
{
    if (file_exists(FWD_IPF_RULES))
        run("C:ipf -Fa -f " FWD_IPF_RULES " -E");
    else
        run("C:ipf -DFa");

    if (file_exists(FWD_IPNAT_RULES))
        run("C:ipnat -CF -f " FWD_IPNAT_RULES);
    else
        run("C:ipnat -CF");
}

int main(void)
{
    struct MsgPort *port;
    struct NotifyRequest nr_ipf, nr_ipnat;
    BOOL watch_ipf = FALSE, watch_ipnat = FALSE;
    APTR handle = NULL;
    BYTE sStop = -1, sBegin = -1, sEnd = -1, sNotify = -1;
    ULONG mStop = 0, mBegin = 0, mEnd = 0, mNotify = 0;
    BOOL paused = FALSE, pending = FALSE;
    int running = 1;

    /* one instance only */
    Forbid();
    if (FindPort(FWD_PORT) != NULL)
    {
        Permit();
        PutStr("firewalld: already running\n");
        return RETURN_WARN;
    }
    Permit();
    if (!(port = CreateMsgPort()))
        return RETURN_FAIL;
    port->mp_Node.ln_Name = (char *)FWD_PORT;
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
            struct TagItem t[] = {
                { NETSERVICE_Name,             (IPTR)"firewall" },
                { NETSERVICE_Order,            (IPTR)90 },
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

    /* re-apply whenever the prefs editor (or anything else) rewrites a
     * rules file */
    if ((sNotify = AllocSignal(-1)) >= 0)
    {
        mNotify = 1UL << sNotify;

        memset(&nr_ipf, 0, sizeof(nr_ipf));
        nr_ipf.nr_Name = (STRPTR)FWD_IPF_RULES;
        nr_ipf.nr_Flags = NRF_SEND_SIGNAL;
        nr_ipf.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
        nr_ipf.nr_stuff.nr_Signal.nr_SignalNum = sNotify;
        watch_ipf = StartNotify(&nr_ipf) ? TRUE : FALSE;

        memset(&nr_ipnat, 0, sizeof(nr_ipnat));
        nr_ipnat.nr_Name = (STRPTR)FWD_IPNAT_RULES;
        nr_ipnat.nr_Flags = NRF_SEND_SIGNAL;
        nr_ipnat.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
        nr_ipnat.nr_stuff.nr_Signal.nr_SignalNum = sNotify;
        watch_ipnat = StartNotify(&nr_ipnat) ? TRUE : FALSE;
    }

    apply_rules();

    while (running)
    {
        ULONG got = Wait(mStop | mBegin | mEnd | mNotify | SIGBREAKF_CTRL_C);

        if (got & (mStop | SIGBREAKF_CTRL_C))
        {
            /* exit WITHOUT touching the kernel rules */
            running = 0;
            continue;
        }
        if (got & mBegin)
            paused = TRUE;              /* no stack calls during the fence */
        if (got & mNotify)
        {
            if (paused)
                pending = TRUE;
            else
                apply_rules();
        }
        if (got & mEnd)
        {
            paused = FALSE;
            pending = FALSE;
            apply_rules();              /* fresh interfaces: re-apply */
        }
        else if (pending && !paused)
        {
            pending = FALSE;
            apply_rules();
        }
    }

    if (watch_ipnat)
        EndNotify(&nr_ipnat);
    if (watch_ipf)
        EndNotify(&nr_ipf);
    if (sNotify >= 0)
        FreeSignal(sNotify);
    if (handle)
        UnregisterNetService(handle);
    if (NetServicesBase)
        CloseLibrary(NetServicesBase);
    if (sStop  >= 0) FreeSignal(sStop);
    if (sBegin >= 0) FreeSignal(sBegin);
    if (sEnd   >= 0) FreeSignal(sEnd);
    RemPort(port);
    DeleteMsgPort(port);
    return RETURN_OK;
}
