/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#if !defined(KRN_Dummy)
#define KRN_Dummy               (TAG_USER + 0x03d00000)
#endif
#if !defined(KRN_FuncPutC)
#define KRN_FuncPutC      (KRN_Dummy + 99) /* RAW FrameBuffer descriptor */
#endif

#include <inttypes.h>

/* Linear framebuffer accessor (kernel_fb.c). Geometry is filled in from the
   boot taglist by kernel_startup and read back by the graphics HIDD. */
void krn_fb_set(uint64_t base, uint32_t w, uint32_t h, uint32_t depth, uint32_t pitch);
int krn_fb_init(unsigned int w, unsigned int h);
unsigned int krn_fb_width(void);
unsigned int krn_fb_height(void);
unsigned int krn_fb_depth(void);
unsigned int krn_fb_pitch(void);
unsigned long long krn_fb_base(void);
