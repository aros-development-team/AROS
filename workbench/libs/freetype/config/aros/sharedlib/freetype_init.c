/*
    Copyright (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: CGFX Library
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE

#include <proto/exec.h>
#include <dos/dos.h>

#include "freetype_intern.h"
#include "libdefs.h"



#ifdef SysBase
#   undef SysBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((struct IntFreeTypeBase *)(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((struct IntFreeTypeBase *)(lib))->seglist)
#define LC_LIBBASESIZE		sizeof(struct IntFreeTypeBase)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct IntFreeTypeBase *)(lib))->libnode)
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_STATIC_EXPUNGELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>


#define SysBase (GetFreeTypeBase(FreeTypeBase)->sysbase)


static ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LIBBASETYPEPTR LIBBASE)
{

    DOSBase = OpenLibrary (DOSNAME, 36);

    return DOSBase ? TRUE : FALSE;
}

static void SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LIBBASETYPEPTR LIBBASE)
{
    CloseLibrary(DOSBase);
};
