/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read the current vertical position of the beam
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"

#include <clib/graphics_protos.h>

/* See rom/graphics/vbeampos.c for documentation */

AROS_LH0(LONG, VBeamPos,
    struct GfxBase *, GfxBase, 64, Graphics)
{
    AROS_LIBFUNC_INIT

    volatile struct Custom *custom = (struct Custom*)0xdff000;
    UWORD v1, v2;
    
    for (;;) {
	v1 = custom->vhposr >> 8;
	v2 = custom->vposr;
	if (v1 == (custom->vhposr >> 8))
	    break;
    }

    return v1 | ((v2 & 7) << 8);

    AROS_LIBFUNC_EXIT
} /* VBeamPos */
