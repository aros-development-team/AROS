/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Init of icon.library
    Lang: english
*/
#include <utility/utility.h> /* this must be before icon_intern.h */

#include "icon_intern.h"
#include "libdefs.h"

#define LC_SYSBASE_FIELD(lib)	(((LIBBASETYPEPTR       )(lib))->ib_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->ib_SegList)
#define LC_RESIDENTNAME		Icon_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT
#define LC_RESIDENTPRI		0
#define LC_LIBBASESIZE		sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->LibNode)

#define LC_NO_CLOSELIB
#define LC_NO_OPENLIB

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

#undef DOSBase
#undef SysBase

struct ExecBase   * SysBase; /* global variable */
struct DosLibrary * DOSBase;

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->ib_SysBase;

    DOSBase = (struct DosLibrary *) OpenLibrary (DOSNAME, 39);
    if (!DOSBase)
	return FALSE;

    LB(lh)->utilitybase = OpenLibrary (UTILITYNAME, 39);
    if (!LB(lh)->utilitybase)
	return NULL;

    LB(lh)->dsh.h_Entry = (void *)dosstreamhook;
    LB(lh)->dsh.h_Data  = DOSBase;

    return TRUE;
} /* L_InitLib */


void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (DOSBase)
	CloseLibrary ((struct Library *)DOSBase);

    if (LB(lh)->utilitybase)
	CloseLibrary (LB(lh)->utilitybase);

} /* L_ExpungeLib */
