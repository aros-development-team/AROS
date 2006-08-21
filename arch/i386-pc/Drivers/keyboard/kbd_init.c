/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: kbd Hidd for standalone i386 AROS
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "kbd.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0 
#include <aros/debug.h>

static int PCKbd_Init(LIBBASETYPEPTR LIBBASE)
{
    struct kbd_staticdata *xsd = &LIBBASE->ksd;
	
    InitSemaphore( &xsd->sema );
	
    return TRUE;
}

ADD2INITLIB(PCKbd_Init, 0)
ADD2LIBS("irq.hidd", 0, static struct Library *, __irqhidd)

