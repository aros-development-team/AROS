/*
    Copyright (C) 2006-2009 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: 32-bit bootstrap code used to boot the 64-bit AROS kernel.
    Lang: English
*/

#define DEBUG
#define BOOTSTRAP

#include "../include/aros/kernel.h"

#include "bootstrap.h"
#include "multiboot.h"
#include "cpu.h"
#include "screen.h"
#include "elfloader.h"
#include "support.h"
#include "vesa.h"

#define PAGE_MASK 0xFFF

/*
    The Multiboot-compliant header has to be within the first 4KB (1KB??) of ELF file, 
    therefore it will be packed into the .aros.startup section. I hope, that turning debug on
    will not shift it into some distinct location.
*/
static const multiboot_header __header __attribute__((used,section(".aros.startup"))) = {
    MB_MAGIC, MB_FLAGS, -(MB_MAGIC+MB_FLAGS)    
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
static unsigned char __stack[65536] __attribute__((used));
static unsigned char __bss_track[32768] __attribute__((used,section(".bss.aros.tables")));

static struct {
    struct segment_desc seg0;      /* seg 0x00 */
    struct segment_desc super_cs;  /* seg 0x08 */
    struct segment_desc super_ds;  /* seg 0x10 */
} GDT __attribute__((used,aligned(128),section(".bss.aros.tables")));

/*
    The MMU pages and directories. They are stored at fixed location and may be either reused in the 
    64-bit kernel, or replaced by it. Four PDE directories (PDE2M structures) are enough to map whole
    4GB address space.
*/
static struct PML4E PML4[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDPE PDP[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDE2M PDE[4][512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));

static struct vbe_mode VBEModeInfo = {0, };
static struct vbe_controller VBEControllerInfo = {0, };

struct TagItem km[KRN__TAGCOUNT * 4] __attribute__((used, section(".bss.aros.tables")));

struct TagItem *tag = &km[0];

static struct {
    void *off;
    unsigned short seg;
} __attribute__((packed)) KernelTarget = { (void*)KERNEL_TARGET_ADDRESS, SEG_SUPER_CS };

/*
    32-bit pseudo-registers used to load the Global and Interrupt Descriptor Tables. They are used only once.
*/
const struct
{
    unsigned short size __attribute__((packed));
    unsigned int base __attribute__((packed));
} 
GDT_sel = {sizeof(GDT)-1, (unsigned int)&GDT};

static const char default_mod_name[] = "<unknown>";

/*
    Setting descriptor tables up. It is perhaps not wise to embed it into the function. Most likely the more
    effective solution would be to make a local copy of new descriptor table instead. But the code is more clear
    and takes effectively *no place* since it is loaded at bootup and removed later.
*/
static void setup_tables()
{
    kprintf("[BOOT] Setting up descriptor tables.\n");

    /* Supervisor segments */
    GDT.super_cs.type=0x1a;	/* code segment */
    GDT.super_cs.dpl=0;		/* supervisor level */
    GDT.super_cs.p=1;		/* present */
    GDT.super_cs.l=1;		/* long (64-bit) one */
    GDT.super_cs.d=0;		/* must be zero */
    GDT.super_cs.limit_low=0xffff;
    GDT.super_cs.limit_high=0xf;
    GDT.super_cs.g=1;
    
    GDT.super_ds.type=0x12;	/* data segment */
    GDT.super_ds.dpl=0;		/* supervisor level */
    GDT.super_ds.p=1;		/* present */
    GDT.super_ds.limit_low=0xffff;
    GDT.super_ds.limit_high=0xf;
    GDT.super_ds.g=1;
    GDT.super_ds.d=1;   
}

/*
    The 64-bit long mode may be activated only, when MMU paging is enabled. Therefore the basic
    MMU tables have to be prepared first. This routine uses 6 tables (2048 + 5 entries) in order
    to map the first 4GB of address space as user-accessible executable RW area.
    
    This mapping may be changed later by the 64-bit kernel, in order to provide separate address spaces,
    protect kernel from being overwritten and so on and so forth.
*/
static void setup_mmu()
{
    int i;
    struct PDE2M *pdes[] = { &PDE[0], &PDE[1], &PDE[2], &PDE[3] };
    
    kprintf("[BOOT] Mapping first 4G area with MMU\n");

    /* PML4 Entry - we need only the first out of 16 entries */
    PML4[0].p  = 1; /* present */
    PML4[0].rw = 1; /* read/write */
    PML4[0].us = 1; /* accessible for user */
    PML4[0].pwt= 0; /* write-through cache */
    PML4[0].pcd= 0; /* cache enabled */
    PML4[0].a  = 0; /* not yet accessed */
    PML4[0].mbz= 0; /* must be zero */
    PML4[0].base_low = (unsigned int)PDP >> 12;
    PML4[0].avl= 0;
    PML4[0].nx = 0;
    PML4[0].avail = 0;
    PML4[0].base_high = 0;

    /*
        PDP Entries. There are four of them used in order to define 2048 pages of 2MB each.
     */
    for (i=0; i < 4; i++)
    {
        int j;
	
        /* Set the PDP entry up and point to the PDE table */
        PDP[i].p  = 1;
        PDP[i].rw = 1;
        PDP[i].us = 1;
        PDP[i].pwt= 0;
        PDP[i].pcd= 0;
        PDP[i].a  = 0;
        PDP[i].mbz= 0;
        PDP[i].base_low = (unsigned int)pdes[i] >> 12;

        PDP[i].nx = 0;
        PDP[i].avail = 0;
        PDP[i].base_high = 0;

        for (j=0; j < 512; j++)
        {
            /* Set PDE entries - use 2MB memory pages, with full supervisor and user access */
	    
            struct PDE2M *PDE = pdes[i];
            PDE[j].p  = 1;
            PDE[j].rw = 1;
            PDE[j].us = 1;
            PDE[j].pwt= 0;  // 1
            PDE[j].pcd= 0;  // 1
            PDE[j].a  = 0;
            PDE[j].d  = 0;
            PDE[j].g  = 0;
            PDE[j].pat= 0;
            PDE[j].ps = 1;
            PDE[j].base_low = ((i << 30) + (j << 21)) >> 13;
            
            PDE[j].avail = 0;
            PDE[j].nx = 0;
            PDE[j].base_high = 0;
        }
    }
}

/*
 * This tiny procedure sets the complete 64-bit environment up - it loads the descriptors, 
 * enables 64-bit mode, loads MMU tables and through paging it activates the 64-bit long mode.
 * 
 * After that it is perfectly safe to jump into the pure 64-bit kernel.
 */
int leave_32bit_mode()
{
    unsigned int v1 = 0, v2 = 0, v3 = 0, v4 = 0;
    int retval = 0;
    asm volatile ("lgdt %0"::"m"(GDT_sel));

    cpuid(0x80000000, v1, v2, v3, v4);
    if (v1 > 0x80000000)
    { 
        cpuid(0x80000001, v1, v2, v3, v4);
        if (v4 & (1 << 29))
        {
            /* Enable PAE */
            wrcr(cr4, _CR4_PAE | _CR4_PGE);
            
            /* enable pages */
            wrcr(cr3, &PML4);
        
            /* enable long mode */
            rdmsr(EFER, &v1, &v2);
            v1 |= _EFER_LME;
            wrmsr(EFER, v1, v2);
        
            /* enable paging and activate long mode */
            wrcr(cr0, _CR0_PG | _CR0_PE);
            retval = 1;
        }
    }
    
    return retval;
}

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
        if (__bs_strncmp(s, mo->name, __bs_strlen(s)) == 0)
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

                D(kprintf("[BOOT] * package %s @ %p:\n", __bs_remove_path((char *)mod->string), mod->mod_start));

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

void setupVESA(unsigned long vesa_base, char *vesa)
{
    if ((vesa_base) && (vesa))
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
        __bs_memcpy((void *)0x1000, vesa_start, vesa_size);
        kprintf("[BOOT] setupVESA: Module copied to 0x1000\n");

        __bs_memcpy((void *)vesa_base, vesa_start, vesa_size);
        kprintf("[BOOT] setupVESA: vesa.bin relocated to trampoline @ %p\n", vesa_base);

        kprintf("[BOOT] setupVESA: BestModeMatch for %dx%dx%d = ",x,y,d);
        mode = findMode(x,y,d);

        getModeInfo(mode);
        __bs_memcpy(&VBEModeInfo, modeinfo, sizeof(struct vbe_mode));
        getControllerInfo();
        __bs_memcpy(&VBEControllerInfo, controllerinfo, sizeof(struct vbe_controller));

        if (VBEModeInfo.mode_attributes & 0x80)
            mode |= 0x4000;

        kprintf("%x\n",mode);
        if (setVbeMode(mode) == 0x004f)
        {
            unsigned char palwidth = 0;

            if (VBEControllerInfo.capabilities & 0x01)
                paletteWidth(0x0800, &palwidth);
            else
                palwidth = 6;

            tag->ti_Tag = KRN_VBEModeInfo;
            tag->ti_Data = KERNEL_OFFSET | (unsigned long long)&VBEModeInfo;
            tag++;

            tag->ti_Tag = KRN_VBEControllerInfo;
            tag->ti_Data = KERNEL_OFFSET | (unsigned long long)&VBEControllerInfo;
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

void change_kernel_address(const char *str)
{
    char *kern = __bs_strstr(str, "base_address=");
    if (kern)
    {
        unsigned long addr=0;
        unsigned char a;
        kern+=13;

        while (*kern && *kern != ' ' )
        {
            a = *kern++;

            if (a >= '0' && a <= '9')
                a -= '0';
            else if (a >= 'a' && a <= 'f')
                a -= 'a' - 10;
            else if (a >= 'A' && a <= 'F')
                a -= 'A' - 10;
            else break;
            
            addr = (addr << 4) + a;
        }

        if (addr >= 0x00200000)
        {
            kprintf("[BOOT] Kernel base address changed to %p\n", addr);

            KernelTarget.off = (void*)addr;
            set_base_address((void *)addr, __bss_track);
        }
        else
        {
            kprintf("[BOOT] Kernel base address too low (%p). Keeping default.\n", addr);
        }
    }
}

void prepare_message(struct multiboot *mb)
{
    /*km.GRUBData.low = mb; */

    tag->ti_Tag = KRN_GDT;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long long)&GDT;
    tag++;

    tag->ti_Tag = KRN_PL4;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long long)&PML4;
    tag++;

    tag->ti_Tag = KRN_KernelBase;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long long)KernelTarget.off;
    tag++;

    tag->ti_Tag = KRN_KernelLowest;
    tag->ti_Data = KERNEL_OFFSET | ((long)kernel_lowest() & ~4095);
    tag++;

    tag->ti_Tag = KRN_KernelHighest;
    tag->ti_Data = KERNEL_OFFSET | (((long)kernel_highest() + 4095) & ~4095);
    tag++;

    tag->ti_Tag = KRN_KernelBss;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long long)__bss_track;
    tag++;

    tag->ti_Tag = KRN_ProtAreaStart;
    tag->ti_Data = &_prot_lo;
    tag++;

    tag->ti_Tag = KRN_ProtAreaEnd;
    tag->ti_Data = &_prot_hi;
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
    char *vesa = __bs_strstr(mb->cmdline, "vesa=");
    int module_count = 0;
    unsigned long vesa_size = 0;
    unsigned long vesa_base = 0;
    struct module *m;

    clr();
//    kprintf("[BOOT] Entered AROS Bootstrap @ %p [asm stub @ %p].\n", __bootstrap, kernel_bootstrap);
    kprintf("[BOOT] Entered AROS Bootstrap @ %p\n", __bootstrap);
    D(kprintf("[BOOT] Multiboot structure @ %p\n", mb));
    D(kprintf("[BOOT] Command line '%s' @ %p\n", mb->cmdline, mb->cmdline));

    D(kprintf("[BOOT] Stack @ %p, [%d bytes]\n", __stack, sizeof(__stack)));

    set_base_address((void *)KERNEL_TARGET_ADDRESS, __bss_track);
    change_kernel_address(mb->cmdline);

    if (vesa)
    {
        vesa_size = (unsigned long)&_binary_vesa_size;
    }

    tag->ti_Tag = KRN_CmdLine;
    tag->ti_Data = KERNEL_OFFSET | (unsigned long long)(mb->cmdline);
    tag++;
    tag->ti_Tag = TAG_DONE;

    if ((mb->mmap_addr) && (mb->mmap_length))
    {
        tag->ti_Tag = KRN_MMAPLength;
        tag->ti_Data = mb->mmap_length;
        tag++;
        tag->ti_Tag = KRN_MMAPAddress;
        tag->ti_Data = KERNEL_OFFSET | mb->mmap_addr;
        tag++;
        tag->ti_Tag = TAG_DONE;

        /* Quickly locate a suitable region to use for lowpage memory */
        unsigned long len = mb->mmap_length;
        struct mb_mmap *mmap = (struct mb_mmap *)(KERNEL_OFFSET | mb->mmap_addr);

        while (len >= sizeof(struct mb_mmap))
        {
            if (mmap->type == MMAP_TYPE_RAM)
            {
                unsigned long addr = (mmap->addr_low | ((unsigned long)mmap->addr_high << 32));
                unsigned long size = (mmap->len_low | ((unsigned long)mmap->len_high << 32));

                if (addr < 0x00100000 && (addr+size) <= 0x00100000)
                {
                    D(kprintf("[BOOT] MMAP: Using Entry %p [%d bytes] for lowpages\n", addr, size));
                    tag->ti_Tag = KRN_MEMLower;
                    vesa_base = ((addr + size) - vesa_size) & ~PAGE_MASK;
                    D(kprintf("[BOOT] MMAP: Adjusted size for VESA Trampoline = %d\n", vesa_base - addr));
                    tag->ti_Data = ((vesa_base - 1)/1024);
                    tag++;
                    break;
                }
            }

            len -= mmap->size+4;
            mmap = (struct mb_mmap *)(mmap->size + (unsigned long)mmap+4);
        }
        tag->ti_Tag = TAG_DONE;
    }
    else
    {
        tag->ti_Tag = KRN_MEMLower;
        vesa_base = (mb->mem_lower - vesa_size) & ~PAGE_MASK;
        tag->ti_Data = ((vesa_base - 1)/1024);
        tag++;
        tag->ti_Tag = KRN_MEMUpper;
        tag->ti_Data = mb->mem_upper;
        tag++;
        tag->ti_Tag = TAG_DONE;     
    }

    if (vesa)
    {
        kprintf("[BOOT] VESA Trampoline @ %p\n", vesa_base);
        setupVESA(vesa_base, vesa);
    }

    /* Setup stage - prepare 64-bit environment */
    setup_tables();
    setup_mmu();

    /* Load the first ELF relocable object - the kernel itself */
    kprintf("[BOOT] Loading kernel\n");
    //load_elf_file(&_binary_aros_o_start, 0); //((unsigned long long)KERNEL_HIGH_OFFSET) << 39);

    /* Search for external modules loaded by GRUB */
    module_count = find_modules(mb, mod);

    /* If any external modules are found, load them now */
    for (m = mod; module_count > 0; module_count--, m++)
    {
        kprintf("[BOOT] Loading %s... ", m->name);
        load_elf_file(m->address, 0);
        kprintf("\n");
    }

    /*
     * Prepare machine to leave the 32-bit mode. Activate 64-bit mode.
     */
    prepare_message(mb);

    /*
     * And do a necessary long jump into the 64-bit kernel
     */
    kprintf("[BOOT] Leaving 32-bit environment."
        " LJMP $%x,$%p\n\n", SEG_SUPER_CS, KernelTarget.off);

    if (leave_32bit_mode())
        asm volatile("ljmp *%0"::"m"(KernelTarget),"D"(&km));
    else
    	kprintf("[BOOT] PANIC! Your CPU does not support Long Mode\n");

    /*
     * This code should not be reached at all.  
     */
    kprintf("HALT!\n");

    for(;;) asm volatile("hlt");
}
