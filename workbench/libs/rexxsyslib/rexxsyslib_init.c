/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RexxSys initialization code.
    Lang: English
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/alib.h>

#include "rexxsyslib_intern.h"

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

#undef SysBase
#define SysBase LC_SYSBASE_FIELD(LIBBASE)

AROS_SET_LIBFUNC(InitData, LIBBASETYPE, LIBBASE)
{
   LIBBASE->rexxmsgid = "RexxMsgId";
   InitSemaphore(&LIBBASE->semaphore);
   NewList(&LIBBASE->library.rl_LibList);
   LIBBASE->library.rl_NumLib = 0;
   NewList(&LIBBASE->library.rl_ClipList);
   LIBBASE->library.rl_NumClip = 0;
   
   return TRUE;
}

ADD2INITLIB(InitData, 0);
