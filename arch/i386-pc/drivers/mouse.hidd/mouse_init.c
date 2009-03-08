/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
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

static int PCMouse_Init(LIBBASETYPEPTR LIBBASE)
{
    struct mouse_staticdata *msd = &LIBBASE->msd;

    D(bug("_mouse: Initializing\n"));

    InitSemaphore(&msd->sema);

    return TRUE;
}

ADD2INITLIB(PCMouse_Init, 0)
ADD2LIBS("irq.hidd", 0, struct Library *, __irqhidd)
