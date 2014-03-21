/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifndef BOOT_H_
#define BOOT_H_

#include <inttypes.h>
#include <sys/types.h>
#include <aros/kernel.h>

#define BOOT_STACK_SIZE		4*10*(4*1024)	/* Minimum stack is 4kb (MMU chosen page size) and we need four stacks */

const char *remove_path(const char *in);
void arm_dcache_invalidate(uint32_t addr, uint32_t length);
void arm_icache_invalidate(uint32_t addr, uint32_t length);
void arm_flush_cache(uint32_t addr, uint32_t length);

extern uint8_t __bootstrap_start;
extern uint8_t __bootstrap_end;

extern void *_binary_kernel_bin_start;
extern long *_binary_kernel_bin_end;
extern long _binary_kernel_bin_size;

void kprintf(const char *format, ...);

#define _STR(x)		# x
#define STR(x)		_STR(x)

#define MAX_BSS_SECTIONS 256

#endif /* BOOT_H_ */
