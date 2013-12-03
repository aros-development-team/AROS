/******************************************************************************
 *
 * Name: aclinux.h - OS specific defines, etc. for AROS
 *
 *****************************************************************************/

#ifndef __ACAROS_H__
#define __ACAROS_H__

#if defined(__i386__) || defined(__x86_64__)
#define ACPI_DIV_64_BY_32(n_hi, n_lo, d32, q32, r32) \
    asm volatile ( \
            "mov %2, %%edx\n" \
            "mov %3, %%eax\n" \
            "div %4\n" \
            "mov %%eax, %0\n" \
            "mov %%edx, %1\n" \
            : "=r" (q32), "=r" (r32) \
            : "r" (n_hi), "r" (n_lo), "r" (d32) \
            : "%eax", "%edx" );

#define ACPI_SHIFT_RIGHT_64(n_hi, n_lo) \
    asm volatile ( \
            "shr $1, %0\n" \
            "rcr $1, %1\n" \
            : "=r" (n_hi), "=r" (n_lo) \
            : "0" (n_hi), "1" (n_lo) \
            );
#endif

/* Common (in-kernel/user-space) ACPICA configuration */

#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_STANDARD_HEADERS
#define ACPI_MUTEX_TYPE             ACPI_BINARY_SEMAPHORE
#define ACPI_USE_LOCAL_CACHE

/*
 * Calling conventions:
 *
 * ACPI_SYSTEM_XFACE        - Interfaces to host OS (handlers, threads)
 * ACPI_EXTERNAL_XFACE      - External ACPI interfaces
 * ACPI_INTERNAL_XFACE      - Internal ACPI interfaces
 * ACPI_INTERNAL_VAR_XFACE  - Internal variable-parameter list interfaces
 */
#define ACPI_SYSTEM_XFACE           
#define ACPI_EXTERNAL_XFACE         
#define ACPI_INTERNAL_XFACE         
#define ACPI_INTERNAL_VAR_XFACE     

#include <stdint.h>
#include <proto/exec.h>

/* Host-dependent types and defines for AROS ACPICA */

#if defined(__x86_64__)
#define ACPI_MACHINE_WIDTH      64
#elif defined(__i386__)
#define ACPI_MACHINE_WIDTH      32
#else
#error Unsupported hardware architecture!
#endif

#define COMPILER_DEPENDENT_INT64    int64_t
#define COMPILER_DEPENDENT_UINT64   uint64_t

#ifndef __cdecl
#define __cdecl
#endif

#define ACPI_SPINLOCK   struct SignalSemaphore *
#define ACPI_SEMAPHORE  struct SignalSemaphore *
#define ACPI_UINTPTR_T  IPTR

#define ACPI_FLUSH_CPU_CACHE()  CacheClearU()
#define ACPI_ACQUIRE_GLOBAL_LOCK(facs,acq) \
     { ACPI_TABLE_FACS *_facs = facs; \
       BOOLEAN ok = FALSE; \
       Disable(); if (_facs->GlobalLock == 0) { _facs->GlobalLock=1; ok = TRUE; } Enable(); \
       acq = ok; }
#define ACPI_RELEASE_GLOBAL_LOCK(facs,pend) \
     { ACPI_TABLE_FACS *_facs = facs; \
       BOOLEAN ok = FALSE; \
       Disable(); if (_facs->GlobalLock == 1) { _facs->GlobalLock=0; ok = TRUE; } Enable(); \
       pend = ok; }

/* AROS uses GCC */

#include "acgcc.h"

#endif /* __ACAROS_H__ */
