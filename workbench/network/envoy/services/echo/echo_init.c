/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: echo.service initialisation
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "echo_intern.h"

static int Echo_Init(struct EchoBase *EchoBase)
{
    InitSemaphore(&EchoBase->eb_Sem);
    return TRUE;
}

ADD2INITLIB(Echo_Init, 0)
