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

#undef SysBase

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB

#include <libcore/libheader.c>

#define RexxSysBase ((struct RexxSysBase_intern *)lh)

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
   RexxSysBase->rexxmsgid = "RexxMsgId";
   return TRUE;
}

