/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "serial_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

AROS_SET_LIBFUNC(UXSer_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("SerialHIDD_Init()\n"));

    /*
        We map the memory into the shared memory space, because it is
        to be accessed by many processes, eg searching for a HIDD etc.

        Well, maybe once we've got MP this might help...:-)
    */
    LIBBASE->hdg_csd.utilitybase = OpenLibrary("utility.library", 37);
    if (LIBBASE->hdg_csd.utilitybase)
    {
	D(bug("  Got UtilityBase\n"));
	ReturnInt("SerialHIDD_Init", ULONG, TRUE);
    }

    ReturnInt("SerialHIDD_Init", ULONG, FALSE);

    AROS_SET_LIBFUNC_EXIT
}


AROS_SET_LIBFUNC(UXSer_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    EnterFunc(bug("SerialHIDD_Expunge()\n"));

    CloseLibrary(LIBBASE->hdg_csd.utilitybase);

    ReturnInt("SerialHIDD_Expunge", ULONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(UXSer_Init, 0)
ADD2EXPUNGELIB(UXSer_Expunge, 0)

int __pthread_do_exit(void)
{
    kprintf("serial.hidd: __pthread_do_exit() was called\n");
    return 0;
}


int __pthread_thread_self(void)
{
    kprintf("serial.hidd: __pthread_thread_self() was called\n");
    return 0;
}
