/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux hidd initialization code.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <utility/utility.h>
#include <oop/oop.h>
#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#include "linux_intern.h"

#include LC_LIBDEFS_FILE
#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

static OOP_AttrBase HiddPixFmtAttrBase = 0;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

static int Init_Hidd(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&LIBBASE->lsd.sema);

#if BUFFERED_VRAM
    InitSemaphore(&LIBBASE->lsd.framebufferlock);
#endif
	
    if (!OOP_ObtainAttrBases(abd))
	return FALSE;
kprintf("OBTAINED ATTRBASES\n");	    

    LIBBASE->lsd.input_task = init_linuxinput_task(&LIBBASE->lsd);
    if (NULL == LIBBASE->lsd.input_task)
	return FALSE;
kprintf("GOT INPUT TASK\n");

    return TRUE;
}

static int Expunge_Hidd(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(abd);

    if (NULL != LIBBASE->lsd.input_task)
	kill_linuxinput_task(&LIBBASE->lsd);
    
    return TRUE;
}

ADD2INITLIB(Init_Hidd, 1)
ADD2EXPUNGELIB(Expunge_Hidd, 1)
