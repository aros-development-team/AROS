/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
*/

#include "x11_debug.h"

#include <aros/symbolsets.h>

#include "x11_types.h"
#include LC_LIBDEFS_FILE
#include "x11_hostlib.h"

void *x11_handle = NULL;
void *libc_handle = NULL;
void *xf86vm_handle = NULL;
#if !X11SOFTMOUSE
void *xcursor_handle = NULL;
#endif

struct x11_func x11_func;
struct libc_func libc_func;
struct xf86vm_func xf86vm_func;
#if !X11SOFTMOUSE
struct xcursor_func xcursor_func;
#endif

static const char *xf86vm_func_names[] = {
    "XF86VidModeGetAllModeLines",
    "XF86VidModeSwitchToMode",
    "XF86VidModeSetViewPort",
    "XF86VidModeQueryVersion",
    "XF86VidModeQueryExtension"
};

#define XF86VM_NUM_FUNCS (5)

static const char *x11_func_names[] = {
    "XCreateImage",
    "XInitImage",
    "XGetImage",
    "XOpenDisplay",
    "XDisplayName",
    "XInternAtom",
    "XCreateColormap",
    "XCreatePixmapCursor",
    "XCreateFontCursor",
    "XCreateGC",
    "XCreatePixmap",
    "XCreateSimpleWindow",
    "XCreateWindow",
    "XLookupKeysym",
    "XMaxRequestSize",
    "XVisualIDFromVisual",
    "XSetErrorHandler",
    "XSetIOErrorHandler",
    "XSetWMHints",
    "XGetWMHints",
    "XSetWMNormalHints",
    "XSetWMProtocols",
    "XAutoRepeatOff",
    "XAutoRepeatOn",
    "XChangeGC",
    "XChangeProperty",
    "XChangeWindowAttributes",
    "XClearArea",
    "XCloseDisplay",
    "XConfigureWindow",
    "XConvertSelection",
    "XCopyArea",
    "XCopyPlane",
    "XDefineCursor",
    "XDeleteProperty",
    "XDestroyWindow",
    "XDrawArc",
    "XDrawLine",
    "XDrawPoint",
    "XDrawString",
    "XEventsQueued",
    "XFillRectangle",
    "XFlush",
    "XFree",
    "XFreeColormap",
    "XFreeGC",
    "XFreePixmap",
    "XGetErrorText",
    "XGetVisualInfo",
    "XGetWindowProperty",
    "XGetWindowAttributes",
    "XGrabKeyboard",
    "XGrabPointer",
    "XMapRaised",
    "XMapWindow",
    "XNextEvent",
    "XParseGeometry",
    "XPending",
    "XPutImage",
    "XRecolorCursor",
    "XRefreshKeyboardMapping",
    "XSelectInput",
    "XSendEvent",
    "XSetBackground",
    "XSetClipMask",
    "XSetClipRectangles",
    "XSetFillStyle",
    "XSetForeground",
    "XSetFunction",
    "XSetIconName",
    "XSetSelectionOwner",
    "XStoreColor",
    "XStoreName",
    "XSync",
    "XAllocColor",
    "XLookupString",
    "XQueryExtension",
    "XDefaultScreen",
    "XRootWindow",
    "XAllocClassHint",
    "XSetClassHint",
    "XSetInputFocus"
#if DEBUG_X11_SYNCHRON
    , "XSynchronize"
#endif
};

#if DEBUG_X11_SYNCHRON
#define X11_NUM_FUNCS (83)
#else
#define X11_NUM_FUNCS (82)
#endif


static const char *libc_func_names[] = {
#if USE_XSHM
    "ftok",
    "shmctl",
    "shmget",
    "shmat",
    "shmdt",
#endif
    "raise"
};

#if USE_XSHM
#define LIBC_NUM_FUNCS (6)
#else
#define LIBC_NUM_FUNCS (1)
#endif

#if !X11SOFTMOUSE
static const char *xcursor_func_names[] = {
    "XcursorImageCreate",
    "XcursorImageDestroy",
    "XcursorImageLoadCursor"
};

#define XCURSOR_NUM_FUNCS (3)
#endif

void *HostLibBase;

void *x11_hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr)
{
    void *handle;
    char *err;
    int i;

    D(bug("[X11host] %s('%s')\n", __PRETTY_FUNCTION__, sofile));

    D(bug("[X11host] %s: attempting to load %d functions\n", __PRETTY_FUNCTION__, nfuncs));

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        bug("[X11host] %s: failed to open '%s': %s\n", __PRETTY_FUNCTION__, sofile, err);
        return NULL;
    }

    for (i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        D(bug("[X11host] %s: 0x%p = '%s'\n", __PRETTY_FUNCTION__, funcptr[i], names[i]));
        if (err != NULL) {
            bug("[X11host] %s: failed to get symbol '%s' (%s)\n", __PRETTY_FUNCTION__, names[i], err);
            HostLib_Close(handle, NULL);
            return NULL;
        }
    }

    return handle;
}

static int x11_hostlib_init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[X11host] %s()\n", __PRETTY_FUNCTION__));

    if ((HostLibBase = OpenResource("hostlib.resource")) == NULL) {
        bug("[X11host] %s: failed to open hostlib.resource!\n", __PRETTY_FUNCTION__);
        return FALSE;
    }

#if !X11SOFTMOUSE
    if ((xcursor_handle = x11_hostlib_load_so(XCURSOR_SOFILE,
        xcursor_func_names, XCURSOR_NUM_FUNCS, (void **) &xcursor_func))
        == NULL)
        return FALSE;
#endif

    if ((xf86vm_handle = x11_hostlib_load_so(XF86VM_SOFILE, xf86vm_func_names, XF86VM_NUM_FUNCS, (void **) &xf86vm_func)) == NULL)
        return FALSE;

    if ((x11_handle = x11_hostlib_load_so(X11_SOFILE, x11_func_names, X11_NUM_FUNCS, (void **) &x11_func)) == NULL)
        return FALSE;

    if ((libc_handle = x11_hostlib_load_so(LIBC_SOFILE, libc_func_names, LIBC_NUM_FUNCS, (void **) &libc_func)) == NULL) {
        HostLib_Close(x11_handle, NULL);
        return FALSE;
    }

    return TRUE;
}

static int x11_hostlib_expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[X11host] %s()\n", __PRETTY_FUNCTION__));

#if !X11SOFTMOUSE
    if (xcursor_handle != NULL)
        HostLib_Close(xcursor_handle, NULL);
#endif

    if (xf86vm_handle != NULL)
        HostLib_Close(xf86vm_handle, NULL);

    if (x11_handle != NULL)
        HostLib_Close(x11_handle, NULL);

    if (libc_handle != NULL)
        HostLib_Close(libc_handle, NULL);

    return TRUE;
}

ADD2INITLIB(x11_hostlib_init, 0)
ADD2EXPUNGELIB(x11_hostlib_expunge, 0)

