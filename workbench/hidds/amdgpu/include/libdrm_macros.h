/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LIBDRM_MACROS_H_
#define _LIBDRM_MACROS_H_
#define drm_private
#define drm_public

static inline int getpagesize(void)
{
    return 4096;
}
#endif
