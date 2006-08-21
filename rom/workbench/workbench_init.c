/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Initialization of workbench.library.
*/

#define DEBUG 1
#include <aros/debug.h>
#include <aros/atomic.h>
#include <aros/symbolsets.h>

#include "workbench_intern.h"
#include LC_LIBDEFS_FILE
#include "handler.h"
#include "support.h"

static int WBInit(LIBBASETYPEPTR LIBBASE)
{
    /* Make sure that the libraries are opened in L_OpenLib() --------------*/
    WorkbenchBase->wb_Initialized = FALSE;

    /* Initialize our private lists ----------------------------------------*/
    NEWLIST(&(WorkbenchBase->wb_AppWindows));
    NEWLIST(&(WorkbenchBase->wb_AppIcons));
    NEWLIST(&(WorkbenchBase->wb_AppMenuItems));
    NEWLIST(&(WorkbenchBase->wb_HiddenDevices));

    /* Initialize our semaphores -------------------------------------------*/
    InitSemaphore(&(WorkbenchBase->wb_WorkbenchPortSemaphore));
    InitSemaphore(&(WorkbenchBase->wb_InitializationSemaphore));
    InitSemaphore(&(WorkbenchBase->wb_BaseSemaphore));
    
    /* Initialize handler message port -------------------------------------*/
    WorkbenchBase->wb_HandlerPort.mp_SigBit  = SIGBREAKB_CTRL_F;
    WorkbenchBase->wb_HandlerPort.mp_SigTask = NULL;      
    WorkbenchBase->wb_HandlerPort.mp_Flags   = PA_IGNORE;
    NEWLIST(&(WorkbenchBase->wb_HandlerPort.mp_MsgList));
    
    /* Initialize miscellanous variables -----------------------------------*/
    WorkbenchBase->wb_DefaultStackSize = 1024 * 32; /* 32kiB */ // FIXME: also read from preferences */
        
    return TRUE;
}

static int WBOpen(LIBBASETYPEPTR LIBBASE)
{
    ObtainSemaphore(&(WorkbenchBase->wb_InitializationSemaphore));

    if (!(WorkbenchBase->wb_Initialized))
    {
        struct CommandLineInterface *cli;
        
        /* Duplicate the search path ---------------------------------------*/
        if ((cli = Cli()) != NULL)
        {
            WorkbenchBase->wb_SearchPath = DuplicateSearchPath
            (
                cli->cli_CommandDir
            );
        }
    
        /* Start workbench handler -----------------------------------------*/
        if
        (
            (
                CreateNewProcTags
                (
                    NP_Entry,     (IPTR) WorkbenchHandler,
                    NP_StackSize,        8129,
                    NP_Name,      (IPTR) "Workbench Handler",
                    NP_UserData,  (IPTR) WorkbenchBase,
                    TAG_DONE
                )
            ) != NULL
        )
        {
            /* Prevent expunging while the handler is running */
            AROS_ATOMIC_INC(WorkbenchBase->LibNode.lib_OpenCnt);
        }
        else
        {
            // FIXME: free resources
            return FALSE;
        }
        
        WorkbenchBase->wb_Initialized = TRUE;
    }

    ReleaseSemaphore(&(WorkbenchBase->wb_InitializationSemaphore));

    return TRUE;
} /* L_OpenLib */

ADD2INITLIB(WBInit, 0);
ADD2OPENLIB(WBOpen, 0);
