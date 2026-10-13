/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_FPU_H_
#define _LINUX_FPU_H_

static inline void kernel_fpu_begin(void) { }
static inline void kernel_fpu_end(void) { }
static inline bool kernel_fpu_available(void) { return true; }

#endif
