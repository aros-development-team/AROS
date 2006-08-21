/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: tap_init.c 23803 2005-12-11 11:58:09Z verhaegs $

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "tap_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

static int UXTap_Init(LIBBASETYPEPTR LIBBASE)
{
    ReturnInt("TapHIDD_Init", ULONG, TRUE);
}

static int UXTap_Expunge(LIBBASETYPEPTR LIBBASE)
{
    ReturnInt("TapHIDD_Expunge", int, TRUE);
}

ADD2INITLIB(UXTap_Init, 0)
ADD2EXPUNGELIB(UXTap_Expunge, 0)
