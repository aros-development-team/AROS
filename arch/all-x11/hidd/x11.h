#ifndef HIDD_X11_H
#define HIDD_X11_H

/*
    (C) 1997 AROS - The Amiga Research OS
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

#ifndef _XLIB_H_
#   include <X11/Xlib.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

/****** X11 hidd  *****************************/

#define HiddX11AB __abHidd_X11Gfx

#define IID_Hidd_X11 "hidd.misc.x11"

extern AttrBase HiddX11AB;

enum {
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
    MethodID mID;
    XEvent *event;
};

VOID Hidd_X11Mouse_HandleEvent(Object *o, XEvent *event);

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
    MethodID mID;
    XEvent *event;
};

VOID Hidd_X11Kbd_HandleEvent(Object *o, XEvent *event);
/* misc */

struct abdescr
{
    STRPTR interfaceid;
    AttrBase *attrbase;
};


BOOL obtainattrbases(struct abdescr *abd, struct Library *OOPBase);
VOID releaseattrbases(struct abdescr *abd, struct Library *OOPBase);


struct x11task_params
{
    struct Task *parent;
    ULONG ok_signal;
    ULONG fail_signal;
    struct x11_staticdata *xsd;
};

struct xwinnode
{
    struct MinNode node;
    Window	xwindow;
};



struct x11_staticdata
{
    struct SignalSemaphore sema; /* Protexting this whole struct */
    struct SignalSemaphore x11sema;
    
    struct Library *oopbase;
    struct Library *utilitybase;
    struct ExecBase *sysbase;
    
    Display *display;
    
    ULONG refcount;

    Class *x11class;    
    Class *gfxclass;
    Class *onbmclass;
    Class *offbmclass;
    Class *mouseclass;
    Class *kbdclass;
    
    Object *gfxhidd;
    Object *mousehidd;
    Object *kbdhidd;
    
    struct MinList xwindowlist;
    struct SignalSemaphore winlistsema;
    
};

Class *init_gfxclass	( struct x11_staticdata * );
Class *init_onbmclass	( struct x11_staticdata * );
Class *init_offbmclass	( struct x11_staticdata * );
Class *init_kbdclass  	( struct x11_staticdata * );
Class *init_mouseclass	( struct x11_staticdata * );
Class *init_x11class	( struct x11_staticdata * );

VOID free_gfxclass	( struct x11_staticdata * );
VOID free_onbmclass	( struct x11_staticdata * );
VOID free_offbmclass	( struct x11_staticdata * );
VOID free_osbmclass	( struct x11_staticdata * );
VOID free_kbdclass	( struct x11_staticdata * );
VOID free_mouseclass	( struct x11_staticdata * );
VOID free_x11class	( struct x11_staticdata * );


#define XSD(cl) ((struct x11_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)XSD(cl)->oopbase)
#define UtilityBase	((struct Library *)XSD(cl)->utilitybase)
#define SysBase		(XSD(cl)->sysbase)



#define LX11 ObtainSemaphore (&XSD(cl)->x11sema);
#define UX11 ReleaseSemaphore(&XSD(cl)->x11sema);

#endif /* HIDD_X11_H */
