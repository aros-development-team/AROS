/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include "videocoregfx_class.h"

int FNAME_SUPPORT(HDMI_SyncGen)(struct List *modelist)
{
    struct DisplayMode *hdmi_mode;
    int modecount = 0;

    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));

#warning "TODO: check if anything is attached to HDMI/DVI"

    return modecount;
}
