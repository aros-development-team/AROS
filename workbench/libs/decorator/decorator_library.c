/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: decorator.library initialization
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "decorator_intern.h"

static ULONG Decorator_Init(struct DecoratorBase *DecoratorBase)
{
    return TRUE;
}

ADD2INITLIB(Decorator_Init, 0)
