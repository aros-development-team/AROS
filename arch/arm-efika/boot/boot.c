/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: boot.c
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
static unsigned long mem_lower;
static void *pkg_image;
static uint32_t pkg_size;

#define MAX_PAGE_TABLES 10

static pde_t page_dir[4096] __attribute__((aligned(16384)));
static pte_t page_tables[MAX_PAGE_TABLES][256] __attribute__((aligned(1024)));

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
	        mem_lower = t->u.mem.start;

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

void setup_mmu(uintptr_t kernel_phys, uintptr_t kernel_virt, uintptr_t length)
{
    int i;
    uintptr_t first_1M_page = kernel_virt & 0xfff00000;
    uintptr_t last_1M_page = ((kernel_virt + length + 0x000fffff) & 0xfff00000) - 1;

    kprintf("[BOOT] Preparing initial MMU map\n");

    /* Clear page dir */
    for (i=0; i < 4096; i++)
        page_dir[i].raw = 0;

    /* 1:1 memory mapping */
    for (i=(mem_lower >> 20); i < (mem_upper >> 20); i++)
    {
        page_dir[i].section.type = PDE_TYPE_SECTION;
        page_dir[i].section.b = 0;
        page_dir[i].section.c = 1;      /* Cacheable */
        page_dir[i].section.ap = 3;     /* All can read&write */
        page_dir[i].section.base_address = i;
    }

    /* 0x70000000 - 0x7fffffff */
    for (i = (0x70000000 >> 20); i < (0x80000000 >> 20); i++)
    {
        page_dir[i].section.type = PDE_TYPE_SECTION;
        page_dir[i].section.b = 1;      /* Shared device */
        page_dir[i].section.c = 0;      /* Non-Cacheable */
        page_dir[i].section.ap = 3;     /* All can read&write */
        page_dir[i].section.base_address = i;
    }

    kprintf("[BOOT] Preparing mapping for kernel (%d bytes, pages %08x to %08x)\n",
            length, first_1M_page, last_1M_page);

    int fp = first_1M_page >> 20;
    int lp = last_1M_page >> 20;
    int used_pte = 0;

    if (lp == fp) lp++;

    for (i = fp; i < lp; i++)
    {
        if (used_pte >= MAX_PAGE_TABLES)
        {
            kprintf("[BOOT] Run out of free page tables. Kernel larger than 10MB?\n");
            for(;;);
        }

        page_dir[i].coarse.type = PDE_TYPE_COARSE;
        page_dir[i].coarse.domain = 0;
        page_dir[i].coarse.sbz = 0;
        page_dir[i].coarse.imp = 0;
        page_dir[i].coarse.base_address = (intptr_t)&page_tables[used_pte] >> 10;

        int j;
        for (j = 0; j < 256; j++)
        {
            uintptr_t synth_addr = (i << 20) + (j << 12);

            if (synth_addr < kernel_virt)
                page_tables[used_pte][j].raw = 0;
            else if (synth_addr < kernel_virt + length)
            {
                page_tables[used_pte][j].raw = 0;
                page_tables[used_pte][j].page.base_address = (synth_addr - kernel_virt + kernel_phys) >> 12;
                page_tables[used_pte][j].page.b = 0;
                page_tables[used_pte][j].page.c = 1;
                page_tables[used_pte][j].page.ap = 3;
                page_tables[used_pte][j].page.type = PTE_TYPE_PAGE;
            }
            else
                page_tables[used_pte][j].raw = 0;
        }

        used_pte++;
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
    tmp &= ~2;          /* Disable MMU and caches */
    tmp |= 1 << 13;     /* Exception vectors at 0xffff0000 */
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
    	kernel_virt = 0xffff0000 - total_size_ro - total_size_rw;

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

                /* load it */
                loadElf(base);
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

    				/* go to the next file */
    				file += len;
    			}
    		}
    	}

    	arm_flush_cache(kernel_phys, total_size_ro + total_size_rw);

        tag->ti_Tag = KRN_KernelBss;
        tag->ti_Data = (IPTR)tracker;
        tag++;


        kprintf("[BOOT] First level MMU page at %08x\n", page_dir);

        setup_mmu(kernel_phys, kernel_virt, total_size_ro + total_size_rw);
    }

    tag->ti_Tag = TAG_DONE;
    tag->ti_Data = 0;

    kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)tag - (intptr_t)tags)/sizeof(struct TagItem));
    kprintf("[BOOT] Bootstrap wasted %d bytes of memory for kernels use\n", mem_used()   );


    /* Write page_dir address to ttbr0 */
    asm volatile ("mcr p15, 0, %0, c2, c0, 0"::"r"(page_dir));
    /* Write ttbr control N = 0 (use only ttbr0) */
    asm volatile ("mcr p15, 0, %0, c2, c0, 2"::"r"(0));

    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
    kprintf("[BOOT] control register %08x\n", tmp);
    tmp |= 1;          /* Enable MMU */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(tmp));

    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
    kprintf("[BOOT] control register %08x\n", tmp);

    kprintf("[BOOT] Heading over to AROS kernel @ %08x\n", entry);

    entry(tags);

    kprintf("[BOOT] Back? Something wrong happened...\n");

    while(1);
}
