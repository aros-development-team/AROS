/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of mathtrans.library
    Lang: english
*/

#include "mathtrans_intern.h"
#include "libdefs.h"

#define LC_SYSBASE_FIELD(lib)	(((LIBBASETYPEPTR       )(lib))->mtb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->mtb_SegList)
#define LC_RESIDENTNAME		mathtrans_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT
#define LC_RESIDENTPRI		0
#define LC_LIBBASESIZE		sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->LibNode)

#define LC_NO_CLOSELIB
#define LC_NO_OPENLIB
#define LC_RESIDENTPRI	    0

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct ExecBase * SysBase; /* global variable */
struct MathBase * MathBase;

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->mtb_SysBase;

    MathBase = (struct MathBase *)OpenLibrary ("mathffp.library", 0);

    if (!MathBase)
	return FALSE;

    return TRUE;
} /* L_InitLib */

void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (MathBase)
	CloseLibrary ((struct Library *)MathBase);

} /* L_ExpungeLib */

