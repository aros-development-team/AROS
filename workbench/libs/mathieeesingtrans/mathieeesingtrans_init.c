/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of mathieeesingtrans.library
    Lang: english
*/

#include "mathieeesingtrans_intern.h"
#include LC_LIBDEFS_FILE

#define LC_SYSBASE_FIELD(lib)	(((LIBBASETYPEPTR)(lib))->mist_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR)(lib))->mist_SegList)
#define LC_RESIDENTNAME		mathieeesingtrans_resident
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

struct ExecBase   * SysBase; /* global variable */
struct MathIeeeSingBasBase * MathIeeeSingBasBase;

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->mist_SysBase;

    if (!MathIeeeSingBasBase)
	MathIeeeSingBasBase = (struct MathIeeeSingBasBase *) OpenLibrary ("mathieeesingbas.library", 39);

    if (!MathIeeeSingBasBase)
	return FALSE;

    return TRUE;
} /* L_InitLib */


void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (MathIeeeSingBasBase)
	CloseLibrary ((struct Library *)MathIeeeSingBasBase);
} /* L_ExpungeLib */
