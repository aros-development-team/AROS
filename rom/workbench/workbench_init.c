/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Initialization of workbench.library.
*/

#define DEBUG 1
#include <aros/debug.h>
#include <aros/atomic.h>

#include "workbench_intern.h"
#include LC_LIBDEFS_FILE
#include "handler.h"

#ifdef SysBase
#   undef SysBase
#endif
#ifdef ExecBase
#   undef ExecBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR)(lib))->wb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR)(lib))->wb_SegList)
#define LC_RESIDENTNAME         Workbench_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI          -120
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->LibNode)

#define LC_NO_CLOSELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

#define SysBase     (WorkbenchBase->wb_SysBase)

ULONG SAVEDS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR WorkbenchBase)
{
    /* Make sure that the libraries are opened in L_OpenLib() --------------*/
    WorkbenchBase->wb_Initialized = FALSE;

    /* Initialize our private lists ----------------------------------------*/
    NEWLIST(&(WorkbenchBase->wb_AppWindows));
    NEWLIST(&(WorkbenchBase->wb_AppIcons));
    NEWLIST(&(WorkbenchBase->wb_AppMenuItems));
    NEWLIST(&(WorkbenchBase->wb_HiddenDevices));

    /* Initialize our semaphores -------------------------------------------*/
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
} /* L_InitLib */

ULONG SAVEDS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR WorkbenchBase)
{
    ObtainSemaphore(&(WorkbenchBase->wb_InitializationSemaphore));

    if (!(WorkbenchBase->wb_Initialized))
    {
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
            AROS_ATOMIC_INCW(WorkbenchBase->LibNode.lib_OpenCnt);
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

void SAVEDS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR WorkbenchBase)
{
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
} /* L_ExpungeLib */
