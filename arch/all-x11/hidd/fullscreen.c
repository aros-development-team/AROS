#include <aros/config.h>
#include <X11/Xlib.h>

#if USE_VIDMODE

#include <X11/extensions/xf86vmode.h>

#define __typedef_BYTE
#define __typedef_BOOL
typedef unsigned char UBYTE;

#include <aros/symbolsets.h>

#define DEBUG 0
#include <aros/debug.h>


#include <proto/hostlib.h>

static void *xvm_handle = NULL;

static struct {
    Bool (*XF86VidModeSwitchToMode) ( Display* , int , XF86VidModeModeInfo* );
    Bool (*XF86VidModeSetViewPort) ( Display* , int , int , int );
    Bool (*XF86VidModeQueryVersion) ( Display* , int* , int* );
    Bool (*XF86VidModeQueryExtension) ( Display* , int* , int* );
    Bool (*XF86VidModeGetAllModeLines) ( Display* , int , int* , XF86VidModeModeInfo*** );
} xvm_func;

static const char *xvm_func_names[] = {
    "XF86VidModeSwitchToMode",
    "XF86VidModeSetViewPort",
    "XF86VidModeQueryVersion",
    "XF86VidModeQueryExtension",
    "XF86VidModeGetAllModeLines"
};

#define XVM_SOFILE "libXxf86vm.so"

#define XVMCALL(func,...)   (xvm_func.func(__VA_ARGS__))

extern void *x11_hostlib_load_so(const char *, const char **, int, void **);

static int xvm_hostlib_init(void *libbase) {
    D(bug("[x11] xvm hostlib init\n"));

    if ((xvm_handle = x11_hostlib_load_so(XVM_SOFILE, xvm_func_names, 5, (void **) &xvm_func)) == NULL)
        return FALSE;

    return TRUE;
}

static int xvm_hostlib_expunge(void *libbase) {
    D(bug("[x11] xvm hostlib expunge\n"));

    if (xvm_handle != NULL)
        HostLib_Close(xvm_handle, NULL);

    return TRUE;
}

ADD2INITLIB(xvm_hostlib_init, 1)
ADD2EXPUNGELIB(xvm_hostlib_expunge, 1)


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
