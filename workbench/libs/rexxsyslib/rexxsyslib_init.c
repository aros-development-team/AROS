/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RexxSys initialization code.
    Lang: English
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/exec.h>

#include "rexxsyslib_intern.h"
#include "libdefs.h"

/****************************************************************************************/

#undef SysBase

/* #define LC_NO_INITLIB    */
/* #define LC_NO_OPENLIB    */
/* #define LC_NO_CLOSELIB   */
/* #define LC_NO_EXPUNGELIB */

#include <libcore/libheader.c>

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR RexxSysBase)
{
    D(bug("Inside Init func of rexxsyslib.library\n"));

    return TRUE;
}

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR RexxSysBase)
{
    D(bug("Inside Open func of rexxsyslib.library\n"));

    return TRUE;
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR RexxSysBase)
{
    D(bug("Inside Close func of rexxsyslib.library\n"));
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR RexxSysBase)
{
    D(bug("Inside Expunge func of rexxsyslib.library\n"));
}

/****************************************************************************************/
