/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Init of workbench.library
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include "workbench_intern.h"
#include "libdefs.h"
#include "handler.h"

#ifdef SysBase
#   undef SysBase
#endif
#ifdef ExecBase
#   undef ExecBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((struct LIBBASETYPEPTR)(lib))->wb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((struct LIBBASETYPEPTR)(lib))->wb_SegList)
#define LC_RESIDENTNAME         Workbench_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI          -120
#define LC_LIBBASESIZE          sizeof(struct LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     struct LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct LIBBASETYPEPTR)(lib))->LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

ULONG SAVEDS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR WorkbenchBase) {
    kprintf( "Workbench: InitLib() called.\n" );
    if( !(WorkbenchBase->wb_UtilityBase = OpenLibrary( UTILITYNAME, 37L )) ) {
        kprintf( "Workbench: Failed opening utility.library!\n" );
        return FALSE;
    }

    if( !(WorkbenchBase->wb_DOSBase = OpenLibrary( DOSNAME, 37L )) ) {
        kprintf( "Workbench: Failed opening utility.library!\n" );
        return FALSE;
    }

    /* Initialize our private lists. */
    NEWLIST( &(WorkbenchBase->wb_AppWindows) );
    NEWLIST( &(WorkbenchBase->wb_AppIcons) );
    NEWLIST( &(WorkbenchBase->wb_AppMenuItems) );
    NEWLIST( &(WorkbenchBase->wb_Listeners) );

    return TRUE;
} /* L_InitLib */

void SAVEDS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR WorkbenchBase) {
    if( (WorkbenchBase->wb_UtilityBase) ) {
        CloseLibrary( WorkbenchBase->wb_UtilityBase );
    }

    if( (WorkbenchBase->wb_DOSBase) ) {
        CloseLibrary( WorkbenchBase->wb_DOSBase );
    }
}