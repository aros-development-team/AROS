/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Init of mathieeedoubbas.library
    Lang: english
*/
#include <utility/utility.h> /* this must be before mathieeedoubbas_intern.h */

#include "mathieeedoubbas_intern.h"
#include "libdefs.h"

#define LC_NO_OPENLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB
#define LC_RESIDENTPRI	    -120

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct ExecBase * SysBase; /* global variable */

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->lh_SysBase;

    return TRUE;
} /* L_InitLib */

