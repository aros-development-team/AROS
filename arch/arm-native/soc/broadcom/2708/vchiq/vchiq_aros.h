/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef VCHIQ_AROS_H
#define VCHIQ_AROS_H

#include <exec/libraries.h>

struct VCHIQBase {
    struct Library vb_Lib;
};

#define VCHIQ_THREAD_NAMELEN    16
#define VCHIQ_THREAD_STACK      (16 * 1024)
#define VCHIQ_THREAD_PRI        5

/* vchiq_aros_platform.c */
int vchiq_aros_platform_init(void);

#endif /* VCHIQ_AROS_H */
