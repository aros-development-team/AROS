/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
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

AROS_SET_LIBFUNC(WBInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

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
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(WBOpen, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    ObtainSemaphore(&(WorkbenchBase->wb_InitializationSemaphore));

    if (!(WorkbenchBase->wb_Initialized))
    {
        struct CommandLineInterface *cli;
        
        /* Open libraries --------------------------------------------------*/
        //FIXME: error handling! libs not closed if open fails!
        if (!(WorkbenchBase->wb_UtilityBase = OpenLibrary(UTILITYNAME, 37L)))
        {
            D(bug("Workbench: Failed to open utility.library!\n"));
            return FALSE;
        }
        
        if (!(WorkbenchBase->wb_IntuitionBase = OpenLibrary(INTUITIONNAME, 37L)))
        {
            D(bug("Workbench: Failed to open intuition.library!\n"));
            return FALSE;
        }

        if (!(WorkbenchBase->wb_DOSBase = OpenLibrary(DOSNAME, 37L)))
        {
            D(bug("Workbench: Failed to open dos.library!\n"));
            return FALSE;
        }
        
        if (!(WorkbenchBase->wb_IconBase = OpenLibrary(ICONNAME, 37L)))
        {
            D(bug("Workbench: Failed to open icon.library!\n"));
            return FALSE;
        }
        
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
    AROS_SET_LIBFUNC_EXIT
} /* L_OpenLib */

AROS_SET_LIBFUNC(WBExpunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    if ((WorkbenchBase->wb_IconBase))
    {
        CloseLibrary(WorkbenchBase->wb_IconBase);
    }
    
    if ((WorkbenchBase->wb_DOSBase))
    {
        CloseLibrary(WorkbenchBase->wb_DOSBase);
    }
    
    if ((WorkbenchBase->wb_IntuitionBase))
    {
        CloseLibrary(WorkbenchBase->wb_IntuitionBase);
    }
    
    if ((WorkbenchBase->wb_UtilityBase))
    {
        CloseLibrary(WorkbenchBase->wb_UtilityBase);
    }
    // FIXME: handler not shut down 

    return TRUE;
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(WBInit, 0);
ADD2OPENLIB(WBOpen, 0);
ADD2EXPUNGELIB(WBExpunge, 0);
