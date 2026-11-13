/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service initialisation
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "fs_intern.h"

static int FSService_Init(struct FSServiceBase *FSServiceBase)
{
    InitSemaphore(&FSServiceBase->fb_Sem);
    return TRUE;
}

ADD2INITLIB(FSService_Init, 0)
