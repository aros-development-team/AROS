/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <inttypes.h>
#include <asm/cpu.h>
#include <asm/arm/mmu.h>
#include <asm/arm/cp15.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <string.h>
#include <stdlib.h>

#include "boot.h"
#include "atags.h"
#include "elf.h"

#include <hardware/sun4i/platform.h>

asm("	.section .aros.startup					\n"
"		.globl bootstrap						\n"
"		.type bootstrap,%function				\n"
"												\n"
"bootstrap:										\n"
"		mov		r0, #0							\n"
"		mov		r4, #2							\n"		/* Size of ATAGS + ATAG_NONE */
"		mov		sp, r0							\n"
"												\n"
"		mov		r0, r2							\n"
"												\n"
"		mov		r3, #0x0002						\n"		/* ATAG_MEM */
"		movt	r3, #0x5441						\n"
"		b		.get_tag						\n"
"												\n"
".tag_compare:									\n"
"		ldr		ip, [r0, #4]					\n"
"		cmp		ip, r3							\n"
"		bne		.get_tag						\n"
"												\n"
"		cmp		sp, #0							\n"		/* Allow only one ATAG_MEM tag, else we get confused  */
"		bne		.fancy_error_loop				\n"
"												\n"
"		ldr		sp, [r0, #12]					\n"		/* Initial stackpointer is end of DRAM minus (space for vectors + initial stack size) */
"		ldr		r5, [r0, #8]					\n"
"		add		sp, sp, r5						\n"
"		mov		r5, sp							\n"
"		lsr		sp, sp, #16						\n"
"		lsl		sp, sp, #16						\n"
"		sub		sp, sp, #0x10000				\n"
"		mov		r6, sp							\n"
"		sub		r6, r6, #"STR(BOOT_STACK_SIZE)"	\n"
"		sub		r6, r5, r6						\n"
"		ldr		r5, [r0, #8]					\n"
"		sub		r5, r5, r6						\n"
"		str		r5, [r0, #8]					\n"		/* In future clean the above mess, uses now too many registers */
"												\n"
".get_tag:										\n"
"		ldr		ip, [r0]						\n"
"		cmp		ip, #0							\n"
"		add		r0, r0, ip, lsl #2				\n"
"		add		r4, r4, ip						\n"
"		bne		.tag_compare					\n"
"												\n"
"		cmp		sp, #0							\n"
"		beq		.fancy_error_loop				\n"
"												\n"
"		sub		sp, sp, r4, lsl #2				\n"		/* Copy ATAGs in the stack */
"		mov		r0, sp							\n"
".atag_copy:									\n"
"		ldr		r3, [r2], #4					\n"
"		str		r3, [r0], #4					\n"
"		subs	r4, r4, #1						\n"
"		bne		.atag_copy						\n"
"												\n"
"		mov		r2, sp							\n"
"		mov		r0, #0							\n"
"		b		boot							\n"
"												\n"
".fancy_error_loop:								\n"
"		b		.								\n"
"												\n"
);

void setup_mmu(uint32_t kernel_phys, uint32_t kernel_virt, uint32_t mem_lower, uint32_t mem_upper) {

    uint32_t i;

    kprintf("[BOOT] MMU kernel_phys %08x\n", kernel_phys);
    kprintf("[BOOT] MMU kernel_virt %08x\n", kernel_virt);
    kprintf("[BOOT] MMU mem_lower   %08x\n", mem_lower);
    kprintf("[BOOT] MMU mem_upper   %08x\n", mem_upper);

    /* Use memory right below kernel for page dir */
    pde_t *page_dir = (pde_t *)(((uintptr_t)kernel_phys - 16384) & ~ 16383);

    kprintf("[BOOT] First level MMU page at %08x\n", page_dir);

    /* Clear page dir */
    for (i=0; i < 4096; i++) {
        page_dir[i].raw = 0;
	}

    /* 1:1 memory mapping */
    for (i=(mem_lower >> 20); i < (mem_upper >> 20); i++) {
    	//kprintf("[BOOT] Memory mapping page %d\n", i);
        //page_dir[i].raw = 0;
        page_dir[i].section.type = PDE_TYPE_SECTION;
        page_dir[i].section.b = 0;
        page_dir[i].section.c = 1;      /* Cacheable */
        page_dir[i].section.ap = 3;     /* All can read&write */
        page_dir[i].section.base_address = i;
    }

    /* v:p memory mapping */
    for (i=(kernel_virt >> 20); i <= (0xffffffff >> 20); i++) {
    	kprintf("[BOOT] Kernel mapping page %d\n", i);
        //page_dir[i].raw = 0;
        page_dir[i].section.type = PDE_TYPE_SECTION;
        page_dir[i].section.b = 0;
        page_dir[i].section.c = 1;      /* Cacheable */
        page_dir[i].section.ap = 3;     /* All can read&write */
        page_dir[i].section.base_address = (kernel_phys >> 20); // Fixme
    }

    pte_t *current_pte = (pte_t *)page_dir;

    /* Write page_dir address to ttbr0 */
    asm volatile ("mcr p15, 0, %0, c2, c0, 0"::"r"(page_dir));
    /* Write ttbr control N = 0 (use only ttbr0) */
    asm volatile ("mcr p15, 0, %0, c2, c0, 2"::"r"(0));

    mem_upper = (intptr_t)current_pte;
}

void boot(uintptr_t dummy, uintptr_t arch, struct tag *atags) {

	struct tag *t = NULL;
	struct TagItem tags[128];
	struct TagItem *tag = &tags[0];

	uint32_t mem_upper = 0;
	uint32_t mem_lower = 0;

	void *pkg_image;
	uint32_t pkg_size;

	uint32_t tmp;

	/* Disable MMU, level one data cache and strict alignment fault checking */
	CP15_C1CR_Clear(C1CRF_C|C1CRF_A|C1CRF_M);

	/* High exception vectors selected, address range = 0xFFFF0000-0xFFFF001C */
	CP15_C1CR_Set(C1CRF_V);

	/* Set cp10 and cp11 for Privileged and User mode access */
	CP15_C1CACR_All(C1CACRV_CPAP(10)|C1CACRV_CPAP(11));

	/* Enable VFP (NEON in our case) */
    fmxr(cr8, fmrx(cr8) | 1 << 30);

	void (*entry)(struct TagItem *tags) = NULL;

    kprintf("[BOOT] AROS for sun4i (" SUN4I_PLATFORM_NAME ") bootstrap\n");

    tag->ti_Tag = KRN_BootLoader;
    tag->ti_Data = (IPTR)"Bootstrap/sun4i (" SUN4I_PLATFORM_NAME ") ARM";
    tag++;

	kprintf("[BOOT] Parsing ATAGS %x\n", tags);

	for_each_tag(t, atags) {
		kprintf("[BOOT]   (%x-%x) tag %08x (%d): ", t, (uint32_t)t+(t->hdr.size<<2)-1, t->hdr.tag, t->hdr.size);

		switch (t->hdr.tag) {

			case ATAG_MEM: {
				kprintf("Memory (%08x-%08x)\n", t->u.mem.start, t->u.mem.size + t->u.mem.start - 1);
	        	tag->ti_Tag = KRN_MEMLower;
	        	tag->ti_Data = t->u.mem.start;
	        	mem_lower = tag->ti_Data;
	        	tag++;

	        	tag->ti_Tag = KRN_MEMUpper;
	        	tag->ti_Data = t->u.mem.start + t->u.mem.size;
	        	mem_upper = tag->ti_Data;
	        	tag++;
			}
			break;

			case ATAG_CMDLINE: {
				kprintf("CMDLine: \"%s\"\n", t->u.cmdline.cmdline);
				tag->ti_Tag = KRN_CmdLine;
	        	tag->ti_Data = (intptr_t)t->u.cmdline.cmdline;
	        	tag++;
			}
			break;

			case ATAG_INITRD2: {
				kprintf("RAMDISK: (%08x-%08x)\n", t->u.initrd.start, t->u.initrd.size + t->u.initrd.start - 1);
				pkg_image = (void *)t->u.initrd.start;
				pkg_size = t->u.initrd.size;
			}
			break;

			default: {
				kprintf("IGN...\n");
			}
			break;

		}
	}

	if(t->hdr.size == 0) {
		kprintf("[BOOT]   (%x-%x) tag %08x (%d): ATAG_NONE\n", t, (uint32_t)t+(2<<2)-1, t->hdr.tag, 2);
	}

    kprintf("[BOOT] Bootstrap @ %08x-%08x\n", &__bootstrap_start, &__bootstrap_end);
    kprintf("[BOOT] Topmost address for kernel: %p\n", mem_upper);

    if (mem_upper) {

    	uint32_t kernel_phys = mem_upper;
    	uint32_t kernel_virt = kernel_phys;

    	uint32_t total_size_ro;
		uint32_t total_size_rw;
    	uint32_t size_ro;
		uint32_t size_rw;

    	/* Calculate total size of kernel and modules */

    	getElfSize(&_binary_kernel_bin_start, &size_rw, &size_ro);

    	total_size_ro = size_ro = (size_ro + 4095) & ~4095;
    	total_size_rw = size_rw = (size_rw + 4095) & ~4095;

    	if (pkg_image && pkg_size) {
    		uint8_t *base = pkg_image;

    		if (base[0] == 0x7f && base[1] == 'E' && base[2] == 'L' && base[3] == 'F') {
    			kprintf("[BOOT] Kernel image is ELF file\n");

    			getElfSize(base, &size_rw, &size_ro);

    	    	total_size_ro += (size_ro + 4095) & ~4095;
    	    	total_size_rw += (size_rw + 4095) & ~4095;
    		} else if (base[0] == 'P' && base[1] == 'K' && base[2] == 'G' && base[3] == 0x01) {
    			kprintf("[BOOT] Kernel image is a package:\n");

    			uint8_t *file = base+4;
    			uint32_t total_length = AROS_BE2LONG(*(uint32_t*)file); /* Total length of the module */
    			const uint8_t *file_end = base+total_length;
    			uint32_t len;

    			kprintf("[BOOT] Package size: %dKB\n", total_length >> 10);

    			file = base + 8;

    			while(file < file_end) {
    				const char *filename = remove_path(file+4);

    				/* get text length */
    				len = AROS_BE2LONG(*(uint32_t*)file);
    				/* display the file name */
    				kprintf("[BOOT]    %s \n", filename);

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
    	kernel_virt = 0xffff0000 - total_size_ro - total_size_rw - BOOT_STACK_SIZE;

		/*
			ffff ffff	(free) Top of the world
			ffff fffb	(free) ~4
				|		(free)
			ffff 003c	routine address FIQ
			ffff 0038	routine address IRQ
			ffff 0034	routine address Reserved
			ffff 0030	routine address Data Abort
			ffff 002c	routine address Prefetch Abort
			ffff 0028	routine address Software interrupt
			ffff 0024	routine address Undefined instruction 
			ffff 0020	routine address Reset
			ffff 001c	FIQ
			ffff 0018	IRQ
			ffff 0014	Reserved
			ffff 0010	Data Abort
			ffff 000c	Prefetch Abort
			ffff 0008	Software interrupt
			ffff 0004	Undefined instruction
			ffff 0000	Reset
			fffe ffff	U-Boot ATAGS end
				|		Stack
			fffe 0000	End of stack
			fffd ffff	End of kernel
				|
		*/

    	/* Adjust "top of memory" pointer */
    	mem_upper = kernel_phys;

    	kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);
    	kprintf("[BOOT] Virtual address of kernel: %p\n", kernel_virt);

    	entry = (void (*)(struct TagItem *))kernel_virt;

        initAllocator(kernel_phys, kernel_phys  + total_size_ro, kernel_virt - kernel_phys);

    	tag->ti_Tag = KRN_KernelLowest;
    	tag->ti_Data = kernel_virt;
    	tag++;

    	tag->ti_Tag = KRN_KernelHighest;
    	tag->ti_Data = kernel_virt + ((total_size_ro + 4095) & ~4095) + ((total_size_rw + 4095) & ~4095);
    	tag++;

     	loadElf(&_binary_kernel_bin_start);

    	if (pkg_image && pkg_size) {
    		uint8_t *base = pkg_image;

    		if (base[0] == 0x7f && base[1] == 'E' && base[2] == 'L' && base[3] == 'F') {
    			kprintf("[BOOT] Kernel image is ELF file\n");

                /* load it */
                loadElf(base);
    		} else if (base[0] == 'P' && base[1] == 'K' && base[2] == 'G' && base[3] == 0x01) {
    			kprintf("[BOOT] Kernel image is a package:\n");

    			uint8_t *file = base+4;
    			uint32_t total_length = AROS_BE2LONG(*(uint32_t*)file); /* Total length of the module */
    			const uint8_t *file_end = base+total_length;
    			uint32_t len;

    			kprintf("[BOOT] Package size: %dKB\n", total_length >> 10);

    			file = base + 8;

    			while(file < file_end) {
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

    				/* go to the next file */
    				file += len;
    			}
    		}
    	}

    	arm_flush_cache(kernel_phys, total_size_ro + total_size_rw);

        tag->ti_Tag = KRN_KernelBss;
        tag->ti_Data = (IPTR)tracker;
        tag++;

		setup_mmu(kernel_phys, kernel_virt, mem_lower, mem_upper);
    }

    tag->ti_Tag = TAG_DONE;
    tag->ti_Data = 0;

    kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)tag - (intptr_t)tags)/sizeof(struct TagItem));
    kprintf("[BOOT] Topmost address for kernel: %p\n", mem_upper);

    if (entry) {
        /* Set domains - Dom0 is usable, rest is disabled */
        asm volatile ("mrc p15, 0, %0, c3, c0, 0":"=r"(tmp));
        kprintf("[BOOT] Domain access control register: %08x\n", tmp);
        asm volatile ("mcr p15, 0, %0, c3, c0, 0"::"r"(0x00000001));

		/* Enable MMU */
		CP15_C1CR_Set(C1CRF_M);

        kprintf("[BOOT] Heading over to AROS kernel @ %08x\n", entry);

        entry(tags);

        kprintf("[BOOT] Back? Something wrong happened...\n");
    } else {
        kprintf("[BOOT] kernel entry pointer not set?\n");
    }

    while(1);
};

