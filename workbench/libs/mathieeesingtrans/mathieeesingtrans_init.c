/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Init of mathieeesingtrans.library
    Lang: english
*/
#include <utility/utility.h> /* must be include before mathieeesingtrans_intern.h */

#include "mathieeesingtrans_intern.h"
#include "libdefs.h"

#define LC_NO_EXPUNGELIB
#define LC_RESIDENTPRI	    -120

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct ExecBase   * SysBase; /* global variable */
struct MathIeeeSingBasBase * MathIeeeSingBasBase;

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->lh_SysBase;

    return TRUE;
} /* L_InitLib */

ULONG SAVEDS L_OpenLib (LC_LIBHEADERTYPEPTR lh)
{
    if (!MathIeeeSingBasBase)
	MathIeeeSingBasBase = (struct MathIeeeSingBasBase *) OpenLibrary ("mathieeesingbas.library", 39);

    if (!MathIeeeSingBasBase)
	return FALSE;
	
    return TRUE;
} /* L_OpenLib */

void SAVEDS L_CloseLib (LC_LIBHEADERTYPEPTR lh)
{
    if (MathIeeeSingBasBase)
	CloseLibrary ((struct Library *)MathIeeeSingBasBase);

} /* L_CloseLib */

