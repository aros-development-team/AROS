#ifndef HIDD_VGA_H
#define HIDD_VGA_H

/*
    (C) 1999 AROS - The Amiga Research OS
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

struct abdescr
{
    STRPTR interfaceid;
    AttrBase *attrbase;
};

struct vga_staticdata
{
    struct SignalSemaphore	sema;	/* Protecting this whole struct */
    struct SignalSemaphore	HW_acc;	/* Exclusive hardware use */
    struct Library	*oopbase;
    struct Library	*utilitybase;
    struct ExecBase	*sysbase;
    struct List		modelist;	/* List of modes supported */
    struct bitmap_data	*visible;	/* Point to visible bitmap */
    Class *vgaclass;
    Class *onbmclass;
    Class *offbmclass;
    Object *vgahidd;
};

BOOL obtainattrbases(struct abdescr *abd, struct Library *OOPBase);
VOID releaseattrbases(struct abdescr *abd, struct Library *OOPBase);

Class *init_vgaclass  ( struct vga_staticdata * );
Class *init_onbmclass  ( struct vga_staticdata * );
Class *init_offbmclass  ( struct vga_staticdata * );

VOID free_vgaclass  ( struct vga_staticdata * );
VOID free_onbmclass  ( struct vga_staticdata * );
VOID free_offbmclass  ( struct vga_staticdata * );

#define XSD(cl) ((struct vga_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)XSD(cl)->oopbase)
#define UtilityBase	((struct Library *)XSD(cl)->utilitybase)
#define SysBase		(XSD(cl)->sysbase)

#endif /* HIDD_VGA_H */
