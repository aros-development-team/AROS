/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RexxSupport initialization code.
    Lang: English
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/alib.h>

#include "rexxsupport_intern.h"
#include LC_LIBDEFS_FILE

#undef SysBase

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

#define LC_LIBHEADERTYPEPTR        LIBBASETYPEPTR
#define LC_LIB_FIELD(libBase)      (libBase)->library.lh_LibNode
#define LC_SYSBASE_FIELD(libBase)  (libBase)->library.lh_SysBase
#define LC_SEGLIST_FIELD(libBase)  (libBase)->library.lh_SegList
#define LC_LIBBASESIZE             (sizeof(LIBBASETYPE))

#include <libcore/libheader.c>

#define SysBase LC_SYSBASE_FIELD(lh)

struct RxsLib *RexxSysBase;
struct DosLibrary *DOSBase;
int errno;

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    NewList(&RSBI(lh)->openports);
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    if (DOSBase == NULL)
	return FALSE;
    RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
    if (RexxSysBase == NULL)
    {
	CloseLibrary((struct Library *)DOSBase);
        return FALSE;
    }
    else
        return TRUE;
}

void SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    CloseLibrary((struct Library *) RexxSysBase);
}
