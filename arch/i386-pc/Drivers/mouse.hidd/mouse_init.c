/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$ 

    Desc: Mouse hidd (COM/PS2/USB) for standalone i386 AROS
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "mouse.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

AROS_SET_LIBFUNC(PCMouse_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    struct mouse_staticdata *msd = &LIBBASE->msd;

    D(bug("_mouse: Initializing\n"));

    InitSemaphore(&msd->sema);

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(PCMouse_Init, 0)
