#ifndef HIDD_GDI_H
#define HIDD_GDI_H

/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the gdi HIDD.
    Lang: English.
*/

#ifndef WM_USER
#define WM_USER 1024
#endif

/* GDI virtual hardware registers */
struct GDI_Control
{
    /* Display */
    unsigned char  GfxIrq;

    /* Mouse */
    unsigned char  MouseIrq;
    unsigned short MouseEvent;
    unsigned short MouseX;
    unsigned short MouseY;
    unsigned short Buttons;
    unsigned short WheelDelta;

    /* Keyboard */
    unsigned char  KbdIrq;
    unsigned short KbdEvent;
    unsigned short KeyCode;
};

#ifdef __AROS__

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>

#include "winapi.h"
#include "gdi_class.h"
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

struct gdi_staticdata
{
    struct SignalSemaphore   sema; /* Protecting this whole struct */

    OOP_Class 	    	    *gfxclass;
    OOP_Class 	    	    *bmclass;
    OOP_Class 	    	    *mouseclass;
    OOP_Class 	    	    *kbdclass;
    
    OOP_Object      	    *mousehidd;
    OOP_Object      	    *kbdhidd;

    struct GDI_Control	    *ctl;
    
    ULONG		     displaynum;
};

struct gdiclbase
{
    struct Library        library;
    
    struct gdi_staticdata xsd;
};

#undef XSD
#define XSD(cl)     	(&((struct gdiclbase *)cl->UserData)->xsd)

#else

#include <windows.h>
#define APTR void *

struct MinNode
{
    struct MinNode * mln_Succ,
		   * mln_Pred;
};

struct MinList
{
    struct MinNode * mlh_Head,
                   * mlh_Tail,
		   * mlh_TailPred;
};

#endif

#define NOTY_SHOW WM_USER

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    struct MinList bitmaps;	/* Currently shown bitmap objects */
    void *display;		/* Windows system display object  */
    void *cursor;		/* Windows mouse cursor object    */
    void *fbwin;		/* Display window		  */
};

#ifdef __AROS__

void GfxIntHandler(struct gfx_data *data, struct Task *task);

#endif

#endif /* HIDD_GDI_H */
