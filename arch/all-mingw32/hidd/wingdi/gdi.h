#ifndef HIDD_GDI_H
#define HIDD_GDI_H

/*
    Copyright  1995-2008, The AROS Development Team. All rights reserved.
    $Id: gdi.h 27106 2007-10-28 10:49:03Z verhaegs $

    Desc: Include for the gdi HIDD.
    Lang: English.
*/

#ifndef WM_QUIT
#define WM_QUIT 18
#define WM_USER 1024
#endif

#define NOTY_WINCREATE    (WM_USER+1)
#define NOTY_WINDISPOSE   (WM_USER+2)
#define NOTY_RESIZEWINDOW (WM_USER+3)

struct NewWindowMsg
{
    void *window;
    ULONG xsize;
    ULONG ysize;
};

#ifdef __AROS__

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>

/* #define GDI_LOAD_KEYMAPTABLE	1*/

#include "gdi_hostlib.h"


/***** GDIMouse HIDD *******************/

/* Private data */
struct pHidd_Mouse_Event;
struct gdimouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};

/* IDs */
#define IID_Hidd_GDIMouse	"hidd.mouse.gdi"
#define CLID_Hidd_GDIMouse	"hidd.mouse.gdi"


/* Methods */
enum
{
    moHidd_GDIMouse_HandleEvent
};

struct pHidd_GDIMouse_HandleEvent
{
    OOP_MethodID mID;
//  XEvent *event;
};

//VOID Hidd_GDIMouse_HandleEvent(OOP_Object *o, XEvent *event);

/***** GDIKbd HIDD *******************/

/* Private data */
struct gdikbd_data
{
    VOID  (*kbd_callback)(APTR, UWORD);
    APTR    callbackdata;
    UWORD   prev_keycode;
};

/* IDs */
#define IID_Hidd_GDIKbd		"hidd.kbd.gdi"
#define CLID_Hidd_GDIKbd	"hidd.kbd.gdi"

/* Methods */
enum
{
    moHidd_GDIKbd_HandleEvent
};

struct pHidd_GDIKbd_HandleEvent
{
    OOP_MethodID     mID;
//  XEvent  	    *event;
};

//VOID Hidd_GDIKbd_HandleEvent(OOP_Object *o, XEvent *event);

struct gdi_staticdata
{
    struct SignalSemaphore   sema; /* Protecting this whole struct */
    struct SignalSemaphore   gdisema;
    
    /* This port is used for asking the gdi task for notifications
       on when some event occurs, for example MapNotify
    */
    struct MsgPort  	    *gditask_notify_port;
    struct MsgPort  	    *gditask_quit_port;
    
    APTR 	    	     display;
    BOOL    	    	     local_display;
    
    ULONG   	    	     refcount;

    OOP_Class 	    	    *gfxclass;
    OOP_Class 	    	    *onbmclass;
    OOP_Class 	    	    *offbmclass;
    OOP_Class 	    	    *mouseclass;
    OOP_Class 	    	    *kbdclass;
    
    OOP_Object      	    *gfxhidd;
    OOP_Object      	    *mousehidd;
    OOP_Object      	    *kbdhidd;

    ULONG		     red_mask;
    ULONG		     green_mask;
    ULONG		     blue_mask;
    ULONG   	    	     red_shift;
    ULONG   	    	     green_shift;
    ULONG   	    	     blue_shift;
    ULONG   	    	     depth; /* Size of pixel in bits */
    
/*  ULONG   	    	     bytes_per_pixel;
    ULONG   	    	     clut_shift;
    ULONG   	    	     clut_mask;

    Atom    	    	     delete_win_atom;
    Atom    	    	     clipboard_atom;
    Atom    	    	     clipboard_property_atom;
    Atom    	    	     clipboard_incr_atom;
    Atom    	    	     clipboard_targets_atom;

    Time    	    	     x_time;
#if 0
    VOID	    	     (*activecallback)(APTR, OOP_Object *, BOOL);
    APTR	    	     callbackdata;
#endif    

    BOOL    	    	    fullscreen;

    struct MsgPort  	    *hostclipboardmp;
    struct Message  	    *hostclipboardmsg;
    ULONG   	    	     hostclipboard_readstate;
    unsigned char   	    *hostclipboard_incrbuffer;
    ULONG   	    	     hostclipboard_incrbuffer_size;
    unsigned char   	    *hostclipboard_writebuffer;
    ULONG   	    	     hostclipboard_writebuffer_size;
    Window    	    	     hostclipboard_writerequest_window;
    Atom    	    	     hostclipboard_writerequest_property;
    ULONG   	    	     hostclipboard_write_chunks;*/
};

struct gdiclbase
{
    struct Library        library;
    
    struct gdi_staticdata xsd;
};

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    APTR	 display;
    int		 depth;
/*  Cursor	 cursor;*/
    APTR	 fbwin; /* Frame buffer window */
};

#define HOSTCLIPBOARDSTATE_IDLE     	0
#define HOSTCLIPBOARDSTATE_READ     	1
#define HOSTCLIPBOARDSTATE_READ_INCR    2
#define HOSTCLIPBOARDSTATE_WRITE    	3
/*
VOID get_bitmap_info(struct gdi_staticdata *xsd, Drawable d, ULONG *sz, ULONG *bpl);

BOOL set_pixelformat(struct TagItem *pftags, struct gdi_staticdata *xsd, Drawable d);

ULONG gdiclipboard_init(struct gdi_staticdata *);
VOID  gdiclipboard_handle_commands(struct gdi_staticdata *);
BOOL  gdiclipboard_want_event(XEvent *);
VOID  gdiclipboard_handle_event(struct gdi_staticdata *, XEvent *);
*/
#undef XSD
#define XSD(cl)     	(&((struct gdiclbase *)cl->UserData)->xsd)

/* This lock has two uses:
- Making GDI calls threadsafe.
- In the bitmap class, protecting the bimtap GC from changes from other tasks
*/

#define LOCK_GDI ObtainSemaphore (&XSD(cl)->gdisema);
#define UNLOCK_GDI ReleaseSemaphore(&XSD(cl)->gdisema);

#define SRCCOPY 0x00CC0020

#else

#include <windows.h>

#endif

#endif /* HIDD_GDI_H */
