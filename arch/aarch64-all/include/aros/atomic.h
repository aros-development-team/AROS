/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.
 */

#ifndef AROS_AARCH64_ATOMIC_H
#define AROS_AARCH64_ATOMIC_H

/* __ARM_ARCH_8A__ is AArch32-only: guarding on it left every AArch64
 * target without asm atomics. */
#ifdef __aarch64__
#include <aros/aarch64/atomic_v8.h>
#endif

#endif /* AROS_AARCH64_ATOMIC_H */
