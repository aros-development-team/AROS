/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Init of icon.library
    Lang: english
*/
#include <utility/utility.h> /* this must be before icon_intern.h */

#include "icon_intern.h"
#include "libdefs.h"

#define LC_NO_CLOSELIB
#define LC_RESIDENTPRI	    -120

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

#undef DOSBase;

struct ExecBase   * SysBase; /* global variable */
struct DosLibrary * DOSBase;

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->lh_SysBase;

    return TRUE;
} /* L_InitLib */

ULONG SAVEDS L_OpenLib (LC_LIBHEADERTYPEPTR lh)
{
#if 0
    if (!LB(lh)->dosbase)
	LB(lh)->dosbase = OpenLibrary (DOSNAME, 39);

    if (!LB(lh)->dosbase)
	return FALSE;
#else
    if (!DOSBase)
	DOSBase = (struct DosLibrary *) OpenLibrary (DOSNAME, 39);

    if (!DOSBase)
	return FALSE;
#endif

    if (!LB(lh)->utilitybase)
	LB(lh)->utilitybase = OpenLibrary (UTILITYNAME, 39);

    if (!LB(lh)->utilitybase)
	return NULL;

    LB(lh)->dsh.h_Entry = (void *)dosstreamhook;
    LB(lh)->dsh.h_Data  = DOSBase;

    return TRUE;
} /* L_OpenLib */

void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (DOSBase)
	CloseLibrary ((struct Library *)DOSBase);

    if (LB(lh)->utilitybase)
	CloseLibrary (LB(lh)->utilitybase);

} /* L_ExpungeLib */

