/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Hiddclass initialization code.
    Lang: English.
*/
#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include "hiddclass_intern.h"

#warning FIXME: define NT_HIDD in libraries.h or something else
#define NT_HIDD NT_LIBRARY

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hd_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hd_SegList)
#define LC_RESIDENTNAME         hiddclass_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT |  RTF_COLDSTART
#define LC_RESIDENTPRI          90
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->hd_LibNode)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB

/* to avoid removing the hiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>


ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    EnterFunc(bug("HIDDClass: OpenLib()\n"));

    if(LC_LIB_FIELD(lh).lib_OpenCnt == 0)
    {
       ReturnInt("HIDDClass: Open", ULONG, init_hiddclass(lh));
    }
    else
    {
       ReturnInt("HIDDClass: OpenLib", ULONG, 1);
    }
}


void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR lh)
{
    EnterFunc(bug("HIDDClass: CloseLib()\n"));

    if(LC_LIB_FIELD(lh).lib_OpenCnt == 0)
    {
        free_hiddclass(lh);
    }

    ReturnVoid("HIDDClass: CloseLib");
}

