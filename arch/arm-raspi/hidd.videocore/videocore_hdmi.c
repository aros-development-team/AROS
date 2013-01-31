/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include "videocore_class.h"

int VideoCore_HDMI_SyncGen(struct List *modelist)
{
    struct DisplayMode *hdmi_mode;
    int modecount = 0;

    D(bug("[VideoCore] VideoCore_HDMI_SyncGen()\n"));

#warning "TODO: check if anything is attached to HDMI/DVI"

    return modecount;
}
