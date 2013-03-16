/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: VideoCore4 hardware functions
    Lang: English
*/

#define DEBUG 0
#include <aros/debug.h>

#include <asm/io.h>

#include "videocoregfx_class.h"
#include "videocoregfx_hardware.h"

BOOL FNAME_HW(InitGfxHW)(APTR param0)
{
    struct VideoCore_staticdata *xsd = param0;

    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));

    return TRUE;
}
