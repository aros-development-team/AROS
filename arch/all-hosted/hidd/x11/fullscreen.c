/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Enable fullscreen mode.
    Lang: English.
*/

#include <aros/config.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#if USE_VIDMODE

#include <X11/extensions/xf86vmode.h>

#define __typedef_BYTE
#define __typedef_BOOL
typedef unsigned char UBYTE;

#include <aros/symbolsets.h>

#define HAVE_XF86VMODE_H

#include "x11_hostlib.h"

static XF86VidModeModeInfo **videomodes;
static int  	    	     num_videomodes;

int x11_fullscreen_supported(Display *display)
{
    int majorversion, minorversion;
    int eventbase, errorbase;

    if (!XVMCALL(XF86VidModeQueryVersion, display, &majorversion, &minorversion))
    {
	return 0;
	
    }
    if (!XVMCALL(XF86VidModeQueryExtension, display, &eventbase, &errorbase))
    {
	return 0;
    }
    
    if (XVMCALL(XF86VidModeGetAllModeLines, display, DefaultScreen(display), &num_videomodes, &videomodes))
    {
    	if (num_videomodes >= 2) return 1;
    }
    
    return 0;
}

void x11_fullscreen_switchmode(Display *display, int *w, int *h)
{
    int i, mode;

    if (videomodes == NULL)
	return;

    for(i = 1, mode = 0; i < num_videomodes; i++)
    {
    	if ((videomodes[i]->hdisplay >= *w) &&
	    (videomodes[i]->vdisplay >= *h) &&
	    (videomodes[i]->hdisplay < videomodes[mode]->hdisplay) &&
	    (videomodes[i]->vdisplay < videomodes[mode]->vdisplay))
	{
	    mode = i;
	}
    }
    
    *w = videomodes[mode]->hdisplay;
    *h = videomodes[mode]->vdisplay;
    
    XVMCALL(XF86VidModeSwitchToMode, display, DefaultScreen(display), videomodes[mode]);
    XVMCALL(XF86VidModeSetViewPort, display, DefaultScreen(display), 0, 0);
}

#else /* if USE_VIDMODE */

int x11_fullscreen_supported(Display *display)
{
    return 0;
}

void x11_fullscreen_switchmode(Display *display, int *w, int *h)
{
}
#endif
