/*
 * bootstrap.c
 *
 *  Created on: Dec 1, 2009
 *      Author: misc
 */

#define DEBUG 0

#include "elf.h"
#include "bootstrap.h"
#include "screen.h"

#include <dos/elf.h>
#include <asm/io.h>
#include <aros/kernel.h>
#include <inttypes.h>
#include <utility/tagitem.h>

#include <stdlib.h>
#include <string.h>

/*
    The Multiboot-compliant header has to be within the first 4KB (1KB??) of ELF file,
    therefore it will be packed into the .aros.startup section. I hope, that turning debug on
    will not shift it into some distinct location.
*/
static const multiboot_header __header __attribute__((used,section(".aros.startup"))) = {
    MB_MAGIC, MB_FLAGS, -(MB_MAGIC+MB_FLAGS)
};

/*
    First portion of code, called from GRUB directly. It has to set the stack up, disable interrupts
    and jump to the regular C code
*/
asm("	.text\n\t"
    "	.globl kernel_bootstrap\n\t"
    "	.type  kernel_bootstrap,@function\n"
    "kernel_bootstrap: movl $__stack+" STR(BOOT_STACK_SIZE) ", %esp\n\t"   /* Load stack pointer */
    "	pushl %ebx\n\t"                                 /* store parameters passed by GRUB in registers */
    "	pushl %eax\n\t"
    "	pushl $0\n\t"
    "	cld\n\t"
    "	cli\n\t"                                        /* Lock interrupts (by flag)... */
    "	jmp __bootstrap"
);

static unsigned char __stack[BOOT_STACK_SIZE] __attribute__((used));
static void die();

static struct TagItem tags[128];
static struct TagItem *tag = &tags[0];

/* C-code entry of bootstrap */
static void __attribute__((used)) __bootstrap(unsigned int magic, unsigned int addr)
{
    struct multiboot *mb = (struct multiboot *)addr;/* Multiboot structure from GRUB */
    unsigned long mem_upper = 0;

    kprintf("[BOOT] Entered AROS Bootstrap @ %p\n", __bootstrap);
    kprintf("[BOOT] Stack @ %p, [%d bytes]\n",__stack, 65536);

    /* Disable interrupts on the XT-PIC directly */
    outb(0xff, 0x21);
    outb(0xff, 0xa1);

    tag->ti_Tag = KRN_BootLoader;
    tag->ti_Data = (IPTR)"Bootstrap/GRUB for x86";
    tag++;

    if (mb->cmdline)
    {
    	char *cmdline = malloc(strlen(mb->cmdline) + 1);
    	strcpy(cmdline, mb->cmdline);
    	kprintf("[BOOT] Command line: %s\n", cmdline);

    	tag->ti_Tag = KRN_CmdLine;
    	tag->ti_Data = (intptr_t)cmdline;
    	tag++;
    }

    if (mb->mmap_length)
    {
    	kprintf("[BOOT] BIOS provides memory map\n");

    	unsigned long len = mb->mmap_length;
    	struct mb_mmap *mmap = (struct mb_mmap *)mb->mmap_addr;

    	void *dst = malloc(mb->mmap_length);
    	memcpy(dst, (void *)mb->mmap_addr, len);

    	tag->ti_Tag = KRN_MMAPAddress;
    	tag->ti_Data = (intptr_t)dst;
    	tag++;
    	tag->ti_Tag = KRN_MMAPLength;
    	tag->ti_Data = len;
    	tag++;

    	/*
    	 * Go through entire list of free RAM
    	 */
    	while (len >= sizeof(struct mb_mmap))
    	{
    		if (mmap->type == MMAP_TYPE_RAM)
    		{
                unsigned long long addr = (mmap->addr_low | ((unsigned long long)mmap->addr_high << 32));
                unsigned long long size = (mmap->len_low | ((unsigned long long)mmap->len_high << 32));

                /*
                 * Check whether the memory region starts in the first 2GB of address space.
                 * Higher areas will not be considered here, as they might potentially
                 * conflict with PCI address space. We do not want to deal with such
                 * trouble here, in this tiny bootstrap code.
                 */
                if (addr < 0x80000000ULL)
                {
                	/*
                	 * Now, check whether the topmost address exceeds the 2GB border. If yes,
                	 * trim it now.
                	 */

                	if (addr + size > 0x80000000ULL)
                	{
                		/* trim the size */
                		size = 0x80000000ULL - addr;
                	}

                	/*
                     *  It does not make any sense to consider regions of memory smaller than
                     * few megabytes. AROS kernel needs more.
                     *
                     */
                    if (size >= 8*1024*1024)
                    {
                    	unsigned long long temp_top = addr + size;

                    	if (temp_top > mem_upper)
                    		mem_upper = temp_top;
                    }
                }
    		}

    		len -= mmap->size+4;
    		mmap = (struct mb_mmap *)(mmap->size + (unsigned long)mmap+4);
    	}
    }
    else
    {
    	kprintf("[BOOT] No memory map. Assuming %p-%p region\n", mb->mem_lower << 10, (mb->mem_upper << 10) - 1);
    	mem_upper = mb->mem_upper << 10;
    	if (mem_upper > 0x80000000)
    		mem_upper = 0x80000000;

    	tag->ti_Tag = KRN_MEMLower;
    	tag->ti_Data = mb->mem_lower << 10;
    	tag++;
    	tag->ti_Tag = KRN_MEMUpper;
    	tag->ti_Data = mb->mem_upper << 10;
    	tag++;
    }

    kprintf("[BOOT] Topmost address for kernel: %p\n", mem_upper);

    /*
     * Since mem_upper is initialized, we got either the memory map from BIOS or just a rough information
     * about the memory bounds. Now, it is safe to continue :)
     */
    if (mem_upper)
    {
	void (*kernel_entry)();

    	mem_upper = mem_upper & ~4095;

    	unsigned long kernel_phys = 0x01000000;
    	unsigned long kernel_virt = kernel_phys;

    	unsigned long total_size_ro, total_size_rw;
    	uint32_t size_ro, size_rw;

    	/* Calculate total size of kickstart and modules */
    	total_size_ro = 0;
    	total_size_rw = 0;

    	if (mb->flags && MB_FLAGS_MODS)
    	{
    		int i;
    		struct mb_module *mod = (struct mb_module *)mb->mods_addr;
    		kprintf("[BOOT] GRUB has loaded %d files\n", mb->mods_count);

    		/* Go through the list of modules loaded by GRUB */
    		for (i=0; i < mb->mods_count; i++, mod++)
    		{
    			char *p = (char *)mod->mod_start;

    			getElfSize(p, &size_rw, &size_ro);

    			total_size_ro += (size_ro + 4095) & ~4095;
    			total_size_rw += (size_rw + 4095) & ~4095;
    		}
    	}

    	kernel_phys = mem_upper - total_size_ro - total_size_rw;
    	kernel_virt = kernel_phys;

    	kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);
    	kprintf("[BOOT] Virtual address of kernel: %p\n", kernel_virt);

    	initAllocator(kernel_phys, kernel_phys  + total_size_ro, kernel_virt - kernel_phys);

    	tag->ti_Tag = KRN_KernelLowest;
    	tag->ti_Data = kernel_phys;
    	tag++;

    	tag->ti_Tag = KRN_KernelHighest;
    	tag->ti_Data = kernel_phys + ((total_size_ro + 4095) & ~4095) + ((total_size_rw + 4095) & ~4095);
    	tag++;

    	if (mb->flags && MB_FLAGS_MODS)
    	{
    		int i;
    		struct mb_module *mod = (struct mb_module *)mb->mods_addr;

    		/* Go through the list of modules loaded by GRUB */
    		for (i=0; i < mb->mods_count; i++, mod++)
    		{
    			char *p = (char *)mod->mod_start;

    			loadElf(p);
    		}
    	}

    	tag->ti_Tag = KRN_KernelBss;
    	tag->ti_Data = (IPTR)tracker;
    	tag++;

    	kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)tag - (intptr_t)tags)/sizeof(struct TagItem));
    	kprintf("[BOOT] Bootstrap wasted %d bytes of memory for kernels use\n", mem_used()   );

    	tag->ti_Tag = TAG_DONE;
    	tag->ti_Data = 0;

    	/* Go to the kernel */
    	kprintf("[BOOT] Jumping to %p\n", kernel_virt);

	kernel_entry = (void *)kernel_virt;
	kernel_entry(tags, AROS_BOOT_MAGIC, addr);
    }

    die();
}

static void die()
{
	while (1) asm volatile("hlt");
}

