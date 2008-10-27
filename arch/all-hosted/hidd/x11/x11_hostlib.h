#ifndef X11_HOSTLIB_H
#define X11_HOSTLIB_H

#include <aros/config.h>

#include <X11/Xlib.h>

#if USE_XSHM
#include <sys/types.h>
#include <sys/shm.h>
#endif

struct x11_func {
    XImage * (*XCreateImage) ( Display* , Visual* , unsigned int , int , int , char* , unsigned int , unsigned int , int , int );
    int (*XInitImage) ( XImage* );
    XImage * (*XGetImage) ( Display* , Drawable , int , int , unsigned int , unsigned int , unsigned long , int );
    Display * (*XOpenDisplay) ( const char* );
    char * (*XDisplayName) ( const char* );
    Atom (*XInternAtom) ( Display* , const char* , int );
    Colormap (*XCreateColormap) ( Display* , Window , Visual* , int );
    Cursor (*XCreatePixmapCursor) ( Display* , Pixmap , Pixmap , XColor* , XColor* , unsigned int , unsigned int );
    Cursor (*XCreateFontCursor) ( Display* , unsigned int );
    GC (*XCreateGC) ( Display* , Drawable , unsigned long , XGCValues* );
    Pixmap (*XCreatePixmap) ( Display* , Drawable , unsigned int , unsigned int , unsigned int );
    Window (*XCreateSimpleWindow) ( Display* , Window , int , int , unsigned int , unsigned int , unsigned int , unsigned long , unsigned long );
    Window (*XCreateWindow) ( Display* , Window , int , int , unsigned int , unsigned int , unsigned int , int , unsigned int , Visual* , unsigned long , XSetWindowAttributes* );
    KeySym (*XLookupKeysym) ( XKeyEvent* , int );
    long (*XMaxRequestSize) ( Display* );
    VisualID (*XVisualIDFromVisual) ( Visual* );
    XErrorHandler (*XSetErrorHandler) ( XErrorHandler );
    XIOErrorHandler (*XSetIOErrorHandler) ( XIOErrorHandler );
    int (*XSetWMHints) ( Display* , Window , XWMHints* );
    int (*XSetWMProtocols) ( Display* , Window , Atom* , int );
    int (*XAutoRepeatOff) ( Display* );
    int (*XAutoRepeatOn) ( Display* );
    int (*XChangeGC) ( Display* , GC , unsigned long , XGCValues* );
    int (*XChangeProperty) ( Display* , Window , Atom , Atom , int , int , const unsigned char* , int );
    int (*XChangeWindowAttributes) ( Display* , Window , unsigned long , XSetWindowAttributes* );
    int (*XClearArea) ( Display* , Window , int , int , unsigned int , unsigned int , int );
    int (*XCloseDisplay) ( Display* );
    int (*XConfigureWindow) ( Display* , Window , unsigned int , XWindowChanges* );
    int (*XConvertSelection) ( Display* , Atom , Atom , Atom , Window , Time );
    int (*XCopyArea) ( Display* , Drawable , Drawable , GC , int , int , unsigned int , unsigned int , int , int );
    int (*XCopyPlane) ( Display* , Drawable , Drawable , GC , int , int , unsigned int , unsigned int , int , int , unsigned long );
    int (*XDefineCursor) ( Display* , Window , Cursor );
    int (*XDeleteProperty) ( Display* , Window , Atom );
    int (*XDestroyWindow) ( Display* , Window );
    int (*XDrawArc) ( Display* , Drawable , GC , int , int , unsigned int , unsigned int , int , int );
    int (*XDrawLine) ( Display* , Drawable , GC , int , int , int , int );
    int (*XDrawPoint) ( Display* , Drawable , GC , int , int );
    int (*XDrawString) ( Display* , Drawable , GC , int , int , const char* , int );
    int (*XEventsQueued) ( Display* , int );
    int (*XFillRectangle) ( Display* , Drawable , GC , int , int , unsigned int , unsigned int );
    int (*XFlush) ( Display* );
    int (*XFree) ( void* );
    int (*XFreeColormap) ( Display* , Colormap );
    int (*XFreeGC) ( Display* , GC );
    int (*XFreePixmap) ( Display* , Pixmap );
    int (*XGetErrorText) ( Display* , int , char* , int );
    XVisualInfo * (*XGetVisualInfo) ( Display* , long , XVisualInfo* , int* );
    int (*XGetWindowProperty) ( Display* , Window , Atom , long , long , int , Atom , Atom* , int* , unsigned long* , unsigned long* , unsigned char** );
    int (*XGetWindowAttributes) ( Display* , Window , XWindowAttributes* );
    int (*XGrabKeyboard) ( Display* , Window , int , int , int , Time );
    int (*XGrabPointer) ( Display* , Window , int , unsigned int , int , int , Window , Cursor , Time );
    int (*XMapRaised) ( Display* , Window );
    int (*XMapWindow) ( Display* , Window );
    int (*XNextEvent) ( Display* , XEvent* );
    int (*XParseGeometry) ( const char* , int* , int* , unsigned int* , unsigned int* );
    int (*XPending) ( Display* );
    int (*XPutImage) ( Display* , Drawable , GC , XImage* , int , int , int , int , unsigned int , unsigned int );
    int (*XRecolorCursor) ( Display* , Cursor , XColor* , XColor* );
    int (*XRefreshKeyboardMapping) ( XMappingEvent* );
    int (*XSelectInput) ( Display* , Window , long );
    int (*XSendEvent) ( Display* , Window , int , long , XEvent* );
    int (*XSetBackground) ( Display* , GC , unsigned long );
    int (*XSetClipMask) ( Display* , GC , Pixmap );
    int (*XSetClipRectangles) ( Display* , GC , int , int , XRectangle* , int , int );
    int (*XSetFillStyle) ( Display* , GC , int );
    int (*XSetForeground) ( Display* , GC , unsigned long );
    int (*XSetFunction) ( Display* , GC , int );
    int (*XSetIconName) ( Display* , Window , const char* );
    int (*XSetSelectionOwner) ( Display* , Atom , Window , Time );
    int (*XStoreColor) ( Display* , Colormap , XColor* );
    int (*XStoreName) ( Display* , Window , const char* );
    int (*XSync) ( Display* , int );
    int (*XAllocColor) ( Display* , Colormap , XColor* );
    int (*XLookupString) ( XKeyEvent* , char* , int , KeySym* , XComposeStatus* );
};

struct libc_func {
#if USE_XSHM
    key_t (*ftok) (const char *, int);
    int (*shmctl) (int, int, struct shmid_ds *);
    int (*shmget) (key_t, size_t, int);
    void * (*shmat) (int, const void *, int);
    int (*shmdt) (const void *);
#endif
    int (*raise) (int);
};

extern void *x11_handle;
extern struct x11_func x11_func;

extern void *libc_handle;
extern struct libc_func libc_func;

#define X11_SOFILE  "libX11.so.6"
#define LIBC_SOFILE "libc.so.6"

#define XCALL(func,...) (x11_func.func(__VA_ARGS__))
#define CCALL(func,...) (libc_func.func(__VA_ARGS__))

#endif
