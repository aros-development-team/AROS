/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of workbench.library
    Lang: english
*/

#include "lowlevel_intern.h"
#include LC_LIBDEFS_FILE

#ifdef SysBase
#   undef SysBase
#endif
#ifdef ExecBase
#   undef ExecBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)	(((LIBBASETYPEPTR)(lib))->ll_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR)(lib))->ll_SegList)
#define LC_RESIDENTNAME		LowLevel_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		-120
#define LC_LIBBASESIZE		sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

struct ExecBase   * SysBase; /* global variable */
struct LocaleBase * LocaleBase;


ULONG SAVEDS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->ll_SysBase;

    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library",39);

    if (LocaleBase == NULL)
    {
        return FALSE;
    }

    InitSemaphore(&lh->ll_Lock);
    lh->ll_VBlank.is_Data = NULL;
    lh->ll_VBlank.is_Code = NULL;

    return TRUE;
} /* L_InitLib */


void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (LocaleBase != NULL) CloseLibrary((struct Library *) LocaleBase);
}
