#ifndef HIDD_DISPLAY_H
#define HIDD_DISPLAY_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

/***** Display gfx HIDD *******************/

/* IDs */
#define IID_Hidd_Displaygfx	"hidd.gfx.display"
#define CLID_Hidd_Displaygfx	"hidd.gfx.display"

/* misc */

struct display_staticdata
{
    struct SignalSemaphore	sema;	/* Protecting this whole struct */
    struct SignalSemaphore	HW_acc;	/* Exclusive hardware use */
    struct Library	*oopbase;
    struct Library	*utilitybase;
    struct ExecBase	*sysbase;
    struct displaybase  *displaybase;
    struct List		modelist;	/* List of modes supported */
    struct bitmap_data	*visible;	/* Point to visible bitmap */

    OOP_Class 	*displayclass;
    OOP_Class 	*onbmclass;
    OOP_Class 	*offbmclass;
    OOP_Object 	*displayhidd;

    VOID	(*activecallback)(APTR, OOP_Object *, BOOL);
    APTR	callbackdata;
    
    OOP_AttrBase  hiddPixFmtAttrBase;
    OOP_AttrBase  hiddBitMapAttrBase;
    OOP_AttrBase  hiddDisplayBitMapAB;
    OOP_AttrBase  hiddDisplayAB;
    OOP_AttrBase  hiddSyncAttrBase;
    OOP_AttrBase  hiddGfxAttrBase;
};

#define __IHidd_PixFmt	(xsd->hiddPixFmtAttrBase)
#define __IHidd_BitMap  (xsd->hiddBitMapAttrBase)
#define __IHidd_DisplayBitMap (xsd->hiddDisplayBitMapAB)
#define __IHidd_DisplayGfx (xsd->hiddDisplayAB)
#define __IHidd_Sync       (xsd->hiddSyncAttrBase)
#define __IHidd_Gfx        (xsd->hiddGfxAttrBase)

#define HiddDisplayBitMapAB  (xsd->hiddDisplayBitMapAB)

OOP_Class *init_displayclass  ( struct display_staticdata * );
OOP_Class *init_onbmclass     ( struct display_staticdata * );
OOP_Class *init_offbmclass    ( struct display_staticdata * );

VOID free_displayclass  ( struct display_staticdata * );
VOID free_onbmclass     ( struct display_staticdata * );
VOID free_offbmclass    ( struct display_staticdata * );

#define XSD(cl) ((struct display_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)XSD(cl)->oopbase)
#define UtilityBase	((struct Library *)XSD(cl)->utilitybase)
#define SysBase		(XSD(cl)->sysbase)

#endif /* HIDD_DISPLAY_H */
