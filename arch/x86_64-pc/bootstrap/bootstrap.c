/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: 32-bit bootstrap code used to boot the 64-bit AROS kernel.
    Lang: English
*/

#define DEBUG
//#define DEBUG_MEM
//#define DEBUG_MEM_TYPE MMAP_TYPE_RAM
//#define DEBUG_TAGLIST

#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <asm/cpu.h>

#include <bootconsole.h>
#include <stdlib.h>
#include <string.h>

#include "bootstrap.h"
#include "elfloader.h"
#include "support.h"
#include "vesa.h"

/*
    The Multiboot-compliant header has to be within the first 4KB (1KB??) of ELF file, 
    therefore it will be packed into the .aros.startup section. I hope, that turning debug on
    will not shift it into some distinct location.
*/

#define MB_FLAGS (MB_PAGE_ALIGN|MB_MEMORY_INFO|MB_VIDEO_MODE)

static const struct multiboot_header __header __attribute__((used,section(".aros.startup"))) =
{
    MB_MAGIC,
    MB_FLAGS,
    -(MB_MAGIC + MB_FLAGS),
    0,
    0,
    0,
    0,
    0,
    1,	/* We prefer text mode, but will accept also framebuffer */
    640,
    200,
    32,
};

/*
    First portion of code, called from GRUB dirrectly. It has to set the stack up, disable interrupts
    and jump to the regular C code
*/
asm("	.text\n\t"
    "	.globl kernel_bootstrap\n\t"
    "	.type  kernel_bootstrap,@function\n"
    "kernel_bootstrap: movl $__stack + 65536, %esp\n\t"		/* Load stack pointer */
    "	pushl %ebx\n\t"                                 /* store parameters passed by GRUB in registers */
    "	pushl %eax\n\t"
    "	pushl $0\n\t"
    "	cld\n\t"
    "	cli\n\t"                                        /* Lock interrupts (by flag)... */
    "	movb $-1,%al\n\t"
    "	outb %al, $0x21\n\t"                            /* And disable them physically */
    "	outb %al, $0xa1\n\t"
    "	jmp __bootstrap"
);

/*
 * Global variables.
 *
 * Stack is used only by bootstrap and is located just "somewhere" in memory.
 * Boot taglist, BSS list, GDT and MMU tables (see cpu.c) are stored in .bss.aros.tables
 * section which is remapped to the begin of 0x00100000. Therefore these data
 * may be re-used by the 64-bit kickstart (we pass this area using KRN_ProtAreaStart
 * and KRN_ProtAreaEnd).
 * It's up to the kickstart to preserve data pointed to by boot taglist (memory map,
 * drive tables, VBE structures, etc), because they may originate from the machine's
 * firmware and not from us.
*/
extern char *_prot_lo, *_prot_hi;

/* Structures to be passed to the kickstart */
static unsigned char __bss_track[32768] __attribute__((section(".bss.aros.tables")));
struct TagItem64 km[32] __attribute__((section(".bss.aros.tables")));
static struct mb_mmap MemoryMap[2];
static struct vbe_mode VBEModeInfo;
static struct vbe_controller VBEControllerInfo;

/* A pointer used for building a taglist */
struct TagItem64 *tag = &km[0];

static unsigned char __stack[65536] __attribute__((used));

/*
 * External modules.
 * 
 * If GRUB has loaded any external modules, they will be loaded here. The modules to be loaded
 * are stored in a list. If a module with given name is loaded more than once, the last version
 * loaded by GRUB is used.
 * 
 * Once the list is ready, the modules will be loaded and linked with kernel one after another. Please
 * note that no information exchange between the modules is allowed. Each of them is absolutely 
 * independent and has to care about itself (it has to clear its own .bss region itself). Kernel knows
 * about all modules only thanks to the list of resident modules, which is created during a memory scan
 * over the whole kernel.
 * 
 * Theoretically, such behaviour can guarantee us, that the GPL'ed modules do not conflict with kernel
 * and may be "part" of it, but since I am not a lawyer, I may be wrong.
 */

/*
 * Find the storage for module with given name. If such module already exists, return a pointer to it,
 * so that it can be overridden by the new one. Otherwise, alloc space for new module.
 */
static struct module *module_prepare(const char *s, const struct module *m, int *count)
{
    struct module *mo = (struct module *)m;
    int c = *count;
    
    /* Repeat for every module in the list */
    while (c>0)
    {
        /* Module exists? Break here to allow overriding it */
#if 0
// FIXME: we do not know the names of ELF modules
        if (strcmp(s, mo->name) == 0)
            break;
#endif
        
        /* Iterate... */
        c--;
        mo++;
    }
    
    /* If the module has not been found on the list, increase the counter */
    if (c==0) *count = *count + 1;
    
    return mo;
}

/*
 * Search for modules
 */
static int find_modules(struct multiboot *mb, const struct module *m, unsigned long *endPtr)
{
    unsigned long end = 0;
    int count = 0;

    /* Are there any modules at all? */
    if (mb->flags && MB_FLAGS_MODS)
    {
        int i;
        struct mb_module *mod = (struct mb_module *)mb->mods_addr;
        D(kprintf("[BOOT] GRUB has loaded %d files\n", mb->mods_count));

        /* Go through the list of modules loaded by GRUB */
        for (i=0; i < mb->mods_count; i++, mod++)
        {
            char *p = (char *)mod->mod_start;

            if (p[0] == 0x7f && p[1] == 'E' && p[2] == 'L' && p[3] == 'F')
            {
                /* 
                 * The loaded file is an ELF object. It may be put directly into our list of modules
                 */
                const char *name = __bs_remove_path((char *)mod->cmdline);
                struct module *mo = module_prepare(name, m, &count);

		/* GRUB doesn't give us names of loaded modules */
                mo->name = "Kickstart ELF";
                mo->eh = (void*)mod->mod_start;

                D(kprintf("[BOOT] * ELF module %s @ %p\n", mo->name, mo->eh));

		if (mod->mod_end > end)
		    end = mod->mod_end;
            }
            else if (p[0] == 'P' && p[1] == 'K' && p[2] == 'G' && p[3] == 0x01)
            {
                /* 
                 * The loaded file is an PKG\0 archive. Scan it to find all modules which are 
                 * stored here.
                 */
                void *file = p + 8;

                D(kprintf("[BOOT] * package @ %p:\n", mod->mod_start));

                while (file < (void*)mod->mod_end)
                {
                    int len = LONG2BE(*(int *)file);
                    const char *s = __bs_remove_path(file+4);
                    struct module *mo = module_prepare(s, m, &count);

                    file += 5+len;
                    len = LONG2BE(*(int *)file);
                    file += 4;

                    mo->name = s;
                    mo->eh = file;
                    D(kprintf("[BOOT]   * PKG module %s @ %p\n", mo->name, mo->eh));
                    
                    file += len;
                }

		if (mod->mod_end > end)
		    end = mod->mod_end;
            }
            else
            	kprintf("[BOOT] Unknown module 0x%p\n", p);
        }
    }

    *endPtr = end;
    /* Return the real amount of modules to load */
    return count;
}

/* Marks console mirror buffer as allocated */
static void AllocFB(void)
{
    if (scr_Type == SCR_GFX)
    {
	D(kprintf("[BOOT] Allocating %u bytes for console mirror (%ux%u)\n", scr_Width * scr_Height, scr_Width, scr_Height));

    	__bs_malloc(scr_Width * scr_Height);
    }
}

static void setupVESA(char *vesa)
{
    long x=0, y=0, d=0;
    unsigned char palwidth = 6;
    long mode;
    short res;

    void *vesa_start = &_binary_vesa_start;
    unsigned long vesa_size = (unsigned long)&_binary_vesa_size;
    void *tmp = __bs_malloc(vesa_size);

    if (!tmp)
    {
    	kprintf("[BOOT] VESA mode setup failed, not enough working memory\n");

    	return;
    }

    x = strtoul(vesa, &vesa, 10);
    vesa++;

    y = strtoul(vesa, &vesa, 10);
    vesa++;
    
    d = strtoul(vesa, &vesa, 10);

    /*
     * 16-bit VBE trampoline is needed only once only here, so
     * we can simply copy it to some address, do what we need, and
     * then forget.
     * However we must keep in mind that low memory can be occupied by
     * something useful, like kickstart modules or boot information.
     * So we preserve our region and put it back when we are done.
     */

    D(kprintf("[BOOT] Backing up low memory, buffer at 0x%p\n", tmp));
    __bs_memcpy(tmp, VESA_START, vesa_size);

    D(kprintf("[BOOT] setupVESA: vesa.bin @ %p [size=%d]\n", &_binary_vesa_start, &_binary_vesa_size));
    __bs_memcpy(VESA_START, vesa_start, vesa_size);

    kprintf("[BOOT] setupVESA: BestModeMatch for %dx%dx%d = ",x,y,d);
    mode = findMode(x,y,d);

    /* Get information and copy it from 16-bit memory space to our 32-bit memory */
    getModeInfo(mode);
    __bs_memcpy(&VBEModeInfo, modeinfo, sizeof(struct vbe_mode));
    getControllerInfo();
    __bs_memcpy(&VBEControllerInfo, controllerinfo, sizeof(struct vbe_controller));

    /* Activate linear framebuffer is supported by the mode */
    if (VBEModeInfo.mode_attributes & VM_LINEAR_FB)
        mode |= VBE_MODE_LINEAR_FB;

    kprintf("%x\n",mode);

    res = setVbeMode(mode);
    if (res == VBE_RC_SUPPORTED)
    {
	/* Try to switch palette width to 8 bits if possible */
        if (VBEControllerInfo.capabilities & VC_PALETTE_WIDTH)
            paletteWidth(0x0800, &palwidth);
    }

    /* Put memory back and reset memory allocator */
    __bs_memcpy(VESA_START, tmp, vesa_size);
    __bs_free();

    if (res == VBE_RC_SUPPORTED)
    {
    	/* Reinitialize our console */
    	fb_Mirror = __bs_malloc(0);
    	con_InitVESA(VBEControllerInfo.version, &VBEModeInfo);
    	AllocFB();

        tag->ti_Tag = KRN_VBEModeInfo;
        tag->ti_Data = KERNEL_OFFSET | (unsigned long)&VBEModeInfo;
        tag++;

        tag->ti_Tag = KRN_VBEControllerInfo;
        tag->ti_Data = KERNEL_OFFSET | (unsigned long)&VBEControllerInfo;
        tag++;

        tag->ti_Tag = KRN_VBEMode;
        tag->ti_Data = mode;
        tag++;

        tag->ti_Tag = KRN_VBEPaletteWidth;
        tag->ti_Data = palwidth;
        tag++;
    }

    kprintf("[BOOT] setupVESA: VESA setup complete\n");
}

static void setupFB(struct multiboot *mb)
{
    if (mb->flags & MB_FLAGS_GFX)
    {
    	kprintf("[BOOT] Got VESA display mode 0x%x from the bootstrap\n", mb->vbe_mode);
    	D(kprintf("[BOOT] Mode info 0x%p, controller into 0x%p\n", mb->vbe_mode_info, mb->vbe_control_info));

    	/*
	 * We are already running in VESA mode set by the bootloader.
	 * Pass on the mode information to AROS.
	 */
	tag->ti_Tag = KRN_VBEModeInfo;
        tag->ti_Data = mb->vbe_mode_info;
        tag++;

        tag->ti_Tag = KRN_VBEControllerInfo;
        tag->ti_Data = mb->vbe_control_info;
        tag++;

        tag->ti_Tag = KRN_VBEMode;
        tag->ti_Data = mb->vbe_mode;
        tag++;

        return;
    }

    if (mb->flags & MB_FLAGS_FB)
    {
    	kprintf("[BOOT] Got framebuffer display %dx%dx%d from the bootstrap\n",
    		mb->framebuffer_width, mb->framebuffer_height, mb->framebuffer_bpp);
	D(kprintf("[BOOT] Address 0x%llp, type %d, %d bytes per line\n", mb->framebuffer_addr, mb->framebuffer_type, mb->framebuffer_pitch));

	/*
	 * AROS VESA driver supports only RGB framebuffer because we are
	 * unlikely to have VGA palette registers for other cases.
	 * FIXME: we have some pointer to palette registers. We just need to
	 * pass it to the bootstrap and handle it there (how? Is it I/O port
	 * address or memory-mapped I/O address?)
	 */
    	if (mb->framebuffer_type != MB_FRAMEBUFFER_RGB)
    	    return;

	/*
    	 * We have a framebuffer but no VBE information.
    	 * Looks like we are running on EFI machine with no VBE support (Mac).
    	 * Convert framebuffer data to VBEModeInfo and hand it to AROS.
    	 */
    	VBEModeInfo.mode_attributes		= VM_SUPPORTED|VM_COLOR|VM_GRAPHICS|VM_NO_VGA_HW|VM_NO_VGA_MEM|VM_LINEAR_FB;
    	VBEModeInfo.bytes_per_scanline		= mb->framebuffer_pitch;
    	VBEModeInfo.x_resolution		= mb->framebuffer_width;
    	VBEModeInfo.y_resolution		= mb->framebuffer_height;
    	VBEModeInfo.bits_per_pixel		= mb->framebuffer_bpp;
    	VBEModeInfo.memory_model		= VMEM_RGB;
    	VBEModeInfo.red_mask_size		= mb->framebuffer_red_mask_size;
    	VBEModeInfo.red_field_position	        = mb->framebuffer_red_field_position;
    	VBEModeInfo.green_mask_size		= mb->framebuffer_green_mask_size;
    	VBEModeInfo.green_field_position	= mb->framebuffer_green_field_position;
    	VBEModeInfo.blue_mask_size		= mb->framebuffer_blue_mask_size;
    	VBEModeInfo.blue_field_position		= mb->framebuffer_blue_field_position;
	VBEModeInfo.phys_base			= mb->framebuffer_addr;
	VBEModeInfo.linear_bytes_per_scanline   = mb->framebuffer_pitch;
	VBEModeInfo.linear_red_mask_size	= mb->framebuffer_red_mask_size;
	VBEModeInfo.linear_red_field_position   = mb->framebuffer_red_field_position;
	VBEModeInfo.linear_green_mask_size	= mb->framebuffer_green_mask_size;
	VBEModeInfo.linear_green_field_position = mb->framebuffer_green_field_position;
	VBEModeInfo.linear_blue_mask_size	= mb->framebuffer_blue_mask_size;
	VBEModeInfo.linear_blue_field_position  = mb->framebuffer_blue_field_position;

	tag->ti_Tag = KRN_VBEModeInfo;
        tag->ti_Data = KERNEL_OFFSET | (unsigned long)&VBEModeInfo;
        tag++;
    }
}

static void prepare_message(unsigned long kick_start, unsigned long kick_base)
{
    D(kprintf("[BOOT] Kickstart 0x%p - 0x%p (entry 0x%p), protection 0x%p - 0x%p\n", kick_start, kernel_highest(), kick_base,
    	      &_prot_lo, &_prot_hi));

    tag->ti_Tag  = KRN_KernelBase;
    tag->ti_Data = KERNEL_OFFSET | kick_base;
    tag++;

    tag->ti_Tag  = KRN_KernelLowest;
    tag->ti_Data = KERNEL_OFFSET | kick_start;
    tag++;

    tag->ti_Tag  = KRN_KernelHighest;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)kernel_highest();
    tag++;

    tag->ti_Tag  = KRN_KernelBss;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)__bss_track;
    tag++;

    tag->ti_Tag  = KRN_DebugInfo;
    tag->ti_Data = KERNEL_OFFSET | DebugInfo_ptr;
    tag++;

    tag->ti_Tag  = KRN_ProtAreaStart;
    tag->ti_Data = (unsigned long)&_prot_lo;
    tag++;

    tag->ti_Tag  = KRN_ProtAreaEnd;
    tag->ti_Data = (unsigned long)&_prot_hi;
    tag++;

    tag->ti_Tag = TAG_DONE;
}

static void panic(const char *str)
{
    kprintf("%s\n", str);
    kprintf("*** SYSTEM PANIC!!! ***\n");

    for(;;)
    	HALT;
}

/*
 * The entry point in C.
 *
 * The bootstrap routine has to load the kickstart at 0x01000000, with RO sections growing up the memory and 
 * RW sections stored beneath the 0x01000000 address. It is supposed to transfer the GRUB information further 
 * into the 64-bit kickstart.
 *
 * The kickstart is assembled from modules which have been loaded by GRUB. The modules may be loaded separately,
 * or as a collection in PKG file. If some file is specified in both PKG file and list of separate modules, the
 * copy in PKG will be skipped.
*/
static void __attribute__((used)) __bootstrap(unsigned int magic, struct multiboot *mb)
{
    struct module *mod;
    char *vesa = NULL;
    int i;
    int module_count = 0;
    const char *cmdline = NULL;
    struct mb_mmap *mmap = NULL;
    unsigned long len = 0;
    unsigned long ro_size = 0;
    unsigned long rw_size = 0;
    unsigned long ksize;
    unsigned long mod_end = 0;
    unsigned long kbase = 0;
    unsigned long kstart = 0;

    /*
     * This will set fb_Mirror address to start of our working memory.
     * We don't know its size yet, we will allocate it later.
     */
    fb_Mirror = __bs_malloc(0);
    con_InitMultiboot(mb);

    kprintf("[BOOT] Entered AROS Bootstrap @ %p\n", __bootstrap);
    D(kprintf("[BOOT] Stack @ %p, [%d bytes]\n", __stack, sizeof(__stack)));
    D(kprintf("[BOOT] Multiboot structure @ %p\n", mb));

    /*
     * Now allocate our mirror buffer.
     * The buffer is used only by graphical console.
     */
    AllocFB();

    if (mb->flags & MB_FLAGS_CMDLINE)
    {
    	cmdline = (const char *)mb->cmdline;
    	D(kprintf("[BOOT] Command line @ %p : '%s'\n", mb->cmdline, cmdline));
    }

    if (cmdline)
    {
    	vesa = strstr(cmdline, "vesa=");

	tag->ti_Tag = KRN_CmdLine;
    	tag->ti_Data = KERNEL_OFFSET | (unsigned long)cmdline;
    	tag++;
    }

    if ((mb->flags & MB_FLAGS_MMAP))
    {
    	mmap = (struct mb_mmap *)mb->mmap_addr;
    	len  = mb->mmap_length;

	D(kprintf("[BOOT] Memory map at 0x%p, length %u\n", mmap, len));
    }

    if (mb->flags & MB_FLAGS_MEM)
    {
        D(kprintf("[BOOT] Low memory %u KB, upper memory %u KB\n", mb->mem_lower, mb->mem_upper));

    	if (!mmap)
    	{
    	    /*
    	     * To simplify things down, memory map is mandatory for our kickstart.
    	     * So we create an implicit one if the bootloader didn't supply it.
    	     */
            D(kprintf("[BOOT] No memory map supplied by the bootloader, using defaults\n"));

	    MemoryMap[0].size = 20;
            MemoryMap[0].addr = 0;
            MemoryMap[0].len  = mb->mem_lower << 10;
            MemoryMap[0].type = MMAP_TYPE_RAM;

            MemoryMap[1].size = 20;
            MemoryMap[1].addr = 0x100000;
            MemoryMap[1].len  = mb->mem_upper << 10;
            MemoryMap[1].type = MMAP_TYPE_RAM;

            mmap = MemoryMap;
            len = sizeof(MemoryMap);
        }

	/* Kickstart wants size in bytes */
    	tag->ti_Tag = KRN_MEMLower;
    	tag->ti_Data = mb->mem_lower << 10;
    	tag++;

	tag->ti_Tag = KRN_MEMUpper;
    	tag->ti_Data = mb->mem_upper << 10;
    	tag++;
    }

    if (!mmap)
    	panic("No memory information provided by the bootloader");

    tag->ti_Tag = KRN_MMAPAddress;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)mmap;
    tag++;

    tag->ti_Tag = KRN_MMAPLength;
    tag->ti_Data = len;
    tag++;

    /*
     * If vesa= option was given, set up the specified video mode explicitly.
     * Otherwise specify to AROS what has been passed to us by the bootloader.
     */
    if (vesa)
        setupVESA(&vesa[5]);
    else
    	setupFB(mb);

    /* Setup stage - prepare the environment */
    setup_mmu();

    /*
     * This will place list of modules in the end of our working memory.
     * It's safe to reserve zero bytes because we won't allocate anything else.
     */
    mod = __bs_malloc(0);

    /* Search for external modules loaded by GRUB */
    module_count = find_modules(mb, mod, &mod_end);

    if (module_count == 0)
    	panic("No kickstart modules found, nothing to run");

    D(kprintf("[BOOT] Modules end at 0x%p\n", mod_end));

    /* Count kickstart size */
    kprintf("[BOOT] Calculating kickstart size...\n");
    for (i = 0; i < module_count; i++)
    {
    	if (!count_elf_size(&mod[i], &ro_size, &rw_size))
    	    panic("Failed to determine kickstart size");
    }

    D(kprintf("[BOOT] Code %u, data %u\n", ro_size, rw_size));

    /*
     * Total kickstart size + alignment window (page size - 1) + some free space (512KB) for
     * boot-time memory allocator.
     * TODO: This is a temporary thing. Currently our kernel expects that it can use addresses beyond
     * KRN_KernelHighest to store boot-time private data, supervisor stack, segment descriptors, MMU stuff, etc.
     * The area is joined with read-only section because it's accessed only by supervisor-mode code
     * and can safely be marked as read-only for users.
     * Boot-time allocator needs to be smarter.
     */
    ksize = ro_size + rw_size + PAGE_SIZE - 1 + 0x80000;

    /* Now locate the highest appropriate region */
#ifdef DEBUG_MEM
    kprintf("[BOOT] Memory map contents:\n", mmap);
#endif
    while (len >= sizeof(struct mb_mmap))
    {
#ifdef DEBUG_MEM
#ifdef DEBUG_MEM_TYPE
        if (mmap->type == DEBUG_MEM_TYPE)
#endif
	    kprintf("[BOOT] Type %d addr %llp len %llp\n", mmap->type, mmap->addr, mmap->len);
#endif

        if (mmap->type == MMAP_TYPE_RAM)
        {
            unsigned long long start = mmap->addr;
            unsigned long long end = mmap->addr + mmap->len;

	    /*
	     * The region must be located in 32-bit memory and must not overlap
	     * our modules.
	     * Here we assume the following:
	     * 1. Multiboot data from GRUB is placed in low memory.
	     * 2. At least one module is placed in upper memory, above ourselves.
	     * 3. There's no usable space below our modules.
	     */
	    if ((start <= 0x100000000 - ksize) && (end >= mod_end + ksize))
	    {
		unsigned long size;

	    	if (start < mod_end)
	    	    start = mod_end;

	    	if (end > 0x100000000)
	    	    end = 0x100000000;

		/* Remember the region if it fits in */
		size = end - start;
		if (size >= ksize)
		{
		    /*
		     * We place .data section at the start of the region, followed by .code section
		     * at page-aligned 'kbase' address.
		     * There must be a space beyond kickstart's read-only section, because the kickstart
		     * will extend it in order to store boot-time configuration and own private data.
		     */
		    kstart = start;
		    kbase = start + rw_size;
		    kbase = (kbase + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	    	}
	    }
	}

        len -= mmap->size+4;
        mmap = (struct mb_mmap *)(mmap->size + (unsigned long)mmap+4);
    }

    if (!kbase)
    	panic("Failed to find %u bytes for the kickstart.\n"
    	      "Your system doesn't have enough memory.");

    kprintf("[BOOT] Loading kickstart, data 0x%p, code 0x%p...\n", kstart, kbase);
    if (!LoadKernel(kbase, kstart, __bss_track, 0, mod, module_count))
        panic("Failed to load the kickstart");

    /* Prepare the rest of boot taglist */
    prepare_message(kstart, kbase);

#ifdef DEBUG_TAGLIST
    kprintf("[BOOT] Boot taglist:\n");
    for (tag = km; tag->ti_Tag != TAG_DONE; tag++)
    	kprintf("[BOOT] 0x%llp 0x%llp\n", tag->ti_Tag, tag->ti_Data);
#endif

    /* Jump to the kickstart */
    kick((void *)kbase, km);

    panic("Failed to run the kickstart");
}
