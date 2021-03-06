/*
    Copyright (C) 1995-2006, The AROS Development Team. All rights reserved.

    Desc: Text.datatype initialization code.
*/

#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "classbase.h"

/***************************************************************************************************/

static int InitSem(struct ClassBase *lh)
{
    InitSemaphore(&lh->cb_LibLock);

    return TRUE;
}

ADD2INITLIB(InitSem, 0);

/***************************************************************************************************/
