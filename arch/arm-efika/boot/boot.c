/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: boot.c
    Lang: english
*/

#include <inttypes.h>
#include <asm/cpu.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <string.h>
#include <stdlib.h>

#include "boot.h"
#include "serialdebug.h"
#include "atags.h"
#include "elf.h"

asm("	.section .aros.startup		\n"
"		.globl bootstrap			\n"
"		.type bootstrap,%function	\n"
"bootstrap:							\n"
"		movw	r0,#:lower16:tmp_stack_ptr\n"
"		movt	r0,#:upper16:tmp_stack_ptr\n"
"		ldr		sp,[r0,#0]			\n"
"		b		boot				\n"
);

static __used unsigned char __stack[BOOT_STACK_SIZE];
static __used void * tmp_stack_ptr = &__stack[BOOT_STACK_SIZE-16];

static struct TagItem tags[128];
static struct TagItem *tag = &tags[0];
static unsigned long mem_upper;
static void *pkg_image;
static uint32_t pkg_size;

static void parse_atags(struct tag *tags)
{
	struct tag *t = NULL;

	kprintf("[BOOT] Parsing ATAGS\n");

	for_each_tag(t, tags)
	{
		kprintf("[BOOT]   %08x (%04x): ", t->hdr.tag, t->hdr.size);
		switch (t->hdr.tag)
		{
		case ATAG_MEM:
			kprintf("Memory (%08x-%08x)\n", t->u.mem.start, t->u.mem.size + t->u.mem.start - 1);
	        tag->ti_Tag = KRN_MEMLower;
	        tag->ti_Data = t->u.mem.start;
	        tag++;
	        tag->ti_Tag = KRN_MEMUpper;
	        tag->ti_Data = t->u.mem.start + t->u.mem.size;
	        tag++;

	        mem_upper = t->u.mem.start + t->u.mem.size;

			break;
		case ATAG_CMDLINE:
		{
			char *cmdline = malloc(strlen(t->u.cmdline.cmdline) + 1);
			strcpy(cmdline, t->u.cmdline.cmdline);
			kprintf("CMDLine: \"%s\"\n", cmdline);

			tag->ti_Tag = KRN_CmdLine;
	        tag->ti_Data = (intptr_t)cmdline;
	        tag++;
		}
			break;
		case ATAG_INITRD2:
			kprintf("RAMDISK: (%08x-%08x)\n", t->u.initrd.start, t->u.initrd.size + t->u.initrd.start - 1);
			pkg_image = (void *)t->u.initrd.start;
			pkg_size = t->u.initrd.size;
			break;
		default:
			kprintf("IGN...\n");
			break;
		}
	}
}

void boot(uintptr_t dummy, uintptr_t arch, struct tag * atags)
{
	uint32_t tmp;
	void (*entry)(struct TagItem *tags);

	/* Enable NEON and VFP */
    asm volatile ("mrc p15, 0, %0, c1, c0, 2":"=r"(tmp));
    tmp |= 3 << 20;
    tmp |= 3 << 22;
    asm volatile ("mcr p15, 0, %0, c1, c0, 2"::"r"(tmp));

    fmxr(cr8, fmrx(cr8) | 1 << 30);

    kprintf("[BOOT] AROS for EfikaMX bootstrap\n");

    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
    kprintf("[BOOT] control register %08x\n", tmp);
    tmp &= ~2;
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(tmp));

    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
        kprintf("[BOOT] control register %08x\n", tmp);

    tag->ti_Tag = KRN_BootLoader;
    tag->ti_Data = (IPTR)"Bootstrap/EfikaMX ARM";
    tag++;

    parse_atags(atags);

    kprintf("[BOOT] Bootstrap @ %08x-%08x\n", &__bootstrap_start, &__bootstrap_end);
    kprintf("[BOOT] Topmost address for kernel: %p\n", mem_upper);

    if (mem_upper)
    {
    	mem_upper = mem_upper & ~4095;

    	unsigned long kernel_phys = mem_upper;
    	unsigned long kernel_virt = kernel_phys;

    	unsigned long total_size_ro, total_size_rw;
    	uint32_t size_ro, size_rw;

    	/* Calculate total size of kernel and modules */

    	getElfSize(&_binary_kernel_bin_start, &size_rw, &size_ro);

    	total_size_ro = size_ro = (size_ro + 4095) & ~4095;
    	total_size_rw = size_rw = (size_rw + 4095) & ~4095;

    	if (pkg_image && pkg_size)
    	{
    		uint8_t *base = pkg_image;

    		if (base[0] == 0x7f && base[1] == 'E' && base[2] == 'L' && base[3] == 'F')
    		{
    			kprintf("[BOOT] Kernel image is ELF file\n");

    			getElfSize(base, &size_rw, &size_ro);

    	    	total_size_ro += (size_ro + 4095) & ~4095;
    	    	total_size_rw += (size_rw + 4095) & ~4095;
    		}
    		else if (base[0] == 'P' && base[1] == 'K' && base[2] == 'G' && base[3] == 0x01)
    		{
    			kprintf("[BOOT] Kernel image is a package:\n");

    			uint8_t *file = base+4;
    			uint32_t total_length = AROS_BE2LONG(*(uint32_t*)file); /* Total length of the module */
    			const uint8_t *file_end = base+total_length;
    			uint32_t len;

    			kprintf("[BOOT] Package size: %dKB\n", total_length >> 10);

    			file = base + 8;

    			while(file < file_end)
    			{
    				const char *filename = remove_path(file+4);

    				/* get text length */
    				len = AROS_BE2LONG(*(uint32_t*)file);
    				/* display the file name */
    				kprintf("[BOOT]    %s ", filename);

    				file += len + 5;

    				len = AROS_BE2LONG(*(uint32_t *)file);
    				file += 4;

    				/* load it */
    				getElfSize(file, &size_rw, &size_ro);

    				total_size_ro += (size_ro + 4095) & ~4095;
    				total_size_rw += (size_rw + 4095) & ~4095;

    				/* go to the next file */
    				file += len;
    			}
    		}
    	}

    	kernel_phys = mem_upper - total_size_ro - total_size_rw;
    	kernel_virt = kernel_phys;

    	kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);
    	kprintf("[BOOT] Virtual address of kernel: %p\n", kernel_virt);

    	entry = (void (*)(struct TagItem))kernel_virt;

        initAllocator(kernel_phys, kernel_phys  + total_size_ro, kernel_virt - kernel_phys);

    	tag->ti_Tag = KRN_KernelLowest;
    	tag->ti_Data = kernel_phys;
    	tag++;

    	tag->ti_Tag = KRN_KernelHighest;
    	tag->ti_Data = kernel_phys + ((total_size_ro + 4095) & ~4095) + ((total_size_rw + 4095) & ~4095);
    	tag++;

     	loadElf(&_binary_kernel_bin_start);

    	if (pkg_image && pkg_size)
    	{
    		uint8_t *base = pkg_image;

    		if (base[0] == 0x7f && base[1] == 'E' && base[2] == 'L' && base[3] == 'F')
    		{
    			kprintf("[BOOT] Kernel image is ELF file\n");

    			getElfSize(base, &size_rw, &size_ro);

    			total_size_ro += (size_ro + 4095) & ~4095;
    			total_size_rw += (size_rw + 4095) & ~4095;
    		}
    		else if (base[0] == 'P' && base[1] == 'K' && base[2] == 'G' && base[3] == 0x01)
    		{
    			kprintf("[BOOT] Kernel image is a package:\n");

    			uint8_t *file = base+4;
    			uint32_t total_length = AROS_BE2LONG(*(uint32_t*)file); /* Total length of the module */
    			const uint8_t *file_end = base+total_length;
    			uint32_t len;

    			kprintf("[BOOT] Package size: %dKB\n", total_length >> 10);

    			file = base + 8;

    			while(file < file_end)
    			{
    				const char *filename = remove_path(file+4);

    				/* get text length */
    				len = AROS_BE2LONG(*(uint32_t*)file);
    				/* display the file name */
    				kprintf("[BOOT]    %s ", filename);

    				file += len + 5;

    				len = AROS_BE2LONG(*(uint32_t *)file);
    				file += 4;

    				/* load it */
    				loadElf(file);

    				total_size_ro += (size_ro + 4095) & ~4095;
    				total_size_rw += (size_rw + 4095) & ~4095;

    				/* go to the next file */
    				file += len;
    			}
    		}
    	}

    	arm_flush_cache(kernel_phys, total_size_ro + total_size_rw);

        tag->ti_Tag = KRN_KernelBss;
        tag->ti_Data = (IPTR)tracker;
        tag++;
    }

    tag->ti_Tag = TAG_DONE;
    tag->ti_Data = 0;

    kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)tag - (intptr_t)tags)/sizeof(struct TagItem));
    kprintf("[BOOT] Bootstrap wasted %d bytes of memory for kernels use\n", mem_used()   );

    kprintf("[BOOT] Heading over to AROS kernel @ %08x\n", entry);
    entry(tags);

    kprintf("[BOOT] Back? Something wrong happened...\n");

    while(1);
}
