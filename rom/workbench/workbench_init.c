/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Initialization of workbench.library.
*/

#define DEBUG 1
#include <aros/debug.h>

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
    /* Make sure that the libraries are opened in L_OpenLib() */
    WorkbenchBase->wb_LibsOpened = FALSE;

    /* Initialize our private lists. */
    NEWLIST(&(WorkbenchBase->wb_AppWindows));
    NEWLIST(&(WorkbenchBase->wb_AppIcons));
    NEWLIST(&(WorkbenchBase->wb_AppMenuItems));
    NEWLIST(&(WorkbenchBase->wb_HiddenDevices));

    /* Initialize our semaphore. */
    InitSemaphore(&(WorkbenchBase->wb_InitializationSemaphore));
    InitSemaphore(&(WorkbenchBase->wb_BaseSemaphore));
    InitSemaphore(&(WorkbenchBase->wb_HandlerSemaphore));
    
    WorkbenchBase->wb_HandlerPort  = NULL;
    WorkbenchBase->wb_HandlerError = 0;

    WorkbenchBase->wb_DefaultStackSize = 1024 * 32; /* 32kB */ // FIXME: also read from preferences */
    
    return TRUE;
} /* L_InitLib */

ULONG SAVEDS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR WorkbenchBase)
{
    ObtainSemaphore(&(WorkbenchBase->wb_InitializationSemaphore));

    if (!(WorkbenchBase->wb_LibsOpened))
    {
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
        
        WorkbenchBase->wb_LibsOpened = TRUE;
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
} /* L_ExpungeLib */
