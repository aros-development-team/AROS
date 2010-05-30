/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: arosmesa_winsys.c 32274 2010-01-03 10:40:42Z deadwood $
*/


#include "arosmesa_winsys.h"

extern struct arosmesa_driver arosmesa_softpipe_driver;
extern struct arosmesa_driver arosmesa_nouveau_driver;
extern struct arosmesa_driver arosmesa_intel_driver;

struct arosmesa_driver * current_driver = NULL;

struct arosmesa_driver * arosmesa_get_driver( void )
{
    return current_driver;
}




/* FIXME: This code should not be here - it should be in "parent" hidd that
 * will select available 3D hidd */

/* THIS CODE IS A HACK */

#include <aros/symbolsets.h>

/* HACK definition to satisfy compiler */
void nouveau_exit(void);
int nouveau_init(void);
void i915_exit(void);
int i915_init(void);

static int
initialize_driver(void)
{
    if (current_driver == NULL)
    {
        /* HACK: goes around the whole libdrm call chain and calls directly into
         * "kernel" driver */

        /* Try using i915 */
        if (i915_init() == 0)
        {
            current_driver = &arosmesa_intel_driver;
        }
    }

    if (current_driver == NULL)
    {
        /* HACK: goes around the whole libdrm call chain and calls directly into
         * "kernel" driver */

        /* Try using nouveau */
        if (nouveau_init() == 0)
        {
            current_driver = &arosmesa_nouveau_driver;
        }
    }

    if (current_driver == NULL)
    {
        /* Use softpipe fallback */
        current_driver = &arosmesa_softpipe_driver;
    }
    
    return 1;
}

static int
deinitialize_driver(void)
{
    /* HACK: goes around the whole libdrm call chain and calls directly into
     * "kernel" driver */
    
    if (current_driver == &arosmesa_nouveau_driver)
    {
        nouveau_exit();
    }
    
    if (current_driver == &arosmesa_intel_driver)
    {
        i915_exit();
    }
    
    return 1;
}

ADD2INIT(initialize_driver, 5);
ADD2EXIT(deinitialize_driver, 5);
