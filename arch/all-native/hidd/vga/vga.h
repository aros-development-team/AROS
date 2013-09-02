#ifndef HIDD_VGA_H
#define HIDD_VGA_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

/***** VGA gfx HIDD *******************/

/* IDs */
#define IID_Hidd_VGAgfx		"hidd.gfx.vga"
#define CLID_Hidd_VGAgfx	"hidd.gfx.vga"

/* misc */

struct vga_staticdata
{
    OOP_Class 	*vgaclass;
    OOP_Class 	*bmclass;
    OOP_Object 	*vgahidd;
    struct SignalSemaphore	sema;	/* Protecting this whole struct */
    struct List		modelist;	/* List of modes supported */

    /* The following should be object data, not class data! */
    struct SignalSemaphore	HW_acc;	/* Exclusive hardware use */
    OOP_Object			*visible;	/* Points to visible bitmap */

    LONG		mouseX;		/* Pointer X position on screen */
    ULONG		mouseW;		/* Pointer width */
    LONG		mouseY;		/* Pointer Y position on screen */
    ULONG		mouseH;		/* Pointer height */
    ULONG		mouseVisible;	/* Is pointer visible flag */
    UBYTE		*mouseShape;	/* Points to pointer shape */
    UBYTE		mouseBase;	/* Pointer base color	   */
};

struct vgabase
{
    struct Library library;

    struct vga_staticdata vsd;
};

void draw_mouse (struct vga_staticdata *);
void erase_mouse (struct vga_staticdata *);
int vgaBlankScreen(int on);

#define XSD(cl) (&((struct vgabase *)cl->UserData)->vsd)

#endif /* HIDD_VGA_H */
