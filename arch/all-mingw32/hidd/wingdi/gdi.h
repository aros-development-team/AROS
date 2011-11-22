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
    unsigned char  GfxIrq;	/* IRQ number */
    unsigned char  ShowDone;	/* NOTY_SHOW completion flag */
    void	   *Active;	/* Set to struct gfx_data * when new display window is activated */

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
    /*
     * These two members should be in the beginning because it's exposed
     * outside (see gdi_class.h)
     */
    ULONG		     displaynum;
    OOP_Class 	    	    *gfxclass;

    OOP_Class 	    	    *bmclass;
    OOP_Class 	    	    *mouseclass;
    OOP_Class 	    	    *kbdclass;

    struct SignalSemaphore   sema;
    struct Task		    *showtask;
    void		    *gfx_int;
    
    OOP_Object      	    *mousehidd;
    OOP_Object      	    *kbdhidd;

    struct GDI_Control	    *ctl;
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

#ifdef __x86_64__
#define __aros __attribute__((sysv_abi))
#else
#define __aros
#endif

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
    struct MinList bitmaps;		/* Currently shown bitmap objects       */
    void *display;			/* Windows system display object        */
    void *cursor;			/* Windows mouse cursor object          */
    void *fbwin;			/* Display window		        */
    void (*cb)(void *data, void *bm);	/* Display activation callback function */
    void *cbdata;			/* User data for activation callback    */
};

#endif /* HIDD_GDI_H */
