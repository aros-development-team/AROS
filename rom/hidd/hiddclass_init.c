/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hiddclass initialization code.
    Lang: English.
*/
#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include "hiddclass_intern.h"


#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hd_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hd_SegList)
#define LC_RESIDENTNAME         HIDD_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI          92
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->hd_LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB
#define LC_STATIC_INITLIB
/* to avoid removing the hiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>


#define SysBase (IntHIDDClassBase->hd_SysBase)

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

static ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("HIDDClass: OpenLib()\n"));
    
    ReturnInt("HIDDClass: Open", ULONG, init_hiddclass(LIBBASE));
}
