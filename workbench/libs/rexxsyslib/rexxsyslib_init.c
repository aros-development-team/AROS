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
#include <proto/alib.h>

#include "rexxsyslib_intern.h"
#include LC_LIBDEFS_FILE

#undef SysBase

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB

#define LC_LIBHEADERTYPEPTR        LIBBASETYPEPTR
#define LC_LIB_FIELD(libBase)      (libBase)->library.rl_Node
#define LC_SYSBASE_FIELD(libBase)  (libBase)->library.rl_SysBase
#define LC_SEGLIST_FIELD(libBase)  (libBase)->library.rl_SegList
#define LC_LIBBASESIZE             (sizeof(LIBBASETYPE))

#include <libcore/libheader.c>

#define SysBase LC_SYSBASE_FIELD(lh)

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
   lh->rexxmsgid = "RexxMsgId";
   InitSemaphore(&lh->semaphore);
   NewList(&lh->library.rl_LibList);
   lh->library.rl_NumLib = 0;
   NewList(&lh->library.rl_ClipList);
   lh->library.rl_NumClip = 0;
   
   return TRUE;
}
