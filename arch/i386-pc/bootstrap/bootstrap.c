/*
 * bootstrap.c
 *
 *  Created on: Dec 1, 2009
 *      Author: misc
 */

#define DEBUG 0

#include "elf.h"
#include "bootstrap.h"
#include "memtest.h"
#include "screen.h"
#include "vesa.h"

#include <aros/multiboot.h>
#include <dos/elf.h>
#include <asm/io.h>
#include <aros/kernel.h>
#include <inttypes.h>
#include <utility/tagitem.h>

#include <bootconsole.h>
#include <stdlib.h>
#include <string.h>

#define DMEM(x) x

/*
    The Multiboot-compliant header has to be within the first 4KB (1KB??) of ELF file,
    therefore it will be packed into the .aros.startup section. I hope, that turning debug on
    will not shift it into some distinct location.
*/
#define MB_FLAGS 0x00000003

static const struct multiboot_header __header __attribute__((used,section(".aros.startup"))) =
{
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

__attribute__((section(".bss.aros.tables"))) static struct TagItem tags[128];
__attribute__((section(".bss.aros.tables"))) static struct mb_mmap MemoryMap[2];

static struct TagItem *tag = &tags[0];
extern char _end;

/* Marks console mirror buffer as allocated */
static void AllocFB(void)
{
    if (scr_Type == SCR_GFX)
    {
	D(kprintf("[BOOT] Allocating %u bytes for console mirror (%ux%u)\n", scr_Width * scr_Height, scr_Width, scr_Height));

    	mem_malloc(scr_Width * scr_Height);
    }
}

/* C-code entry of bootstrap */
static void __attribute__((used)) __bootstrap(unsigned int magic, unsigned int addr)
{
    struct multiboot *mb = (struct multiboot *)addr;/* Multiboot structure from GRUB */
    unsigned long mem_lower = 0;
    unsigned long mem_upper = 0;
    unsigned long mmap_len = 0;
    struct mb_mmap *mmap = NULL;
    struct vbe_controller *vctl = NULL;
    struct vbe_mode *vmode = NULL;
    long modenum = 0;
    char *cmdline = NULL;
    unsigned long start_addr = (unsigned long)&_end;

    /*
     * This will set fb_Mirror address to start of our working memory.
     * We don't know its size yet, we will allocate it later.
     */
    fb_Mirror = mem_malloc(0);
    con_InitMultiboot((struct multiboot *)addr);
    AllocFB();

    kprintf("[BOOT] Entered AROS Bootstrap @ %p\n", __bootstrap);
    kprintf("[BOOT] Stack @ %p, [%d bytes]\n",__stack, BOOT_STACK_SIZE);

    /* Disable interrupts on the XT-PIC directly */
    outb(0xff, 0x21);
    outb(0xff, 0xa1);

    if (mb->flags & MB_FLAGS_LDRNAME)
    {
    	tag->ti_Tag  = KRN_BootLoader;
    	tag->ti_Data = mb->loader_name;
    	tag++;
    }

    if (mb->cmdline)
    {
	cmdline = (char *)mb->cmdline;
    	kprintf("[BOOT] Command line: %s\n", cmdline);

    	tag->ti_Tag = KRN_CmdLine;
    	tag->ti_Data = mb->cmdline;
    	tag++;
    }

    if (mb->mmap_length)
    {
    	kprintf("[BOOT] BIOS provides memory map\n");

    	mmap_len = mb->mmap_length;
    	mmap = (struct mb_mmap *)mb->mmap_addr;
    }

    if (mb->flags & MB_FLAGS_MEM)
    {    
        kprintf("[BOOT] Low memory %u KB, upper memory %u KB\n", mb->mem_lower, mb->mem_upper);

	mem_lower = mb->mem_lower << 10;
	mem_upper = mb->mem_upper << 10;

    	tag->ti_Tag = KRN_MEMLower;
    	tag->ti_Data = mb->mem_lower << 10;
    	tag++;
    	tag->ti_Tag = KRN_MEMUpper;
    	tag->ti_Data = mb->mem_upper << 10;
    	tag++;
    }
 
    if (!mmap)
    {
	kprintf("[BOOT] No memory map supplied by the bootloader, using defaults\n");

	if ((!mem_lower) && (!mem_upper))
	{
	    unsigned long high_end;

	    kprintf("[BOOT] No memory information from the bootloader, probing memory...\n");

	    /* Determine topmost address of high memory */
	    high_end = RamCheck(start_addr, 0x80000000);

	    mem_lower = 640 << 10; /* Assume 640KB of low memory */
	    mem_upper = high_end - 0x100000;
	}

	MemoryMap[0].size      = 20;
	MemoryMap[0].addr      = 0;
	MemoryMap[0].addr_high = 0;
	MemoryMap[0].len       = mem_lower;
	MemoryMap[0].len_high  = 0;
	MemoryMap[0].type      = MMAP_TYPE_RAM;

	MemoryMap[1].size      = 20;
	MemoryMap[1].addr      = 0x100000;
	MemoryMap[1].addr_high = 0;
	MemoryMap[1].len       = mem_upper;
	MemoryMap[1].len_high  = 0;
	MemoryMap[1].type      = MMAP_TYPE_RAM;

	mmap_len = sizeof(MemoryMap);
	mmap = MemoryMap;
    }

    /* To simplify things down, memory map is mandatory for AROS */
    tag->ti_Tag = KRN_MMAPAddress;
    tag->ti_Data = (intptr_t)mmap;
    tag++;
    tag->ti_Tag = KRN_MMAPLength;
    tag->ti_Data = mmap_len;
    tag++;
    
    /* Calculate total size of kickstart and modules */
    if (mb->flags & MB_FLAGS_MODS)
    {
	void (*kernel_entry)();
	unsigned long kernel_phys = 0;
	unsigned long total_size_ro = 0;
	unsigned long total_size_rw = 0;
	unsigned long total_size;
	int i;
    	struct mb_module *mod = (struct mb_module *)mb->mods_addr;

    	kprintf("[BOOT] GRUB has loaded %d files\n", mb->mods_count);

    	/* Go through the list of modules loaded by GRUB */
    	for (i=0; i < mb->mods_count; i++)
    	{
	    uint32_t size_ro, size_rw;

    	    getElfSize((char *)mod->mod_start, &size_rw, &size_ro);

	    if (mod->mod_end > start_addr)
		start_addr = mod->mod_end + 1;

	    mod++;

    	    total_size_ro += (size_ro + 4095) & ~4095;
    	    total_size_rw += (size_rw + 4095) & ~4095;
    	}

	/* 8 KB is boot-time memory for the kickstart */
	total_size = total_size_ro + total_size_rw + 8192;
	kprintf("[BOOT] Calculated kickstart size: %lu bytes\n", total_size);

    	/* Go through entire list of free RAM */
    	while (mmap_len >= sizeof(struct mb_mmap))
    	{
    	    if (mmap->type == MMAP_TYPE_RAM)
    	    {
                /*
                 * Check whether the memory region starts in the first 2GB of address space.
                 * Higher areas will not be considered here, as they might potentially
                 * conflict with PCI address space. We do not want to deal with such
                 * trouble here, in this tiny bootstrap code.
                 */
                if ((!mmap->addr_high) && (mmap->addr < 0x80000000))
                {
		    unsigned long start = mmap->addr;
		    unsigned long end   = mmap->addr + mmap->len;
		    
                    /* Now, check whether the topmost address exceeds the 2GB border. */
                    if (mmap->len_high || (end > 0x80000000))
                    {
                	/* If yes, trim the size */
			end = 0x80000000;
		    }

		    DMEM(kprintf("[MMAP] Memory 0x%p - 0x%p\n", start, end));

		    /* start_addr must be inside the region */
		    if (end > start_addr)
		    {
			/* We can't place anything below start_addr */
			if (start < start_addr)
			    start = start_addr;

			/* Page-align the region */
			start = (start + 4095) & ~4095;
			end  &= ~4095;

			/* Remember the highest area whose size fits */
			if (end - start >= total_size)
			    kernel_phys = start;
		    }
    		}
	    }

    	    mmap_len -= mmap->size+4;
    	    mmap = (struct mb_mmap *)(mmap->size + (unsigned long)mmap+4);
    	}

    	if (!kernel_phys)
	{
	    kprintf("Failed to find RAM region for the kickstart!\n");
	    kprintf("Not enough memory in the machine!\n");
	    die();
	}

    	kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);

    	initAllocator(kernel_phys, kernel_phys + total_size_ro, 0);

    	tag->ti_Tag  = KRN_KernelLowest;
    	tag->ti_Data = kernel_phys;
    	tag++;

    	tag->ti_Tag  = KRN_KernelHighest;
    	tag->ti_Data = kernel_phys + total_size;
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

    	tag->ti_Tag  = KRN_KernelBss;
    	tag->ti_Data = (IPTR)tracker;
    	tag++;

	if (cmdline)
	{
	    unsigned char palette = setupVesa(cmdline, &modenum);

	    if (palette)
	    {
		/* Reinitialize our console */
		mem_free();
		fb_Mirror = mem_malloc(0);
		con_InitVESA(controllerinfo->version, modeinfo);
		AllocFB();

		/*
		 * We have already used loaded modules, so we can just
		 * use our VESA data located in low memory.
		 * The kickstart will relocate them before doing anything else.
		 */
		vctl  = controllerinfo;
		vmode = modeinfo;

		tag->ti_Tag  = KRN_VBEPaletteWidth;
		tag->ti_Data = palette;
		tag++;
	    }
	}

	if (vmode && vctl)
	{
	    kprintf("[BOOT] Detected VESA graphics mode %lu\n", modenum);

	    tag->ti_Tag  = KRN_VBEModeInfo;
            tag->ti_Data = (unsigned long)vmode;
            tag++;

            tag->ti_Tag  = KRN_VBEControllerInfo;
            tag->ti_Data = (unsigned long)vctl;
            tag++;

            tag->ti_Tag  = KRN_VBEMode;
            tag->ti_Data = modenum;
            tag++;
	}

    	kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)tag - (intptr_t)tags)/sizeof(struct TagItem));
    	kprintf("[BOOT] Bootstrap wasted %d bytes of memory for kernels use\n", mem_used()   );

    	tag->ti_Tag = TAG_DONE;

    	/* Go to the kernel */
    	kprintf("[BOOT] Jumping to %p\n", kernel_phys);

	kernel_entry = (void *)kernel_phys;
	kernel_entry(tags, AROS_BOOT_MAGIC);
    }
    else
	kprintf("[BOOT] No kickstart modules loaded, nothing to run\n");

    die();
}

static void die()
{
	while (1) asm volatile("hlt");
}

