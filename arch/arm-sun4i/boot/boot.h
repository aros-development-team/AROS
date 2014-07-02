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
#include <aros/macros.h>

#define MEM_OFFSET_VECTOR	0x10000
#define MMU_L1_SIZE			(4*4096)
#define MEM_OFFSET_MMU1		(4*4096)
#define MEM_OFFSET_MMU2		(4*256*4096)
#define MEM_OFFSET_STACKS	4*10*1024

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
