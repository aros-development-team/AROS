#include <aros/config.h>
#include <X11/Xlib.h>

#if USE_VIDMODE

#include <X11/extensions/xf86vmode.h>

static XF86VidModeModeInfo **videomodes;
static int  	    	     num_videomodes;

int x11_fullscreen_supported(Display *display)
{
    int majorversion, minorversion;
    int eventbase, errorbase;

    if (!XF86VidModeQueryVersion(display, &majorversion, &minorversion))
    {
	return 0;
	
    }
    if (!XF86VidModeQueryExtension(display, &eventbase, &errorbase))
    {
	return 0;
    }
    
    if (XF86VidModeGetAllModeLines(display, DefaultScreen(display), &num_videomodes, &videomodes))
    {
    	if (num_videomodes >= 2) return 1;
    }
    
    return 0;
}

void x11_fullscreen_switchmode(Display *display, int *w, int *h)
{
    int i, mode;
   
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
    
    XF86VidModeSwitchToMode(display, DefaultScreen(display), videomodes[mode]);
    XF86VidModeSetViewPort(display, DefaultScreen(display), 0, 0);
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
