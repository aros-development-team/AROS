/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
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
        // TODO:
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
    APTR lognode;

    if (!LIBBASE->lrb_Task)
        LIBBASE->lrb_Task = thisTask;

    SetSignal(0, SIGF_SINGLE);

    logAddEntry(LOGF_Flag_Type_Information, &LIBBASE->lrb_LRProvider, "", __func__, 0,
                        "AROS System Logger v%u.%u\nLogging started", MAJOR_VERSION, MINOR_VERSION);
    for (;;)
    {
        Wait(SIGF_SINGLE);
        {
            // TODO
            // logEventBroadcast(EHMB_REMENTRY, le, NULL);
        }
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

    InitSemaphore(&LIBBASE->lrb_ReentrantLock);

    if((LIBBASE->lrb_LRProvider.lrh_Pool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC, 16384, 1024)))
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
