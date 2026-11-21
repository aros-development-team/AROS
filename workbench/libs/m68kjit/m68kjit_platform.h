/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kjit engine: what the engine asks of the system it runs on.
    m68kjit_platform.c answers for AROS and is built into the library. The host
    checks in developer/debug/test/m68kjit_host have their own file.
*/

#ifndef M68KJIT_PLATFORM_H
#define M68KJIT_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

/* 1 where code the translator writes can be run. A build for a host says
   so itself. Without that this is AROS: hosted AROS has another contract
   for executable memory, so only the standalone flavour runs code. */
#ifndef M68KJIT_PLAT_RUNS
#include <aros/config.h>
#define M68KJIT_PLAT_RUNS (AROS_FLAVOUR == AROS_FLAVOUR_STANDALONE)
#endif

void *m68kjit_plat_alloc(size_t size);             /* zeroed memory, or NULL */
void  m68kjit_plat_free(void *p);

/* memory that code is written to and run from, or NULL */
void *m68kjit_plat_code_alloc(size_t size);
void  m68kjit_plat_code_free(void *code, size_t size);
/* around every write into it. After write_end the len bytes at start are
   what the processor executes. */
void  m68kjit_plat_code_write_begin(void *code, size_t size);
void  m68kjit_plat_code_write_end(void *code, size_t size, void *start, size_t len);
#ifdef M68KJIT_TEST_SEAMS
const char *m68kjit_plat_code_method(void);        /* how that memory is made, for a line of the checks */
#endif

void  m68kjit_plat_dead_end(void);                 /* the engine found itself wrong; does not return */

#endif /* M68KJIT_PLATFORM_H */
