/*
    Copyright � 2013-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCM VideoCore4 Gfx Hidd hardware functions
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>

#include <asm/io.h>

#include "vc4gfx_hidd.h"
#include "vc4gfx_hardware.h"

BOOL FNAME_HW(InitGfxHW)(APTR param0)
{
    //struct VideoCore_staticdata *xsd = param0;

    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));

    return TRUE;
}
