/* SPDX-License-Identifier: MIT */
/* HarfBuzz on AROS with its thread safety on (no HB_NO_MT): included by
 * hb-config.hh through HB_CONFIG_OVERRIDE_H. Reference counts and lazy
 * loaders use GCC's __atomic builtins (hb-atomic.hh, no library needed on
 * x86_64, i386, ARM and AArch64); the few locks HarfBuzz takes (user data,
 * shape-plan caches, face tables) are exec SignalSemaphores, which work
 * across tasks and CPUs. One object (hb_font_t, hb_buffer_t) is still used
 * by one task at a time, as everywhere in HarfBuzz. */
#ifndef HB_AROS_CONFIG_H
#define HB_AROS_CONFIG_H
#include <exec/semaphores.h>
#include <proto/exec.h>

typedef struct SignalSemaphore hb_mutex_impl_t;
#define hb_mutex_impl_init(M)   InitSemaphore (M)
#define hb_mutex_impl_lock(M)   ObtainSemaphore (M)
#define hb_mutex_impl_unlock(M) ReleaseSemaphore (M)
#define hb_mutex_impl_finish(M) HB_STMT_START {} HB_STMT_END

#endif
