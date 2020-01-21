#ifndef HIDD_X11_H
#define HIDD_X11_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the x11 HIDD.
    Lang: English.
*/

#include <aros/config.h>

#include <oop/oop.h>
#include <proto/exec.h>
#include <hidd/keyboard.h>

#include "x11_class.h"

#ifndef X11_TYPES_H
/* Note: x11_types.h is not included intentionally to resolve compilation
 * issues on linux-armhf where certain definitions collide between AROS
 * and Linux.
 * In every source file x11_types.h needs to be included before x11.h
 */
#include <X11/X.h>                  // Simple types definitions
typedef struct _XEvent XEvent;      // Used only as pointer
typedef struct _Display Display;    // Used only as pointer
typedef APTR GC;                    // GC is a pointer type
typedef struct _XVisualInfo XVisualInfo; // Used only as pointer
#endif

/****************************************************************************************/

#define USE_X11_DRAWFUNCS   1
#define X11SOFTMOUSE        0   /* Use software mouse sprite */
#define ADJUST_XWIN_SIZE    1   /* Resize the xwindow to the size of the actual visible screen */

/****************************************************************************************/

/***** X11Mouse HIDD *******************/

/* Private data */
struct pHidd_Mouse_Event;
struct x11mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};

/* IDs */
#define IID_Hidd_Mouse_X11	"hidd.mouse.x11"


/* Methods */
enum
{
    moHidd_Mouse_X11_HandleEvent
};

struct pHidd_Mouse_X11_HandleEvent
{
    OOP_MethodID mID;
    XEvent *event;
};

VOID Hidd_Mouse_X11_HandleEvent(OOP_Object *o, XEvent *event);

/***** X11Kbd HIDD *******************/

/* Private data */
struct x11kbd_data
{
    KbdIrqCallBack_t  kbd_callback;
    APTR    callbackdata;
    UWORD   prev_keycode;
};

/* IDs */
#define IID_Hidd_Kbd_X11		"hidd.kbd.x11"

/* Methods */
enum
{
    moHidd_Kbd_X11_HandleEvent
};

struct pHidd_Kbd_X11_HandleEvent
{
    OOP_MethodID     mID;
    XEvent  	    *event;
};

VOID Hidd_Kbd_X11_HandleEvent(OOP_Object *o, XEvent *event);

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
    Window	     masterxwindow;
    OOP_Object	    *bmobj; 
    BOOL    	     window_mapped;
};

/* Messages used for sending info to the HIDD's task */

enum
{
        NOTY_MAPWINDOW,
        NOTY_WINCREATE,
        NOTY_WINDISPOSE,
        NOTY_RESIZEWINDOW,
        NOTY_NEWCURSOR
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

struct XKeyTableData
{
    UBYTE	   	        keycode2rawkey[256];
    BOOL                        havetable;    
};

struct x11_staticdata
{
    /*
     * The first two members MUST be in the beginning of this structure
     * because they are exposed to disk-based part (see x11_class.h)
     */
    struct XKeyTableData        *xtd;
    OOP_Class 	    	        *gfxclass;

    OOP_Class 	    	        *bmclass;
    OOP_Class 	    	        *mouseclass;
    OOP_Class 	    	        *kbdclass;

    struct SignalSemaphore      sema; /* Protecting this whole struct */
    
    /* This port is used for asking the x11 task for notifications
       on when some event occurs, for example MapNotify
    */
    struct MsgPort  	        *x11task_notify_port;
    
    Display 	    	        *display;
    BOOL    	    	        local_display;
    
    ULONG   	    	        refcount;
    
    OOP_Object      	        *gfxhidd;
    OOP_Object      	        *mousehidd;
    OOP_Object      	        *kbdhidd;

#if USE_XSHM
    struct SignalSemaphore      shm_sema;	/* singlethread access to shared mem */
    BOOL    	    	        use_xshm;	/* May we use Xshm? */
    void    	    	        *xshm_info;
#endif    
    
    /* This window is used as a friend drawable for pixmaps. The window is
       never mapped, i.e. it is never shown onscreen.
    */
    Window  	    	        dummy_window_for_creating_pixmaps;
    
    XVisualInfo     	        *vi;
    ULONG   	    	        red_shift;
    ULONG   	    	        green_shift;
    ULONG   	    	        blue_shift;

    ULONG   	    	        depth; /* Size of pixel in bits */ /* stegerg: was called "size" */
    ULONG   	    	        bytes_per_pixel;
    
    ULONG   	    	        clut_shift;
    ULONG   	    	        clut_mask;
    
    Atom    	    	        delete_win_atom;
    Atom    	    	        clipboard_atom;
    Atom    	    	        clipboard_property_atom;
    Atom    	    	        clipboard_incr_atom;
    Atom    	    	        clipboard_targets_atom;
    Time    	    	        x_time;

    VOID	    	        (*activecallback)(APTR, OOP_Object *);
    APTR	    	        callbackdata;

    ULONG                       options;

    struct MsgPort  	        *hostclipboardmp;
    struct Message  	        *hostclipboardmsg;
    ULONG   	    	        hostclipboard_readstate;
    unsigned char   	        *hostclipboard_incrbuffer;
    ULONG   	    	        hostclipboard_incrbuffer_size;
    unsigned char   	        *hostclipboard_writebuffer;
    ULONG   	    	        hostclipboard_writebuffer_size;
    Window    	    	        hostclipboard_writerequest_window;
    Atom    	    	        hostclipboard_writerequest_property;
    ULONG   	    	        hostclipboard_write_chunks;
};

#define OPTION_FULLSCREEN       (1 << 0)
#define OPTION_BACKINGSTORE     (1 << 1)
#define OPTION_FORCESTDMODES    (1 << 2)
#define OPTION_DELAYXWINMAPPING (1 << 3)

/* Send the message and wait for the reply */
static inline void X11DoNotify(struct x11_staticdata *xsd, struct notify_msg *msg)
{
    PutMsg(xsd->x11task_notify_port, &msg->execmsg);
    WaitPort(msg->execmsg.mn_ReplyPort);
    GetMsg(msg->execmsg.mn_ReplyPort);
}

struct x11clbase
{
    struct Library        library;
    
    struct x11_staticdata xsd;
};

/* Private Attrs and methods for the X11Gfx Hidd */

#define CLID_Hidd_Gfx_X11   "hidd.gfx.x11"
#define IID_Hidd_Gfx_X11    "hidd.gfx.x11"

#define PEN_BITS    4
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    Display	*display;
    int		 screen;
    int		 depth;
    Colormap	 colmap;
    Cursor	 cursor;

    /* baseclass for CreateObject */
    OOP_Class *basebm;
};

#define HOSTCLIPBOARDSTATE_IDLE     	0
#define HOSTCLIPBOARDSTATE_READ     	1
#define HOSTCLIPBOARDSTATE_READ_INCR    2
#define HOSTCLIPBOARDSTATE_WRITE    	3

VOID get_bitmap_info(struct x11_staticdata *xsd, Drawable d, ULONG *sz, ULONG *bpl);

BOOL set_pixelformat(struct TagItem *pftags, struct x11_staticdata *xsd, Drawable d);


OOP_Class *init_gfxclass	( struct x11_staticdata * );
OOP_Class *init_onbmclass	( struct x11_staticdata * );
OOP_Class *init_offbmclass	( struct x11_staticdata * );
OOP_Class *init_kbdclass  	( struct x11_staticdata * );
OOP_Class *init_mouseclass	( struct x11_staticdata * );

VOID free_gfxclass	( struct x11_staticdata * );
VOID free_onbmclass	( struct x11_staticdata * );
VOID free_offbmclass	( struct x11_staticdata * );
VOID free_osbmclass	( struct x11_staticdata * );
VOID free_kbdclass	( struct x11_staticdata * );
VOID free_mouseclass	( struct x11_staticdata * );

ULONG x11clipboard_init(struct x11_staticdata *);
VOID  x11clipboard_handle_commands(struct x11_staticdata *);
BOOL  x11clipboard_want_event(XEvent *);
VOID  x11clipboard_handle_event(struct x11_staticdata *, XEvent *);

int X11_Init(struct x11_staticdata *xsd);

#undef XSD
#define XSD(cl)     	(&((struct x11clbase *)cl->UserData)->xsd)

/*
 * This lock has two uses:
 * - Making X calls threadsafe.
 * - In the bitmap class, protecting the bitmap X GC from changes from other tasks
 * Since X makes intensive use of malloc(), and can interfere not only with other X calls,
 * but with all other host OS calls, we use global lock provided by hostlib.resource.
 */

#define LOCK_X11   HostLib_Lock();
#define UNLOCK_X11 HostLib_Unlock();

#endif /* HIDD_X11_H */
