/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#ifndef BOOT_H_
#define BOOT_H_

#include <inttypes.h>
#include <sys/types.h>
#include <aros/kernel.h>

// Create some temporary storage in .bss for both stack and local allocations.
// Must match __bcm2708_bootstacksize/__bcm2708_boottagssize in bcm2708_boot.h:
// the taglist is placed at (0x4000 - 16) - BOOT_STACK_SIZE - BOOT_TAGS_SIZE,
// which has to land on bm_boottags in struct bcm2708bootmem.
#define BOOT_STACK_SIZE		(768 << 3)
#define BOOT_TAGS_SIZE          (64 << 4)
#define BOOT_TMP_SIZE		(256 * 1024)

#define MAX_BSS_SECTIONS        256

/*
 * Upper bound on an ELF's section count when its section-header table and
 * deltas[] array are held as VLAs on the boot stack. Two arrays of ~72
 * bytes/section share the stack with the call frames, so cap well below
 * BOOT_STACK_SIZE/72; /128 leaves ample headroom (real ROM modules use
 * ~11 sections). Raise BOOT_STACK_SIZE if this ever legitimately trips.
 */
#define BOOT_MAX_SHNUM          (BOOT_STACK_SIZE / 128)

#define KERNEL_VIRT_ADDRESS     0xf8000000UL

void explicit_mem_init(void *, unsigned long);
size_t mem_avail();
size_t mem_used();
const char *remove_path(const char *in);
void aarch64_flush_cache(uintptr_t addr, uintptr_t length);
uintptr_t elf_get_ro_virt(void);

extern uint8_t __bootstrap_start;
extern uint8_t __bootstrap_end;

extern void *_binary_core_bin_start;
extern long *_binary_core_bin_end;
extern long _binary_core_bin_size;

void kprintf(const char *format, ...);

#define _STR(x)		#x
#define STR(x)		_STR(x)

#endif /* BOOT_H_ */
