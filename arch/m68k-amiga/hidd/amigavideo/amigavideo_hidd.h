#ifndef AMIGAVIDEO_HIDD_H
#define AMIGAVIDEO_HIDD_H

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <graphics/gfxbase.h>
#include <graphics/copper.h>

#include "amigavideo_intern.h"

/* Private instance data for Gfx hidd class */
struct amigagfx_data
{
    struct MinList              bitmaps;		/* Currently shown bitmap objects       */
    OOP_Object                  *compositor;
#if USE_FAST_BMSTACKCHANGE
    OOP_MethodFunc         bmstackchange;
    OOP_Class 	          *bmstackchange_Class;
#endif
};

void AmigaVideo_ParseCopperlist(struct amigavideo_staticdata *, struct amigabm_data *, struct CopList  *);

#endif /* AMIGAVIDEO_HIDD_H */
