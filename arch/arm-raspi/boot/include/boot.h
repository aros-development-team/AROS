/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef BOOT_H_
#define BOOT_H_

#include <inttypes.h>
#include <sys/types.h>
#include <aros/kernel.h>

// Create some temporary storage in .bss for both stack and local allocations.
#define BOOT_STACK_SIZE		(768 << 2)
#define BOOT_TAGS_SIZE          (64 << 3)
#define BOOT_TMP_SIZE		65536

#define MAX_BSS_SECTIONS        256

#define KERNEL_VIRT_ADDRESS     0xf8000000

void explicit_mem_init(void *, unsigned long);
size_t mem_avail();
size_t mem_used();
const char *remove_path(const char *in);
void arm_flush_cache(uint32_t addr, uint32_t length);

extern uint8_t __bootstrap_start;
extern uint8_t __bootstrap_end;

#if AROS_BIG_ENDIAN
extern void *_binary_core_be_bin_start;
extern long *_binary_core_be_bin_end;
extern long _binary_core_be_bin_size;
#else
extern void *_binary_core_bin_start;
extern long *_binary_core_bin_end;
extern long _binary_core_bin_size;
#endif

void kprintf(const char *format, ...);

#define _STR(x)		#x
#define STR(x)		_STR(x)

#endif /* BOOT_H_ */
