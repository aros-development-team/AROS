/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of mathieeedoubbas.library
    Lang: english
*/
#include <utility/utility.h> /* this must be before mathieeedoubbas_intern.h */

#include "mathieeedoubtrans_intern.h"
#include LC_LIBDEFS_FILE

#define LC_NO_OPENLIB
#define LC_NO_EXPUNGELIB
#define LC_RESIDENTPRI	    -120

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct ExecBase * SysBase; /* global variable */
struct Library * MathIeeeDoubBasBase;

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->lh_SysBase;

    MathIeeeDoubBasBase = (struct Library *)OpenLibrary("mathieeedoubbas.library",39);
    if (!MathIeeeDoubBasBase)
      return FALSE;

    return TRUE;
} /* L_InitLib */

void L_CloseLib (LC_LIBHEADERTYPEPTR lh)
{
    if (MathIeeeDoubBasBase)
      CloseLibrary((struct Library *)MathIeeeDoubBasBase);
} /* L_OpenLib */

