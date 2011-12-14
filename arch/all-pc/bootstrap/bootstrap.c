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
//#include <asm/cpu.h>
#include <asm/x86_64/cpu.h>

#include <bootconsole.h>
#include <elfloader.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bootstrap.h"
#include "elfloader.h"
#include "support.h"
#include "vesa.h"

#ifdef MULTIBOOT_64BIT
#define DEF_SYSBASE 8
#else
#define DEF_SYSBASE 4
#endif

/*
 * The Multiboot-compliant header has to be within the first 4KB (1KB??) of ELF file, 
 * therefore it will be packed into the .aros.startup section. I hope, that turning debug on
 * will not shift it into some distinct location.
 * We support both legacy Multiboot v1 specification (for backwards compatibility with GRUB1)
 * and new Multiboot v2 specification (EFI-compatible).
 */

#ifdef BOOTLOADER_grub2
#define MB_FLAGS (MB_PAGE_ALIGN|MB_MEMORY_INFO|MB_VIDEO_MODE)
#else
/*
 * This makes the bootstrap working with GRUB 1 which doesn't
 * support MB_VIDEO_MODE. However, this disables framebuffer support in the GRUB,
 * so this would not work on Mac.
 */
#define MB_FLAGS (MB_PAGE_ALIGN|MB_MEMORY_INFO)
#endif

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

struct ar_header {
    char name[16];      /* ASCII, 0x20 padded */
    char timestamp[12]; /* Decimal */
    char owner[6];      /* Decimal */
    char group[6];      /* Decimal */
    char mode[8];       /* Octal */
    char size[10];      /* Decimal */
    char magic[2];      /* 0x60 0x0a */
};

static const void *ar_data(const struct ar_header *header)
{
    unsigned int pad = 0;

    if (header->magic[0] != 0x60 || header->magic[1] != 0x0a)
        return NULL;

    /* BSD extension - name is appended after the header */
    if (memcmp(header->name, "#1/", 3) == 0) {
        pad = strtoul(&header->name[3], NULL, 10);
    }

    return ((const char *)(&header[1])) + pad;
}


static const struct ar_header *ar_next(const struct ar_header *header)
{
    unsigned int len;
    char size[11];

    if (header->magic[0] != 0x60 || header->magic[1] != 0x0a)
        return NULL;

    /* All 10 characters may be used! */
    memcpy(size, header->size, 10);
    size[10] = 0;

    len = strtoul(size, NULL, 10);
    if (len & 1)
        len++;

    /* BSD extension - name is appended after the header */
    if (memcmp(header->name, "#1/", 3) == 0) {
        int extra = strtoul(&header->name[3], NULL, 10);
        if (extra & 1)
            extra++;
        len += extra;
    }

    return (((const void *)(&header[1])) + len);
}

static char *ar_name(const struct ar_header *header, const struct ar_header *longnames)
{
    const char *name;
    char *tmp;
    int len;

    name = &header->name[0];

    if (name[0] == '/' && isdigit(name[1]) && longnames != NULL) {
        /* GNU style long names */
        const char *cp;
        int offset = strtoul(&name[1], NULL, 10);

        name = ar_data(longnames) + offset;
        for (len = 0, cp = name; *cp != '\n'; len++, cp++) {
            if (*cp == '/')
                break;
        }
    } else if (name[0] == '#' && name[1] == '1' && name[2] == '/') {
        /* BSD style long names */
        len = strtoul(&name[3], NULL, 0);
        name = ((char *)(&header[1]));
    } else {
        const char *cp;

        for (len = 0, cp = name; *cp != ' '; len++, cp++)
            if (longnames != NULL && *cp == '/')
                break;
    }

    tmp = __bs_malloc(len + 1);
    memcpy(tmp, name, len);
    tmp[len] = 0;

    return tmp;
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
    else if (memcmp(p,"!<arch>\n",8) == 0) {
        const struct ar_header *file;
        char *name;
        const struct ar_header *longnames = NULL;

        /* ar(1) archive */
        D(kprintf("[BOOT] * archive @ %p:\n", mod_start));

        /* Look for the GNU extended name section */
        for (file = (void *)(p + 8); (void *)file < (void*)mod_end; file = ar_next(file))
        {
            name = ar_name(file, NULL);
            if (strcmp(name, "//") == 0)
            {
                longnames = file;
                break;
            }
        }
        D(kprintf("[BOOT] *   longnames @ %p\n", longnames));

        for (file = (void *)(p + 8); (void *)file < (void*)mod_end; file = ar_next(file))
        {
            const char *data = ar_data(file);
            char *s = ar_name(file, longnames);

            if (memcmp(data,"\177ELF",4) == 0) {
                struct ELFNode *mo = module_prepare(s);

                mo->Name = s;
                mo->eh = (void *)data;
                D(kprintf("[BOOT] *   ar module %s @ %p\n", mo->Name, mo->eh));
            } else {
                D(kprintf("[BOOT] *   Ignored @ %p (%s)\n", file, s));
            }
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
#ifdef DEBUG_MEM
    struct mb_mmap *mm2;
#endif

    /*
     * This will set fb_Mirror address to start of our working memory.
     * We don't know its size yet, we will allocate it later.
     */
    fb_Mirror = __bs_malloc(0);

#ifdef MULTIBOOT_64BIT
    /*
     * tell the CPU that we will support SSE. We do it here because x86-64 compiler
     * with -m32 switch will use SSE for operations on long longs.
     */                                                                                    
    wrcr(cr4, rdcr(cr4) | (3 << 9));                                                                                               
    /* Clear the EM and MP flags of CR0 */                                                                                         
    wrcr(cr0, rdcr(cr0) & ~6);
#endif

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

#ifdef DEBUG_MEM
    ksize = len;
    mm2   = mmap;

    kprintf("[BOOT] Memory map contents:\n", mmap);
    while (ksize >= sizeof(struct mb_mmap))
    {
#ifdef DEBUG_MEM_TYPE
        if (mm2->type == DEBUG_MEM_TYPE)
#endif
	    kprintf("[BOOT] Type %d addr %llp len %llp\n", mm2->type, mm2->addr, mm2->len);

        ksize -= mm2->size+4;
        mm2 = (struct mb_mmap *)(mm2->size + (unsigned long)mm2 + 4);
    }
#endif

    D(kprintf("[BOOT] Modules end at 0x%p\n", mod_end));
    if (!firstMod)
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
    while (len >= sizeof(struct mb_mmap))
    {
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
	    if ((start <= 0x100000000ULL - ksize) && (end >= mod_end + ksize))
	    {
		unsigned long size;

	    	if (start < mod_end)
	    	    start = mod_end;

	    	if (end > 0x100000000ULL)
	    	    end = 0x100000000ULL;

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

    if (!LoadKernel(firstMod, (void *)kbase, (void *)kstart, (struct KernelBSS *)__bss_track, DEF_SYSBASE, &kend, &kentry, &kdebug))
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
