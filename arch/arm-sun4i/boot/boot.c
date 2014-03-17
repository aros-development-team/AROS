/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <inttypes.h>
#include <asm/cpu.h>
#include <asm/arm/mmu.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <string.h>
#include <stdlib.h>

#include "boot.h"
#include "atags.h"
#include "elf.h"

#include <hardware/sun4i/platform.h>

#define READ_ARM_REGISTER(var) \
__asm volatile("mov %[" #var "], " #var "\n\t" : [var] "=r" (var))

asm("	.section .aros.startup				\n"
"		.globl bootstrap					\n"
"		.type bootstrap,%function			\n"
"											\n"
"bootstrap:									\n"
"		mov		r0, r2						\n"
"											\n"
"		mov		r3, #0x0002					\n"		/* ATAG_MEM */
"		movt	r3, #0x5441					\n"
"											\n"
"		b		.get_tag					\n"
"											\n"
".tag_compare:								\n"
"		ldr		ip, [r0, #4]				\n"
"		cmp		ip, r3						\n"
"		beq		.atag_mem_found				\n"
"											\n"
".get_tag:									\n"
"		ldr		ip, [r0]					\n"
"		cmp		ip, #0						\n"
"		add		r0, r0, ip, lsl #2			\n"
"		bne		.tag_compare				\n"
"											\n"
"		b		.fancy_error_loop			\n"
"											\n"
".atag_mem_found:							\n"		/* Set initial stackpointer to end of DRAM */
"		ldr		sp, [r0, #12]				\n"
"		ldr		r0, [r0, #8]				\n"
"		add		sp, sp, r0					\n"
"											\n"
"		mrc		p15, 0, r0, c1, c0, 2		\n"		/* Enable NEON and VFP */
"		orr		r0, r0, #(0xf << 20)		\n"
"		mcr		p15, 0, r0, c1, c0, 2		\n"
"		isb									\n"
"		mov		r0, #0x40000000				\n"
"		vmsr	fpexc, r0					\n"
"											\n"
"		mrc		p15, 0, r0, c1, c0, 0		\n"
"		bic		r0, r0, #0x7				\n"		/* Disable MMU, level one data cache and strict alignment fault checking */
"		orr		r0, r0, #(1 << 13)			\n"		/* High exception vectors selected, address range = 0xFFFF0000-0xFFFF001C */
"		mcr		p15, 0, r0, c1, c0, 0		\n"
"											\n"
"		mov		r0, #0						\n"
"		b		boot						\n"
"											\n"
".fancy_error_loop:							\n"
"		b		.							\n"
"											\n"
);

void debug_control_register(void) {

	uint32_t tmp;

	asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
	kprintf("[BOOT] c1 control register\n");

	kprintf("    MMU ");
	if(tmp & (1<<0)){
		kprintf("enabled\n");
	}else{
		kprintf("disabled\n");
	}

	kprintf("    Strict alignment fault checking ");
	if(tmp & (1<<1)){
		kprintf("enabled\n");
	}else{
		kprintf("disabled\n");
	}

	kprintf("    Data caching  ");
	if(tmp & (1<<2)){
		kprintf("enabled\n");
	}else{
		kprintf("disabled\n");
	}  

	kprintf("    Program flow prediction ");
	if(tmp & (1<<11)){
		kprintf("enabled\n");
	}else{
		kprintf("disabled\n");
	}

	kprintf("    Instruction caching ");
	if(tmp & (1<<12)){
		kprintf("enabled\n");
	}else{
		kprintf("disabled\n");
	}

	kprintf("    Exception vectors ");
	if(tmp & (1<<13)){
		kprintf("0xFFFF0000-0xFFFF001C\n");
	}else{
		kprintf("0x00000000-0x0000001C\n");
	}

	kprintf("\n");

}

static __used unsigned char __stack[BOOT_STACK_SIZE];
static __used void * tmp_stack_ptr = &__stack[BOOT_STACK_SIZE-16];

static struct TagItem tags[128];
static struct TagItem *tag = &tags[0];
static unsigned long *mem_upper = NULL;
static unsigned long *mem_lower = NULL;
static void *pkg_image;
static uint32_t pkg_size;

static void parse_atags(struct tag *tags) {
	struct tag *t = NULL;

	kprintf("[BOOT] Parsing ATAGS\n");

	for_each_tag(t, tags) {
		kprintf("[BOOT]   %08x (%04x): ", t->hdr.tag, t->hdr.size);

		switch (t->hdr.tag) {

			case ATAG_MEM: {
				kprintf("Memory (%08x-%08x)\n", t->u.mem.start, t->u.mem.size + t->u.mem.start - 1);
	        	tag->ti_Tag = KRN_MEMLower;
	        	tag->ti_Data = t->u.mem.start;
	        	mem_lower = &tag->ti_Data;
	        	tag++;

	        	tag->ti_Tag = KRN_MEMUpper;
	        	tag->ti_Data = t->u.mem.start + t->u.mem.size;
	        	mem_upper = &tag->ti_Data;
	        	tag++;
			}
			break;

			case ATAG_CMDLINE: {
				char *cmdline = malloc(strlen(t->u.cmdline.cmdline) + 1);
				strcpy(cmdline, t->u.cmdline.cmdline);
				kprintf("CMDLine: \"%s\"\n", cmdline);

				tag->ti_Tag = KRN_CmdLine;
	        	tag->ti_Data = (intptr_t)cmdline;
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
}

void setup_mmu(uintptr_t kernel_phys, uintptr_t kernel_virt, uintptr_t length) {

    uint32_t i;

    uintptr_t first_1M_page = kernel_virt & 0xfff00000;
    uintptr_t last_1M_page = ((kernel_virt + length + 0x000fffff) & 0xfff00000) - 1;

    kprintf("[BOOT] Preparing initial MMU map\n");

    /* Use memory right below kernel for page dir */
    pde_t *page_dir = (pde_t *)(((uintptr_t)kernel_phys - 16384) & ~ 16383);

    kprintf("[BOOT] First level MMU page at %08x\n", page_dir);

    /* Clear page dir */
    for (i=0; i < 4096; i++) {
        page_dir[i].raw = 0;
	}

    /* 1:1 memory mapping */
    for (i=(*mem_lower >> 20); i < (*mem_upper >> 20); i++) {
        //page_dir[i].raw = 0;
        page_dir[i].section.type = PDE_TYPE_SECTION;
        page_dir[i].section.b = 0;
        page_dir[i].section.c = 1;      /* Cacheable */
        page_dir[i].section.ap = 3;     /* All can read&write */
        page_dir[i].section.base_address = i;
    }

    /* v:p memory mapping */
    for (i=(kernel_virt >> 20); i <= (0xffffffff >> 20); i++) {
        //page_dir[i].raw = 0;
        page_dir[i].section.type = PDE_TYPE_SECTION;
        page_dir[i].section.b = 0;
        page_dir[i].section.c = 1;      /* Cacheable */
        page_dir[i].section.ap = 3;     /* All can read&write */
        page_dir[i].section.base_address = (kernel_phys >> 20);
    }

    pte_t *current_pte = (pte_t *)page_dir;

    /* Write page_dir address to ttbr0 */
    asm volatile ("mcr p15, 0, %0, c2, c0, 0"::"r"(page_dir));
    /* Write ttbr control N = 0 (use only ttbr0) */
    asm volatile ("mcr p15, 0, %0, c2, c0, 2"::"r"(0));

    *mem_upper = (intptr_t)current_pte;
}


void boot(uintptr_t dummy, uintptr_t arch, struct tag * atags) {
	uint32_t tmp;

	uint32_t r0,sp;
	READ_ARM_REGISTER(r0);
	READ_ARM_REGISTER(sp);

	void (*entry)(struct TagItem *tags) = NULL;

    kprintf("[BOOT] AROS for sun4i (" SUN4I_PLATFORM_NAME ") bootstrap\n");
	kprintf("[BOOT] r0 = %x\n", r0);
	kprintf("[BOOT] sp = %x\n\n", sp);

	debug_control_register();

    tag->ti_Tag = KRN_BootLoader;
    tag->ti_Data = (IPTR)"Bootstrap/sun4i (" SUN4I_PLATFORM_NAME ") ARM";
    tag++;

    parse_atags(atags);

    kprintf("[BOOT] Bootstrap @ %08x-%08x\n", &__bootstrap_start, &__bootstrap_end);
    kprintf("[BOOT] Topmost address for kernel: %p\n", *mem_upper);

	if(*mem_upper){
		if((*mem_upper & 0x0000ffff) == 0x0000ffff) {
			*mem_upper = (*mem_upper & 0xffff0000);
		}else{
			*mem_upper = (*mem_upper & 0xffff0000) - 0x10000;
		}
	}

    kprintf("[BOOT] Topmost address for kernel: %p\n", *mem_upper);

	uint32_t *vectortest;
	vectortest = 0x7fff0000;
	*vectortest = 0xdeadbeef;
    kprintf("[BOOT] vectortest %x(%x)\n", vectortest, *vectortest);

	vectortest = 0xffff0000;
    kprintf("[BOOT] vectortest %x(%x)\n", vectortest, *vectortest);


    if (*mem_upper) {
    	*mem_upper = *mem_upper & ~4095;

    	unsigned long kernel_phys = *mem_upper;
    	unsigned long kernel_virt = kernel_phys;

    	unsigned long total_size_ro, total_size_rw;
    	uint32_t size_ro, size_rw;

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

    	kernel_phys = *mem_upper - total_size_ro - total_size_rw;
    	kernel_virt = 0xffff0000 - total_size_ro - total_size_rw;

    	/* Adjust "top of memory" pointer */
    	*mem_upper = kernel_phys;

    	kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);
    	kprintf("[BOOT] Virtual address of kernel: %p\n", kernel_virt);

    	entry = (void (*)(struct TagItem *))kernel_virt;

        initAllocator(kernel_phys, kernel_phys  + total_size_ro, kernel_virt - kernel_phys);

    	tag->ti_Tag = KRN_KernelLowest;
    	tag->ti_Data = kernel_phys;
    	tag++;

    	tag->ti_Tag = KRN_KernelHighest;
    	tag->ti_Data = kernel_phys + ((total_size_ro + 4095) & ~4095) + ((total_size_rw + 4095) & ~4095);
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

      setup_mmu(kernel_phys, kernel_virt, total_size_ro + total_size_rw);
    }

    tag->ti_Tag = TAG_DONE;
    tag->ti_Data = 0;

    kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)tag - (intptr_t)tags)/sizeof(struct TagItem));
    kprintf("[BOOT] Bootstrap wasted %d bytes of memory for kernels use\n", mem_used()   );

    if (entry) {
        /* Set domains - Dom0 is usable, rest is disabled */
        asm volatile ("mrc p15, 0, %0, c3, c0, 0":"=r"(tmp));
        kprintf("[BOOT] Domain access control register: %08x\n", tmp);
        asm volatile ("mcr p15, 0, %0, c3, c0, 0"::"r"(0x00000001));

		debug_control_register();
        asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
        tmp |= 1;          /* Enable MMU */
        asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(tmp));
		debug_control_register();

        kprintf("[BOOT] Heading over to AROS kernel @ %08x\n", entry);

        entry(tags);

        kprintf("[BOOT] Back? Something wrong happened...\n");
    } else {
        kprintf("[BOOT] kernel entry pointer not set?\n");
    }

    while(1);
}
