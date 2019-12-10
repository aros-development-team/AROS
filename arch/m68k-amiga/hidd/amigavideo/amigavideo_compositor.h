#ifndef AMIGAVIDEO_COMPOSITOR_H
#define AMIGAVIDEO_COMPOSITOR_H
/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "compositor.h"

#include <exec/lists.h>
#include <exec/semaphores.h>

struct amigacomposit_data
{
   struct List                 visbmstack;
   struct List                 obscuredbmstack;

   struct SignalSemaphore      semaphore;

   UWORD                        displaywidth;
   UWORD                        displayheight;

   OOP_Object                  *gfx;           /* GFX driver object */
   struct Task                 *housekeeper;
};

#define LOCK_COMPOSITOR_READ       { ObtainSemaphoreShared(&compdata->semaphore); }
#define LOCK_COMPOSITOR_WRITE      { ObtainSemaphore(&compdata->semaphore); }
#define UNLOCK_COMPOSITOR          { ReleaseSemaphore(&compdata->semaphore); }

#endif /* AMIGAVIDEO_COMPOSITOR_H */
