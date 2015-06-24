#ifndef X11_HOSTLIB_H
#define X11_HOSTLIB_H

#include <aros/config.h>

#define timeval sys_timeval
#ifndef _XLIB_H_
#   include <X11/Xlib.h>
#endif
#undef timeval

struct x11_func
{
    Display * (*XOpenDisplay) ( const char* );
    int (*XCloseDisplay) ( Display* );
    int (*XFree) ( void* );
#if defined(RENDERER_SEPARATE_X_WINDOW)
    Colormap (*XCreateColormap) ( Display* , Window , Visual* , int );
    Window (*XCreateWindow) ( Display* , Window , int , int , unsigned int , unsigned int , unsigned int , int , unsigned int , Visual* , unsigned long , XSetWindowAttributes* );
    int (*XDestroyWindow) ( Display* , Window );
    int (*XFlush) ( Display* );
    int (*XMapWindow) ( Display* , Window );
#endif
};

extern void *x11_handle;
extern struct x11_func x11_func;

#define XCALL(func,...) (x11_func.func(__VA_ARGS__))

#ifdef HOST_OS_linux
#define X11_SOFILE    "libX11.so.6"
#endif

#ifdef HOST_OS_darwin
#define X11_SOFILE    "/usr/X11/lib/libX11.6.dylib"
#endif

#ifndef X11_SOFILE
#define X11_SOFILE    "libX11.so"
#endif

#endif
