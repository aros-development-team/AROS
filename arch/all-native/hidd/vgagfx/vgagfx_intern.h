#ifndef VGAGFX_INTERN_H
#define VGAGFX_INTERN_H

/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private Includes for the VGA Gfx Hidd.
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

/* misc */

struct VGAGfx_staticdata
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

    /* baseclass for CreateObject */
    OOP_Class *basebm;

    OOP_AttrBase hiddAttrBase;
    OOP_AttrBase bitMapAttrBase;
    OOP_AttrBase chunkyBMAttrBase;
    OOP_AttrBase pixFmtAttrBase;
    OOP_AttrBase gfxAttrBase;
    OOP_AttrBase syncAttrBase;
    OOP_AttrBase vgaBitMapAttrBase;
};

#define XSD(cl) (&((struct VGAGfxBase *)cl->UserData)->vsd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddAttrBase
#undef HiddVGABitMapAB

/* These must stay in the same order as interfaces[] array in vesagfx_init.c */
#define HiddChunkyBMAttrBase	  csd->chunkyBMAttrBase
#define HiddBitMapAttrBase	  csd->bitMapAttrBase
#define HiddGfxAttrBase		  csd->gfxAttrBase
#define HiddPixFmtAttrBase	  csd->pixFmtAttrBase
#define HiddSyncAttrBase	  csd->syncAttrBase
#define HiddAttrBase		  csd->hiddAttrBase
#define HiddVGABitMapAB	  csd->vgaBitMapAttrBase
#define csd XSD(cl)

struct VGAGfxBase
{
    struct Library library;

    struct VGAGfx_staticdata vsd;
};

void draw_mouse (struct VGAGfx_staticdata *);
void erase_mouse (struct VGAGfx_staticdata *);
int vgaBlankScreen(int on);

#endif /* VGAGFX_INTERN_H */
