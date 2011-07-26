/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: 32-bit bootstrap code used to boot the 64-bit AROS kernel.
    Lang: English
*/

//#define DEBUG
//#define DEBUG_MEM
//#define DEBUG_MEM_TYPE MMAP_TYPE_RAM
//#define DEBUG_TAGLIST

#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/multiboot2.h>
#include <asm/cpu.h>

#include <bootconsole.h>
#include <elfloader.h>
#include <stdlib.h>
#include <string.h>

#include "bootstrap.h"
#include "elfloader.h"
#include "support.h"
#include "vesa.h"

/*
 * The Multiboot-compliant header has to be within the first 4KB (1KB??) of ELF file, 
 * therefore it will be packed into the .aros.startup section. I hope, that turning debug on
 * will not shift it into some distinct location.
 * We support both legacy Multiboot v1 specification (for backwards compatibility with GRUB1)
 * and new Multiboot v2 specification (EFI-compatible).
 */

#define MB_FLAGS (MB_PAGE_ALIGN|MB_MEMORY_INFO|MB_VIDEO_MODE)

const struct multiboot_header __header __attribute__((used,section(".aros.startup"))) =
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
    32
};

struct my_mb2_header
{
    struct mb2_header 			header;
    struct mb2_header_tag_framebuffer	tag_fb;
    struct mb2_header_tag		tag_end;
};

const struct my_mb2_header __header_v2 __attribute__((used,section(".aros.startup"),aligned(8))) =
{
    {
    	MB2_MAGIC,
    	MB2_ARCH_I386,
    	sizeof(struct my_mb2_header),
    	-(MB2_MAGIC + MB2_ARCH_I386 + sizeof(struct my_mb2_header))
    },
    {
        MB2_HEADER_TAG_FRAMEBUFFER,
        MBTF_OPTIONAL,
        sizeof(struct mb2_header_tag_framebuffer),
        640,
        200,
        32
    },
    {
        MB2_HEADER_TAG_END,
        0,
        sizeof(struct mb2_header_tag)
    }
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

/*
 * Structures to be passed to the kickstart.
 * They are placed in protected area in order to guarantee that the kickstart
 * won't overwrite them while starting up.
 */
__attribute__((section(".bss.aros.tables"))) static unsigned char __bss_track[32768];
__attribute__((section(".bss.aros.tables"))) struct TagItem64 km[32];
__attribute__((section(".bss.aros.tables"))) struct mb_mmap MemoryMap[2];
__attribute__((section(".bss.aros.tables"))) struct vbe_mode VBEModeInfo;
__attribute__((section(".bss.aros.tables"))) struct vbe_controller VBEControllerInfo;

/* A pointer used for building a taglist */
struct TagItem64 *tag = &km[0];

/* Our modules list */
static struct ELFNode *firstMod = NULL;
static struct ELFNode *lastMod = (struct ELFNode *)&firstMod;

/* Our stack */
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
static struct ELFNode *module_prepare(const char *s)
{
    struct ELFNode *mo;

    if (s)
    {
    	/* Repeat for every module in the list */
    	for (mo = firstMod; mo; mo = mo->Next)
    	{
	    /* Module exists? Break here to allow overriding it */
            if (strcmp(s, mo->Name) == 0)
            	return mo;
        }
    }

    mo = __bs_malloc(sizeof(struct ELFNode));
    mo->Next = NULL;

    lastMod->Next = mo;
    lastMod = mo;

    return mo;
}

unsigned long AddModule(unsigned long mod_start, unsigned long mod_end, unsigned long end)
{
    char *p = (char *)mod_start;

    if (p[0] == 0x7f && p[1] == 'E' && p[2] == 'L' && p[3] == 'F')
    {
        /* 
         * The loaded file is an ELF object. It may be put directly into our list of modules.
         * Unfortunately GRUB doesn't give us names of loaded modules
         */
        struct ELFNode *mo = module_prepare(NULL);

        mo->Name = "Kickstart ELF";
        mo->eh = (void*)mod_start;

        D(kprintf("[BOOT] * ELF module %s @ %p\n", mo->Name, mo->eh));

	if (mod_end > end)
	    end = mod_end;
    }
    else if (p[0] == 'P' && p[1] == 'K' && p[2] == 'G' && p[3] == 0x01)
    {
        /* 
         * The loaded file is an PKG\0 archive. Scan it to find all modules which are 
         * stored here.
         */
        void *file = p + 8;

        D(kprintf("[BOOT] * package @ %p:\n", mod_start));

        while (file < (void*)mod_end)
        {
            int len = LONG2BE(*(int *)file);
            char *s = __bs_remove_path(file+4);
            struct ELFNode *mo = module_prepare(s);

            file += 5+len;
            len = LONG2BE(*(int *)file);
            file += 4;

            mo->Name = s;
            mo->eh = file;
            D(kprintf("[BOOT]   * PKG module %s @ %p\n", mo->Name, mo->eh));

            file += len;
        }

	if (mod_end > end)
	    end = mod_end;
    }
    else
       	kprintf("[BOOT] Unknown module 0x%p\n", p);

    return end;
}

/* Marks console mirror buffer as allocated */
void AllocFB(void)
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
    memcpy(tmp, VESA_START, vesa_size);

    D(kprintf("[BOOT] setupVESA: vesa.bin @ %p [size=%d]\n", &_binary_vesa_start, &_binary_vesa_size));
    memcpy(VESA_START, vesa_start, vesa_size);

    kprintf("[BOOT] setupVESA: BestModeMatch for %dx%dx%d = ",x,y,d);
    mode = findMode(x,y,d);

    /* Get information and copy it from 16-bit memory space to our 32-bit memory */
    getModeInfo(mode);
    memcpy(&VBEModeInfo, modeinfo, sizeof(struct vbe_mode));
    getControllerInfo();
    memcpy(&VBEControllerInfo, controllerinfo, sizeof(struct vbe_controller));

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
    memcpy(VESA_START, tmp, vesa_size);
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

int ParseCmdLine(const char *cmdline)
{
    if (cmdline)
    {
        /*
     	 * If vesa= option was given, set up the specified video mode explicitly.
     	 * Otherwise specify to AROS what has been passed to us by the bootloader.
     	 */
    	char *vesa = strstr(cmdline, "vesa=");

	tag->ti_Tag  = KRN_CmdLine;
    	tag->ti_Data = KERNEL_OFFSET | (unsigned long)cmdline;
    	tag++;

    	if (vesa)
    	{
    	    setupVESA(&vesa[5]);
    	    return 0;
    	}
    }
    return 1;
}

struct mb_mmap *mmap_make(unsigned long *len, unsigned long mem_lower, unsigned long long mem_upper)
{
    D(kprintf("[BOOT] No memory map supplied by the bootloader, using defaults\n"));

    MemoryMap[0].size = 20;
    MemoryMap[0].addr = 0;
    MemoryMap[0].len  = mem_lower;
    MemoryMap[0].type = MMAP_TYPE_RAM;

    MemoryMap[1].size = 20;
    MemoryMap[1].addr = 0x100000;
    MemoryMap[1].len  = mem_upper;
    MemoryMap[1].type = MMAP_TYPE_RAM;
    
    *len = sizeof(MemoryMap);
    return MemoryMap;
}

static void prepare_message(unsigned long kick_start, unsigned long kick_base, void *kick_end, void *DebugInfo_ptr)
{
    D(kprintf("[BOOT] Kickstart 0x%p - 0x%p (base 0x%p), protection 0x%p - 0x%p\n", kick_start, kick_end, kick_base,
    	      &_prot_lo, &_prot_hi));

    tag->ti_Tag  = KRN_KernelBase;
    tag->ti_Data = KERNEL_OFFSET | kick_base;
    tag++;

    tag->ti_Tag  = KRN_KernelLowest;
    tag->ti_Data = KERNEL_OFFSET | kick_start;
    tag++;

    tag->ti_Tag  = KRN_KernelHighest;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)kick_end;
    tag++;

    tag->ti_Tag  = KRN_KernelBss;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)__bss_track;
    tag++;

    tag->ti_Tag  = KRN_DebugInfo;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)DebugInfo_ptr;
    tag++;

    tag->ti_Tag  = KRN_ProtAreaStart;
    tag->ti_Data = (unsigned long)&_prot_lo;
    tag++;

    tag->ti_Tag  = KRN_ProtAreaEnd;
    tag->ti_Data = (unsigned long)&_prot_hi;
    tag++;

    tag->ti_Tag = TAG_DONE;
}

void panic(const char *str)
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
static void __bootstrap(unsigned int magic, void *mb)
{
    struct mb_mmap *mmap = NULL;
    unsigned long len = 0;
    unsigned long ro_size = 0;
    unsigned long rw_size = 0;
    unsigned long ksize;
    unsigned long mod_end = 0;
    unsigned long kbase = 0;
    unsigned long kstart = 0;
    void *kend = NULL;
    kernel_entry_fun_t kentry = NULL;
    struct ELF_ModuleInfo *kdebug = NULL;

    /*
     * This will set fb_Mirror address to start of our working memory.
     * We don't know its size yet, we will allocate it later.
     */
    fb_Mirror = __bs_malloc(0);

    switch(magic)
    {
    case MB_STARTUP_MAGIC:
	/* Parse multiboot v1 information */
	mod_end = mb1_parse(mb, &mmap, &len);
	break;

    case MB2_STARTUP_MAGIC:
    	/* Parse multiboot v2 information */
    	mod_end = mb2_parse(mb, &mmap, &len);
	break;

    default:
    	/* What to do here? We have no console... Die silently... */
    	return;
    }

    D(kprintf("[BOOT] Modules end at 0x%p\n", mod_end));
    if (!mod_end)
    {
    	panic("No kickstart modules found, nothing to run");
    }

    tag->ti_Tag = KRN_MMAPAddress;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long)mmap;
    tag++;

    tag->ti_Tag = KRN_MMAPLength;
    tag->ti_Data = len;
    tag++;

    /* Setup stage - prepare the environment */
    setup_mmu();

    /* Count kickstart size */
    if (!GetKernelSize(firstMod, &ro_size, &rw_size, NULL))
    {
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
    {
    	panic("Failed to find %u bytes for the kickstart.\n"
    	      "Your system doesn't have enough memory.");
    }

    kprintf("[BOOT] Loading kickstart, data 0x%p, code 0x%p...\n", kstart, kbase);

    if (!LoadKernel(firstMod, (void *)kbase, (void *)kstart, (struct KernelBSS *)__bss_track, 8, &kend, &kentry, &kdebug))
    {
        panic("Failed to load the kickstart");
    }

    /* Prepare the rest of boot taglist */
    prepare_message(kstart, kbase, kend, kdebug);

#ifdef DEBUG_TAGLIST
    kprintf("[BOOT] Boot taglist:\n");
    for (tag = km; tag->ti_Tag != TAG_DONE; tag++)
    	kprintf("[BOOT] 0x%llp 0x%llp\n", tag->ti_Tag, tag->ti_Data);
#endif

    /* Jump to the kickstart */
    kick(kentry, km);

    panic("Failed to run the kickstart");
}

void Hello(void)
{
    kprintf  ("[BOOT] Entered AROS Bootstrap @ 0x%p\n", __bootstrap);
    D(kprintf("[BOOT] Stack @ 0x%p, [%d bytes]\n", __stack, sizeof(__stack)));
}
