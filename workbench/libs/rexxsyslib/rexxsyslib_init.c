/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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

static int InitData(struct RexxSysBase_intern *RSBI)
{
   struct RxsLib *RexxSysBase = &RSBI->library;
   RSBI->rexxmsgid = "RexxMsgId";
   InitSemaphore(&RSBI->semaphore);
   NewList(&RexxSysBase->rl_LibList);
   RexxSysBase->rl_NumLib = 0;
   NewList(&RexxSysBase->rl_ClipList);
   RexxSysBase->rl_NumClip = 0;
   
   return TRUE;
}

ADD2INITLIB(InitData, 0);
