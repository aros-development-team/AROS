/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Text.datatype initialization code.
    Lang: English.
*/

#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "classbase.h"

/***************************************************************************************************/

AROS_SET_LIBFUNC(InitSem, struct ClassBase, lh)
{
    AROS_SET_LIBFUNC_INIT

    InitSemaphore(&lh->cb_LibLock);
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(InitSem, 0);

/***************************************************************************************************/
