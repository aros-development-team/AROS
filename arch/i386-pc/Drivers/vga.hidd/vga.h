#ifndef HIDD_VGA_H
#define HIDD_VGA_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the vga gfx HIDD.
    Lang: English.
*/


#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#include "bitmap.h"

/***** Mouse HIDD *******************/

/* IDs */
#define IID_Hidd_HwMouse	"hidd.mouse.hw"
#define CLID_Hidd_HwMouse	"hidd.mouse.hw"

/* Methods */
enum
{
    moHidd_Mouse_HandleEvent
};

struct pHidd_Mouse_HandleEvent
{
    OOP_MethodID mID;
    ULONG event;
};

VOID Hidd_Mouse_HandleEvent(OOP_Object *o, ULONG event);

/***** VGA gfx HIDD *******************/

/* IDs */
#define IID_Hidd_VGAgfx		"hidd.gfx.vga"
#define CLID_Hidd_VGAgfx	"hidd.gfx.vga"

struct MouseShape {
    ULONG width;
    ULONG height;
    ULONG bpp;
    UBYTE *data;
};

/* misc */

struct vga_staticdata
{
    struct SignalSemaphore	sema;	/* Protecting this whole struct */
    struct SignalSemaphore	HW_acc;	/* Exclusive hardware use */
    struct List		modelist;	/* List of modes supported */
    struct bitmap_data	*visible;	/* Point to visible bitmap */

    OOP_Class 	*vgaclass;
    OOP_Class 	*onbmclass;
    OOP_Class 	*offbmclass;
#if 0
    OOP_Class 	*mouseclass;
#endif
    OOP_Object 	*vgahidd;
#if 0
    OOP_Object 	*mousehidd;
#endif
    LONG		mouseX;		/* Pointer X position on screen */
    ULONG		mouseW;		/* Pointer width */
    LONG		mouseY;		/* Pointer Y position on screen */
    ULONG		mouseH;		/* Pointer height */
    ULONG		mouseVisible;	/* Is pointer visible flag */
    UBYTE		*mouseShape;	/* Points to pointer shape */

    VOID	(*activecallback)(APTR, OOP_Object *, BOOL);
    APTR	callbackdata;
};

struct vgabase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
    
    struct vga_staticdata vsd;
};

#if 0
    /* nlorentz: This function is no lonfger necessary	*/
BOOL set_pixelformat(OOP_Object *);

#endif

void draw_mouse (struct vga_staticdata *);

#define XSD(cl) (&((struct vgabase *)cl->UserData)->vsd)

#endif /* HIDD_VGA_H */
