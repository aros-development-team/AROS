/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: Linear framebuffer accessor. The bootstrap allocates the VideoCore
          framebuffer and passes its geometry in the kernel boot taglist;
          kernel_startup records it here and the graphics HIDD (fbgfx) reads
          it back. The surface lives in the VideoCore region, which is
          identity mapped, so the physical base doubles as the CPU address.
*/

#include <inttypes.h>

#include "kernel_fb.h"

static uint64_t fb_base;
static uint32_t fb_w, fb_h, fb_pitch, fb_depth;

void krn_fb_set(uint64_t base, uint32_t w, uint32_t h, uint32_t depth, uint32_t pitch)
{
    fb_base  = base;
    fb_w     = w;
    fb_h     = h;
    fb_depth = depth;
    fb_pitch = pitch;
}

/* The bootstrap already brought the framebuffer up; just report availability. */
int krn_fb_init(unsigned int w, unsigned int h)
{
    (void)w; (void)h;
    return (fb_base != 0 && fb_pitch != 0);
}

unsigned int krn_fb_width(void)  { return fb_w; }
unsigned int krn_fb_height(void) { return fb_h; }
unsigned int krn_fb_depth(void)  { return fb_depth; }
unsigned int krn_fb_pitch(void)  { return fb_pitch; }
unsigned long long krn_fb_base(void) { return fb_base; }
