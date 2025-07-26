/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/log.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <resources/log.h>

#include "log_intern.h"

#include LC_LIBDEFS_FILE

#define DOSBase LIBBASE->lrb_DosBase
#define TimerBase LIBBASE->lrb_TimerIOReq.tr_node.io_Device


/*****************************************************************************************

    NAME
        --background_logresource--

    LOCATION
        log.resource

    NOTES
        This resource serves as a base interface to AROS's logging mechanisms.

        It provides centralized collection and management of structured log messages
        from system and application components, enabling uniform diagnostics and
        analysis capabilities across the OS.

        Messages are stored as dynamically allocated nodes
        which are organized into two linked lists:
            - A global list maintained by the log.resource itself
            - A per-producer list

        Entries are added using `logAddEntryA()` which supports a printf-style format string
        and metadata such as component, subcomponent, originator, and event ID.

        Each entry stores:
            - A human-readable formatted message (`le_Entry`)
            - Producer and originator info (`lep_Producer`, `lectx_Originator`)
            - Component tags (`le_Node.ln_Name`, `lep_Node.ln_Name`)
            - Time/date stamp (`le_DateStamp`)
            - Priority flags and event identifiers

        Consumers may iterate over entries using `logNextEntry()` with an `entryHandle` pointer,
        which is updated in place to walk through the list. Constants `LOGEntry_First` and
        `LOGEntry_Last` mark the bounds of traversal.

        Attributes of each entry can be queried with `logGetEntryAttrs()`, which takes a
        taglist and returns selected fields such as LOGMA_Entry, LOGMA_Flags, LOGMA_Component,
        and others.

        Entries may be removed via `logRemEntry()`, which safely unlinks and deallocates
        all associated resources, including copied strings.

        `logLockEntries()` and `logUnlockEntries()` provide concurrency protection for
        iteration or filtering.

        The log.resource is self-contained and allocates memory via the producer's
        memory pool (`lrh_Pool`) to simplify cleanup and ownership tracking.

*****************************************************************************************/

BOOL GM_UNIQUENAME(OpenTimer)(LIBBASETYPEPTR LIBBASE)
{
    if (TimerBase)
        return TRUE;
    else
    {
        LIBBASE->lrb_TimerIOReq.tr_node.io_Message.mn_Node.ln_Type  = NT_REPLYMSG;
        LIBBASE->lrb_TimerIOReq.tr_node.io_Message.mn_Length        = sizeof(struct timerequest);
        LIBBASE->lrb_TimerIOReq.tr_node.io_Message.mn_ReplyPort     = NULL;
        if(!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) &LIBBASE->lrb_TimerIOReq, 0))
        {
            LIBBASE->lrb_TimerIOReq.tr_node.io_Message.mn_Node.ln_Name = "log.resource";
            LIBBASE->lrb_TimerIOReq.tr_node.io_Command = TR_ADDREQUEST;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL GM_UNIQUENAME(OpenDOS)(LIBBASETYPEPTR LIBBASE)
{
    if (DOSBase)
        return TRUE;
    else if ((DOSBase = OpenLibrary("dos.library", 39)))
        return TRUE;
    return FALSE;
}

BOOL GM_UNIQUENAME(OpenUtility)(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->lrb_UtilityBase)
        return TRUE;
    else if ((LIBBASE->lrb_UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 39)))
        return TRUE;
    return FALSE;
}

#define UtilityBase LIBBASE->lrb_UtilityBase

AROS_LH1(APTR, logInitialise,
         AROS_LHA(APTR, logProvider, A0),
         LIBBASETYPEPTR, LIBBASE, 1, log)
{
    AROS_LIBFUNC_INIT

    APTR logPool;

    if (LIBBASE->lrb_Task)
    {
        if((logProvider) && (logPool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC, 16384, 1024)))
        {
            struct LogResHandle *lpHandle = AllocVecPooled(logPool, sizeof(struct LogResHandle));
            lpHandle->lrh_Node.ln_Type = NT_PROVIDER;
            lpHandle->lrh_Node.ln_Pred = NULL;
            lpHandle->lrh_Node.ln_Succ = NULL;
            lpHandle->lrh_Pool = logPool;
            lpHandle->lrh_Node.ln_Name = logCopyStr(((struct Node *)logProvider)->ln_Name);
            NewList(&lpHandle->lrh_Entries);
            AddTail(&LIBBASE->lrb_Providers, &lpHandle->lrh_Node);

            return lpHandle;
        }
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, logFinalise,
         AROS_LHA(APTR, logProvider, A0),
         LIBBASETYPEPTR, LIBBASE, 2, log)
{
    AROS_LIBFUNC_INIT

    struct LogResHandle *lpHandle, *tmp;
    ForeachNodeSafe(&LIBBASE->lrb_Providers, lpHandle, tmp)
    {
        if (lpHandle->lrh_Node.ln_Name && !logCmpStr(lpHandle->lrh_Node.ln_Name, ((struct Node *)logProvider)->ln_Name))
        {
            // TODO:
        }
    }

    AROS_LIBFUNC_EXIT
}

static AROS_INTH1(log_LowMemHandler, LIBBASETYPEPTR, LIBBASE)
{
    AROS_INTFUNC_INIT

    return MEM_DID_NOTHING;

    AROS_INTFUNC_EXIT
}

static void log_Task(LIBBASETYPEPTR LIBBASE)
{
    struct Task *thisTask = FindTask(NULL);

    if (!LIBBASE->lrb_Task)
    {
        struct logEntryPrivate *lastBroadcast = NULL;

        LIBBASE->lrb_sigDIE = AllocSignal(-1);
        if (LIBBASE->lrb_sigDIE == -1)
            return;

        /* we are now ready ..*/
        LIBBASE->lrb_Task = thisTask;

        ULONG sigmask = (1 << LIBBASE->lrb_sigDIE) | (1 << LIBBASE->lrb_ServicePort->mp_SigBit);

        logAddEntry(LOGF_Flag_Type_Information, &LIBBASE->lrb_LRProvider, "", __func__, 0,
                            "AROS System Logger v%u.%u\nLogging started\nListening on port 0x%p", MAJOR_VERSION, MINOR_VERSION, LIBBASE->lrb_ServicePort);

        while (LIBBASE->lrb_Task != NULL)
        {
            ULONG signals = Wait(sigmask);
            {
                if (signals & (1 << LIBBASE->lrb_sigDIE))
                {
                    logAddEntry(LOGF_Flag_Type_Information, &LIBBASE->lrb_LRProvider, "", __func__, 0,
                            "Support task ending\n");
                    LIBBASE->lrb_Task = NULL;
                }
                if (signals & (1 << LIBBASE->lrb_ServicePort->mp_SigBit))
                {
                    while ((lastBroadcast = (struct logEntryPrivate *)GetMsg(LIBBASE->lrb_ServicePort)) != NULL)
                    if (lastBroadcast->le_Node.ln_Type == EHMB_ADDENTRY)
                    {
                        logLockEntries(LLF_WRITE);
                        AddTail(&LIBBASE->lrb_LRProvider.lrh_Entries, &lastBroadcast->lep_Node);
                        lastBroadcast->le_Node.ln_Type = NT_LOGENTRY;
                        logEventBroadcast(EHMB_ADDENTRY, &lastBroadcast->le_Node, NULL);
                        logUnlockEntries(LLF_WRITE);
                    }
                    else if (lastBroadcast->le_Node.ln_Type == EHMB_REMENTRY)
                    {
                        logEventBroadcast(EHMB_REMENTRY, &lastBroadcast->le_Node, NULL);
                        struct LogResHandle *lrHandle = lastBroadcast->lep_Producer;
                        FreeVec(lastBroadcast->le_Node.ln_Name);
                        FreeVec(lastBroadcast->lectx_Originator);
                        FreeVec(lastBroadcast->le_Entry);
                        FreeVecPooled(lrHandle->lrh_Pool, lastBroadcast);
                    }
                    else
                    {
                        logAddEntry(LOGF_Flag_Type_Warn, &LIBBASE->lrb_LRProvider, "", __func__, 0,
                            "Unhandled event type, received\n");
                    }
                }
            }
        }
        FreeSignal(LIBBASE->lrb_sigDIE);
    }
    else
    {
        logAddEntry(LOGF_Flag_Type_Information, &LIBBASE->lrb_LRProvider, "", __func__, 0,
                            "Broadcaster already running\n");
    }
}

/* LibInit */
static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->lrb_LRProvider.lrh_Node.ln_Name = LIBBASE->lrb_Lib.lib_Node.ln_Name;
    NewList(&LIBBASE->lrb_LRProvider.lrh_Entries);
    NewList(&LIBBASE->lrb_Providers);
    AddTail(&LIBBASE->lrb_Providers, &LIBBASE->lrb_LRProvider.lrh_Node);
    NewList(&LIBBASE->lrb_Listeners);

    InitSemaphore(&LIBBASE->lrb_ListenerLock);
    InitSemaphore(&LIBBASE->lrb_ReentrantLock);

    if((LIBBASE->lrb_LRProvider.lrh_Pool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC, 1024, 1024)))
    {
        LIBBASE->lrb_LowMemHandler.is_Node.ln_Name = LIBBASE->lrb_Lib.lib_Node.ln_Name;
        LIBBASE->lrb_LowMemHandler.is_Code         = (VOID_FUNC)log_LowMemHandler;
        LIBBASE->lrb_LowMemHandler.is_Data         = LIBBASE;
        AddMemHandler(&LIBBASE->lrb_LowMemHandler);

        LIBBASE->lrb_Task = NewCreateTask(TASKTAG_NAME, "Log Event Broadcaster",
                    TASKTAG_PC, log_Task,
                    TASKTAG_PRI, 21,
                    TASKTAG_TASKMSGPORT, &LIBBASE->lrb_ServicePort,
                    TASKTAG_ARG1, LIBBASE,
                    TAG_END);

        return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
