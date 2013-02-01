/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: VideoCore4 hardware functions
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>

#include <asm/io.h>

#include "videocore_class.h"
#include "videocore_hardware.h"

BOOL initVideoCoreGfxHW(APTR param0)
{
    struct VideoCore_staticdata *xsd = param0;

    D(bug("[VideoCoreGfx] initVideoCoreGfxHW()\n"));

    return TRUE;
}
