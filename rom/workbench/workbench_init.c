/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Initialization of workbench.library.
*/

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

        WorkbenchBase->wb_LibsOpened = TRUE;
    }

    ReleaseSemaphore(&(WorkbenchBase->wb_InitializationSemaphore));

    return TRUE;
} /* L_OpenLib */

void SAVEDS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR WorkbenchBase)
{
    if ((WorkbenchBase->wb_UtilityBase))
    {
        CloseLibrary(WorkbenchBase->wb_UtilityBase);
    }

    if ((WorkbenchBase->wb_IntuitionBase))
    {
        CloseLibrary(WorkbenchBase->wb_IntuitionBase);
    }

    if ((WorkbenchBase->wb_DOSBase))
    {
        CloseLibrary(WorkbenchBase->wb_DOSBase);
    }
} /* L_ExpungeLib */
