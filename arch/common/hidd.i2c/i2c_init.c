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

    LIBBASE->sd.utilitybase = OpenLibrary((STRPTR)UTILITYNAME, 0);
    if (LIBBASE->sd.utilitybase != NULL)
    {
        D(bug("[I2C] Got UtilityBase @ 0x%08x\n", LIBBASE->sd.utilitybase));
        return TRUE;
    }

    return FALSE;
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(I2C_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    D(bug("[I2C] Closing libraries\n"));
    CloseLibrary(LIBBASE->sd.utilitybase);

    D(bug("[I2C] Goodbye\n"));
   
    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(I2C_Init, 0)
ADD2EXPUNGELIB(I2C_Expunge, 0)
