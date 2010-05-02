/*
 * intelG45_lowlevel.c
 *
 *  Created on: May 2, 2010
 *      Author: misc
 */

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <utility/tagitem.h>

#include <hidd/graphics.h>
#include <hidd/i2c.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdint.h>



#include LC_LIBDEFS_FILE

#include "intelG45_intern.h"
#include "intelG45_regs.h"

void G45_InitMode(struct g45staticdata *sd, GMAState_t *state,
		uint16_t width, uint16_t height, uint8_t depth, uint32_t pixelclock, intptr_t framebuffer,
        uint16_t hdisp, uint16_t vdisp, uint16_t hstart, uint16_t hend, uint16_t htotal,
        uint16_t vstart, uint16_t vend, uint16_t vtotal, uint32_t flags)
{
	D(bug("[GMA] InitMode\n"));


}

void G45_LoadState(struct g45staticdata *sd, GMAState_t *state)
{

}

void G45_SaveState(struct g45staticdata *sd, GMAState_t *state)
{

}
