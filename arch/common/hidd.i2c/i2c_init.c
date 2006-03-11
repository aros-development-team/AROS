/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <utility/utility.h>

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>

#include "i2c.h"
#include LC_LIBDEFS_FILE

AROS_SET_LIBFUNC(I2C_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    InitSemaphore(&LIBBASE->sd.driver_lock);
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(I2C_Init, 0)
