/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GRAPHICS_HIDD_INTERN_H
#define GRAPHICS_HIDD_INTERN_H

/* Include files */

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif
#include <dos/dos.h>
#include <intuition/screens.h>


struct HIDDGraphicsAmigaIntuiData
{
    Class *bitMapClass;  /* bitmap class     */
    Class *gcClass;      /* graphics context */
};


struct HIDDBitMapAmigaIntuiData
{
    struct Screen *screen;
};

struct class_static_data
{
    struct ExecBase      * sysbase;
    struct Library       * utilitybase;
    struct Library       * oopbase;
    struct Library       * intuitionbase;
    struct Library       * superclassbase;   /* graphics hidd superclass */

    Class                *gfxhiddclass; /* graphics hidd class      */
    Class                *bitmapclass;  /* bitmap class             */
    Class                *gcclass;      /* graphics context class   */
};


/* Library base */

struct IntHIDDGraphicsAmigaIntuiBase
{
    struct Library            hdg_LibNode;
    struct ExecBase          *hdg_SysBase;
    struct Library           *hdg_UtilityBase;
    BPTR                      hdg_SegList;

    struct class_static_data *hdg_csd;
};


#define CSD(x) ((struct class_static_data *)x)

#undef SysBase
#define SysBase (CSD(cl->UserData)->sysbase)

#undef UtilityBase
#define UtilityBase (CSD(cl->UserData)->utilitybase)

#undef OOPBase
#define OOPBase (CSD(cl->UserData)->oopbase)

#undef IntuitionBase
#define IntuitionBase (CSD(cl->UserData)->intuitionbase)

/* pre declarations */

Class *init_gfxhiddclass(struct class_static_data *csd);
void   free_gfxhiddclass(struct class_static_data *csd);

Class *init_bitmapclass(struct class_static_data *csd);
void   free_bitmapclass(struct class_static_data *csd);

#endif /* GRAPHICS_HIDD_INTERN_H */
