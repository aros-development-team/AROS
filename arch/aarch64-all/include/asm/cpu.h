/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: AArch64 CPU barrier and hint instructions
*/

#ifndef ASM_AARCH64_CPU_H
#define ASM_AARCH64_CPU_H

static inline void isb() { asm volatile("isb" : : : "memory"); }
static inline void dsb() { asm volatile("dsb sy" : : : "memory"); }
static inline void dmb() { asm volatile("dmb sy" : : : "memory"); }
static inline void sev() { asm volatile("sev"); }
static inline void wfe() { asm volatile("wfe"); }

#endif /* ASM_AARCH64_CPU_H */
