/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Init of mathtrans.library
    Lang: english
*/
#include <utility/utility.h> /* this must be before mathtrans_intern.h */

#include "mathtrans_intern.h"
#include "libdefs.h"

#define LC_NO_CLOSELIB
#define LC_RESIDENTPRI	    -120

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct ExecBase * SysBase; /* global variable */
struct MathBase * MathBase;

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->lh_SysBase;

    return TRUE;
} /* L_InitLib */

ULONG SAVEDS L_OpenLib (LC_LIBHEADERTYPEPTR lh)
{
    if (!MathBase)
	MathBase = (struct MathBase *)OpenLibrary ("mathffp.library", 39);

    if (!MathBase)
	return FALSE;

    return TRUE;
} /* L_OpenLib */

void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (MathBase)
	CloseLibrary ((struct Library *)MathBase);

} /* L_ExpungeLib */

