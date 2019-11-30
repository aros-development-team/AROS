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
   struct List                 bitmapstack;

   struct SignalSemaphore      semaphore;

   OOP_Object                  *gfx;           /* GFX driver object */
};

#define LOCK_COMPOSITOR_READ       { ObtainSemaphoreShared(&compdata->semaphore); }
#define LOCK_COMPOSITOR_WRITE      { ObtainSemaphore(&compdata->semaphore); }
#define UNLOCK_COMPOSITOR          { ReleaseSemaphore(&compdata->semaphore); }

#endif /* AMIGAVIDEO_COMPOSITOR_H */
