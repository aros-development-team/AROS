#ifndef HIDD_X11_H
#define HIDD_X11_H

/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the x11 HIDD.
    Lang: English.
*/


#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#define timeval sys_timeval
#ifndef _XLIB_H_
#   include <X11/Xlib.h>
#endif

#ifndef _XUTIL_H
#   include <X11/Xutil.h>
#endif
#undef timeval

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#include "xshm.h"


#define X11_LOAD_KEYMAPTABLE	    1

/****** X11 hidd  *****************************/

#define HiddX11AB __abHidd_X11Gfx

#define IID_Hidd_X11 "hidd.misc.x11"

extern OOP_AttrBase HiddX11AB;

enum
{
   aoHidd_X11_SysDisplay,
   
   num_Hidd_X11_Attrs
};

#define aHidd_X11_SysDisplay	(aoHidd_X11_SysDisplay + HiddX11AB)

/***** X11Mouse HIDD *******************/

/* IDs */
#define IID_Hidd_X11Mouse	"hidd.mouse.x11"
#define CLID_Hidd_X11Mouse	"hidd.mouse.x11"


/* Methods */
enum
{
    moHidd_X11Mouse_HandleEvent
};

struct pHidd_X11Mouse_HandleEvent
{
    OOP_MethodID mID;
    XEvent *event;
};

VOID Hidd_X11Mouse_HandleEvent(OOP_Object *o, XEvent *event);

/***** X11Kbd HIDD *******************/

/* IDs */
#define IID_Hidd_X11Kbd		"hidd.kbd.x11"
#define CLID_Hidd_X11Kbd	"hidd.kbd.x11"

/* Methods */
enum
{
    moHidd_X11Kbd_HandleEvent
};

struct pHidd_X11Kbd_HandleEvent
{
    OOP_MethodID     mID;
    XEvent  	    *event;
};

VOID Hidd_X11Kbd_HandleEvent(OOP_Object *o, XEvent *event);
/* misc */




struct x11task_params
{
    struct Task     	    *parent;
    ULONG   	     	     ok_signal;
    ULONG   	     	     fail_signal;
    ULONG   	     	     kill_signal;
    struct x11_staticdata   *xsd;
};

struct xwinnode
{
    struct MinNode   node;
    Window	     xwindow;
    OOP_Object	    *bmobj; 
    BOOL    	     window_mapped;
};


/* Message used for getting info on when a window has been mapped */

enum
{
	NOTY_MAPWINDOW,
	NOTY_WINCREATE,
	NOTY_WINDISPOSE,
	NOTY_RESIZEWINDOW
};


struct notify_msg
{
     struct Message  execmsg;     
     ULONG  	     notify_type; /* NOTY_xxxx */
     Display 	    *xdisplay;     
     Window 	     xwindow;
     Window 	     masterxwindow;
     OOP_Object     *bmobj;     
     /* Only for NOTY_RESIZEWINDOW */
     ULONG  	     width;
     ULONG  	     height;
};


struct x11_staticdata
{
    struct SignalSemaphore   sema; /* Protecting this whole struct */
    struct SignalSemaphore   x11sema;
    
    /* This port is used for asking the x11 task for notifications
       on when some event occurs, for example MapNotify
    */
    struct MsgPort  	    *x11task_notify_port;
    
    struct Library  	    *oopbase;
    struct Library  	    *utilitybase;
    struct ExecBase 	    *sysbase;
    struct Library  	    *dosbase;
    
    Display 	    	    *display;
    BOOL    	    	     local_display;
    
    ULONG   	    	     refcount;

    OOP_Class 	    	    *x11class;    
    OOP_Class 	    	    *gfxclass;
    OOP_Class 	    	    *onbmclass;
    OOP_Class 	    	    *offbmclass;
    OOP_Class 	    	    *mouseclass;
    OOP_Class 	    	    *kbdclass;
    
    OOP_Object      	    *gfxhidd;
    OOP_Object      	    *mousehidd;
    OOP_Object      	    *kbdhidd;

#if USE_XSHM
    struct SignalSemaphore   shm_sema;	/* singlethread access to shared mem */
    BOOL    	    	     use_xshm;	/* May we use Xshm ?	*/
    void    	    	    *xshm_info;
#endif    
    
    /* This window is used as a frien drawable for pixmaps. The window is
       never mapped, ie. it is never shown onscreen.
    */
    Window  	    	     dummy_window_for_creating_pixmaps;
    
    XVisualInfo     	     vi;
    ULONG   	    	     red_shift;
    ULONG   	    	     green_shift;
    ULONG   	    	     blue_shift;

    ULONG   	    	     depth; /* Size of pixel in bits */ /* stegerg: was called "size" */
    ULONG   	    	     bytes_per_pixel;
    
    ULONG   	    	     clut_shift;
    ULONG   	    	     clut_mask;
    
    Atom    	    	     delete_win_atom;

#if 0
    VOID	    	     (*activecallback)(APTR, OOP_Object *, BOOL);
    APTR	    	     callbackdata;
#endif    
};


VOID get_bitmap_info(struct x11_staticdata *xsd, Drawable d, ULONG *sz, ULONG *bpl);

BOOL set_pixelformat(struct TagItem *pftags, struct x11_staticdata *xsd, Drawable d);


OOP_Class *init_gfxclass	( struct x11_staticdata * );
OOP_Class *init_onbmclass	( struct x11_staticdata * );
OOP_Class *init_offbmclass	( struct x11_staticdata * );
OOP_Class *init_kbdclass  	( struct x11_staticdata * );
OOP_Class *init_mouseclass	( struct x11_staticdata * );
OOP_Class *init_x11class	( struct x11_staticdata * );

VOID free_gfxclass	( struct x11_staticdata * );
VOID free_onbmclass	( struct x11_staticdata * );
VOID free_offbmclass	( struct x11_staticdata * );
VOID free_osbmclass	( struct x11_staticdata * );
VOID free_kbdclass	( struct x11_staticdata * );
VOID free_mouseclass	( struct x11_staticdata * );
VOID free_x11class	( struct x11_staticdata * );


#define XSD(cl)     	((struct x11_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)XSD(cl)->oopbase)
#define UtilityBase	((struct Library *)XSD(cl)->utilitybase)
#define SysBase		(XSD(cl)->sysbase)
#define DosBase		(XSD(cl)->dosbase)


/* This lock has two uses:
- Making X calls threadsafe.
- In the bitmap class, protecting the bimtap X GC from changes
from other tasks
*/

#define LX11 ObtainSemaphore (&XSD(cl)->x11sema);
#define UX11 ReleaseSemaphore(&XSD(cl)->x11sema);

#endif /* HIDD_X11_H */
