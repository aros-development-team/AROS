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

#include <bootconsole.h>
#include <stdlib.h>
#include <string.h>

#include "bootstrap.h"
#include "cpu.h"
#include "elfloader.h"
#include "support.h"
#include "vesa.h"

#define PAGE_MASK 0xFFF

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
    "kernel_bootstrap: movl $__stack+65536, %esp\n\t"   /* Load stack pointer */
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
    Global variables.

    Stack is used only by bootstrap and is located just "somewhere" in memory. The IGATES, TSS and GDT
    are stored in .bss.aros.tables section which is remapped to the begin of 0x00100000. Therefore this 
    tables may be re-used by the 64-bit kernel.
*/
extern char *_prot_lo, *_prot_hi;

/* Structures to be passed to the kickstart */
static struct mb_mmap MemoryMap[2];
static struct vbe_mode VBEModeInfo;
static struct vbe_controller VBEControllerInfo;
static unsigned char __bss_track[32768];
struct TagItem64 km[KRN__TAGCOUNT * 4];
struct TagItem64 *tag = &km[0];

static unsigned char __stack[65536] __attribute__((used));

static const char default_mod_name[] = "<unknown>";

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
static int find_modules(struct multiboot *mb, const struct module *m)
{
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
                const char *s = default_mod_name;
                struct module *mo = module_prepare(s, m, &count);

                mo->name = s;
                mo->address = (void*)mod->mod_start;

                D(kprintf("[BOOT] * ELF module %s @ %p\n", mo->name, mo->address));
            }
            else if (p[0] == 'P' && p[1] == 'K' && p[2] == 'G' && p[3] == 0x01)
            {
                /* 
                 * The loaded file is an PKG\0 archive. Scan it to find all modules which are 
                 * stored here.
                 */
                void *file = p + 8;

                D(kprintf("[BOOT] * package %s @ %p:\n", __bs_remove_path((char *)mod->cmdline), mod->mod_start));

                while (file < (void*)mod->mod_end)
                {
                    int len = LONG2BE(*(int *)file);
                    const char *s = __bs_remove_path(file+4);
                    struct module *mo = module_prepare(s, m, &count);

                    file += 5+len;
                    len = LONG2BE(*(int *)file);
                    file += 4;

                    mo->name    = s;
                    mo->address = file;
                    D(kprintf("[BOOT]   * PKG module %s @ %p\n", mo->name, mo->address));
                    
                    file += len;
                }
            }
        }
    }

    /* Return the real amount of modules to load */
    return count;
}

static void setupVESA(unsigned long vesa_base, char *vesa)
{
    D(kprintf("[BOOT] VESA Trampoline @ %p\n", vesa_base));

    if (vesa_base && vesa)
    {
        long x=0, y=0, d=0;
        long mode;

        void *vesa_start = &_binary_vesa_start;
        unsigned long vesa_size = (unsigned long)&_binary_vesa_size;
        vesa+=5;

        while (*vesa && *vesa != ',' && *vesa != 'x' && *vesa != ' ')
        {
            x = x*10 + *vesa++ - '0';
        }
        vesa++;
        while (*vesa && *vesa != ',' && *vesa != 'x' && *vesa != ' ')
        {
            y = y*10 + *vesa++ - '0';
        }
        vesa++;
        while (*vesa && *vesa != ',' && *vesa != 'x' && *vesa != ' ')
        {
            d = d*10 + *vesa++ - '0';
        }

        kprintf("[BOOT] setupVESA: vesa.bin @ %p [size=%d]\n", &_binary_vesa_start, &_binary_vesa_size);
#warning "TODO: Fix vesa.bin.o to support relocation (ouch)"
        memcpy((void *)0x1000, vesa_start, vesa_size);
        kprintf("[BOOT] setupVESA: Module copied to 0x1000\n");

        memcpy((void *)vesa_base, vesa_start, vesa_size);
        kprintf("[BOOT] setupVESA: vesa.bin relocated to trampoline @ %p\n", vesa_base);

        kprintf("[BOOT] setupVESA: BestModeMatch for %dx%dx%d = ",x,y,d);
        mode = findMode(x,y,d);

        getModeInfo(mode);
        memcpy(&VBEModeInfo, modeinfo, sizeof(struct vbe_mode));
        getControllerInfo();
        memcpy(&VBEControllerInfo, controllerinfo, sizeof(struct vbe_controller));

	/* Activate linear framebuffer is supported by the mode */
        if (VBEModeInfo.mode_attributes & VM_LINEAR_FB)
            mode |= VBE_MODE_LINEAR_FB;

        kprintf("%x\n",mode);
        if (setVbeMode(mode) == VBE_RC_SUPPORTED)
        {
            unsigned char palwidth = 6;

	    /* Try to switch palette width to 8 bits if possible */
            if (VBEControllerInfo.capabilities & VC_PALETTE_WIDTH)
                paletteWidth(0x0800, &palwidth);

	    /* Reinitialize our console */
	    con_InitVESA(VBEControllerInfo.version, &VBEModeInfo);

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
}

static void setupFB(struct multiboot *mb)
{
    if (mb->flags & MB_FLAGS_GFX)
    {
    	kprintf("[BOOT] Got VESA display mode 0x%x from the bootstrap\n", mb->vbe_mode);
    	/*
	 * We are already running in VESA mode set by the bootloader.
	 * Pass on the mode information to AROS.
	 */
	tag->ti_Tag = KRN_VBEModeInfo;
        tag->ti_Data = KERNEL_OFFSET | mb->vbe_mode_info;
        tag++;

        tag->ti_Tag = KRN_VBEControllerInfo;
        tag->ti_Data = KERNEL_OFFSET | mb->vbe_control_info;
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

static void prepare_message(void *kick_base)
{
    D(kprintf("[BOOT] Kickstart 0x%p - 0x%p (entry 0x%p), protection 0x%p - 0x%p\n", kernel_lowest(), kernel_highest(), kick_base,
    	      &_prot_lo, &_prot_hi));

    tag->ti_Tag = KRN_KernelBase;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)kick_base;
    tag++;

    tag->ti_Tag = KRN_KernelLowest;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)kernel_lowest();
    tag++;

    tag->ti_Tag = KRN_KernelHighest;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)kernel_highest();
    tag++;

    tag->ti_Tag = KRN_KernelBss;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)__bss_track;
    tag++;

    tag->ti_Tag = KRN_ProtAreaStart;
    tag->ti_Data = (unsigned long)&_prot_lo;
    tag++;

    tag->ti_Tag = KRN_ProtAreaEnd;
    tag->ti_Data = (unsigned long)&_prot_hi;
    tag++;

    tag->ti_Tag = TAG_DONE;
}

/*
    The entry point in C.
    
    The bootstrap routine has to load the kernel at 0x01000000, with RO sections growing up the memory and 
    RW sections stored beneath the 0x01000000 address. It is supposed to transfer the GRUB information further 
    into the 64-bit kernel.
    
    Thanks to smart linker scripts, the 64-bit kernel is pointed by the kernCode and kernData. They have to be
    copied to specified locations only.
    
    After kernel copying, the bootstrap has to check whether the modules have been loaded by GRUB. The modules 
    may be loaded separately, or as a collection in an IFF file. These modules have to be linked with the kernel 
    together. If any file is specified in both IFF file and list of separate modules, the copy in IFF has to
    be skipped.
*/
static void __attribute__((used)) __bootstrap(unsigned int magic, unsigned int addr)
{
    struct multiboot *mb = (struct multiboot *)addr;/* Multiboot structure from GRUB */
    struct module *mod = (struct module *)__stack;  /* The list of modules at the bottom of stack */
    char *vesa = NULL;
    int module_count = 0;
    unsigned long vesa_size = 0;
    unsigned long vesa_base = 0;
    void *kick_base = (void *)KERNEL_TARGET_ADDRESS;
    struct module *m;
    const char *cmdline = NULL;
    struct mb_mmap *mmap = NULL;
    unsigned long len = 0;

    /*
     * We are loaded at 0x200000, so we can use one megabyte at 0x100000
     * as our working area. Let's put console mirror there.
     * I hope every PC has memory at this address.
     */
    fb_Mirror = (char *)0x100000;
    con_InitMultiboot(mb);

    kprintf("[BOOT] Entered AROS Bootstrap @ %p\n", __bootstrap);
    D(kprintf("[BOOT] Stack @ %p, [%d bytes]\n", __stack, sizeof(__stack)));
    D(kprintf("[BOOT] Multiboot structure @ %p\n", mb));

    if (mb->flags & MB_FLAGS_CMDLINE)
    {
    	cmdline = (const char *)mb->cmdline;
    	D(kprintf("[BOOT] Command line @ %p : '%s'\n", mb->cmdline, cmdline));
    }

    if (cmdline)
    {
        char *kern = strstr(cmdline, "base_address=");
        
        if (kern)
        {
            unsigned long p = strtoul(&kern[13], NULL, 0);

            if (p >= 0x00200000)
            {
            	kick_base = (void *)p;
            	kprintf("[BOOT] Kernel base address changed to %p\n", kick_base);
            }
            else
	        kprintf("[BOOT] Kernel base address too low (%p). Keeping default.\n", p);
	}

    	vesa = strstr(cmdline, "vesa=");
    	if (vesa)
            vesa_size = (unsigned long)&_binary_vesa_size;

	tag->ti_Tag = KRN_CmdLine;
    	tag->ti_Data = KERNEL_OFFSET | (unsigned long)cmdline;
    	tag++;
    }

    if ((mb->flags & MB_FLAGS_MMAP))
    {
    	mmap = (struct mb_mmap *)mb->mmap_addr;
    	len  = mb->mmap_length;

#ifdef DEBUG_MEM
	kprintf("[BOOT] Memory map at 0x%p:\n", mmap);
        while (len >= sizeof(struct mb_mmap))
        {
#ifdef DEBUG_MEM_TYPE
            if (mmap->type == DEBUG_MEM_TYPE)
#endif
		kprintf("[BOOT] Type %d addr %llp len %llp\n", mmap->type, mmap->addr, mmap->len);

            len -= mmap->size+4;
            mmap = (struct mb_mmap *)(mmap->size + (unsigned long)mmap+4);
        }

    	mmap = (struct mb_mmap *)mb->mmap_addr;
    	len = mb->mmap_length;
#endif

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
        setupVESA(vesa_base, vesa);
    else
    	setupFB(mb);

    /* Setup stage - prepare the environment */
    setup_mmu(kick_base);

    kprintf("[BOOT] Loading kickstart...\n");
    set_base_address(kick_base, __bss_track);

    /* Search for external modules loaded by GRUB */
    module_count = find_modules(mb, mod);

    if (module_count == 0)
    	panic("No kickstart modules found, nothing to run");

    /* If any external modules are found, load them now */
    for (m = mod; module_count > 0; module_count--, m++)
    {
        kprintf("[BOOT] Loading %s... ", m->name);
        load_elf_file(m->address, 0);
        kprintf("\n");
    }

    /* Prepare the rest of boot taglist */
    prepare_message(kick_base);

#ifdef DEBUG_TAGLIST
    kprintf("[BOOT] Boot taglist:\n");
    for (tag = km; tag->ti_Tag != TAG_DONE; tag++)
    	kprintf("[BOOT] 0x%llp 0x%llp\n", tag->ti_Tag, tag->ti_Data);
#endif

    /* Jump to the kickstart */
    kick(km);

    panic("Failed to run the kickstart");
}
