#ifndef HIDD_GDI_H
#define HIDD_GDI_H

/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id: gdi.h 27106 2007-10-28 10:49:03Z sonic $

    Desc: Include for the gdi HIDD.
    Lang: English.
*/

#ifndef WM_USER
#define WM_USER 1024
#endif

/* GDI virtual hardware registers */
struct Gfx_Control
{
    unsigned char IrqNum;
    void *cursor;
};

struct MouseData
{
    unsigned char  IrqNum;
    unsigned short EventCode;
    unsigned short MouseX;
    unsigned short MouseY;
    unsigned short Buttons;
    unsigned short WheelDelta;
};

struct KeyboardData
{
    unsigned char IrqNum;
    unsigned short EventCode;
    unsigned short KeyCode;
};

#ifdef __AROS__

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>

#include "winapi.h"
#include "gdi_hostlib.h"

/***** GDIMouse HIDD *******************/

/* Private data */
struct pHidd_Mouse_Event;
struct gdimouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
    void *interrupt;
    UWORD buttons;
};

/* IDs */
#define IID_Hidd_GDIMouse	"hidd.mouse.gdi"
#define CLID_Hidd_GDIMouse	"hidd.mouse.gdi"

/***** GDIKbd HIDD *******************/

/* Private data */
struct gdikbd_data
{
    VOID  (*kbd_callback)(APTR, UWORD);
    APTR    callbackdata;
    void *interrupt;
};

/* IDs */
#define IID_Hidd_GDIKbd		"hidd.kbd.gdi"
#define CLID_Hidd_GDIKbd	"hidd.kbd.gdi"


/***** GDIGfx HIDD *******************/

#define CLID_Hidd_GDIGfx	"hidd.gfx.gdi"

struct gdi_staticdata
{
    struct SignalSemaphore   sema; /* Protecting this whole struct */
     
    APTR 	    	     display;	  /* System display object */
    
    OOP_Class 	    	    *gfxclass;
    OOP_Class 	    	    *bmclass;
    OOP_Class 	    	    *mouseclass;
    OOP_Class 	    	    *kbdclass;
    
    OOP_Object      	    *gfxhidd;
    OOP_Object      	    *mousehidd;
    OOP_Object      	    *kbdhidd;

    struct Gfx_Control	    *ctl;
};

struct gdiclbase
{
    struct Library        library;
    
    struct gdi_staticdata xsd;
};

#undef XSD
#define XSD(cl)     	(&((struct gdiclbase *)cl->UserData)->xsd)

#if AROS_BIG_ENDIAN
#define Machine_ARGB32 vHidd_StdPixFmt_ARGB32
#define Machine_0RGB32 vHidd_StdPixFmt_0RGB32
#else
#define Machine_ARGB32 vHidd_StdPixFmt_BGRA32
#define Machine_0RGB32 vHidd_StdPixFmt_BGR032
#endif

#else

#include <windows.h>
#define APTR void *

#endif

#define NOTY_SHOW WM_USER

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    void *display;   /* Windows system display object  */
    void *cursor;    /* Windows mouse cursor object    */
    void *bitmap;    /* Currently shown bitmap object  */
    void *fbwin;     /* Display window		       */
};

#ifdef __AROS__

void GfxIntHandler(struct gfx_data *data, struct Task *task);

#endif

#endif /* HIDD_GDI_H */
