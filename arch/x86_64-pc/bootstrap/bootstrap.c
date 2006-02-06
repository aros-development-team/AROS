/*
    Copyright (C) 2006 The AROS Development Team. All rights reserved.
    $Id:$
    
    Desc: 32-bit bootstrap code used to boot the 64-bit AROS kernel.
    Lang: English
*/

#include "bootstrap.h"
#include "multiboot.h"
#include "cpu.h"
#include "screen.h"
#include "elfloader.h"
#include "support.h"

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
static unsigned char __stack[65536] __attribute__((used));
static struct int_gate_64bit IGATES[256] __attribute__((used,aligned(256),section(".bss.aros.tables")));
static struct tss_64bit TSS __attribute__((used,aligned(128),section(".bss.aros.tables")));
static struct {
    struct segment_desc	seg0;       /* seg 0x00 */
    struct segment_desc super_cs;  /* seg 0x08 */
    struct segment_desc super_ds;  /* seg 0x10 */
    struct segment_desc user_cs32; /* seg 0x18 */
    struct segment_desc user_ds;   /* seg 0x20 */
    struct segment_desc user_cs;   /* seg 0x28 */
    struct segment_desc tss_low;   /* seg 0x30 */
    struct segment_ext 	tss_high;
} GDT __attribute__((used,aligned(128),section(".bss.aros.tables")));

/*
    The MMU pages and directories. They are stored at fixed location and may be either reused in the 
    64-bit kernel, or replaced by it. Four PDE directories (PDE2M structures) are enough to map whole
    4GB address space.
*/
static struct PML4E PML4[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDPE PDP[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDE2M PDE_1[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDE2M PDE_2[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDE2M PDE_3[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));
static struct PDE2M PDE_4[512] __attribute__((used,aligned(4096),section(".bss.aros.tables")));

/*
    32-bit pseudo-registers used to load the Global and Interrupt Descriptor Tables. They are used only once.
*/
const struct
{
    unsigned short size __attribute__((packed));
    unsigned int base __attribute__((packed));
} 
GDT_sel = {sizeof(GDT)-1, (unsigned int)&GDT},
IDT_sel = {sizeof(IGATES)-1, (unsigned int)IGATES};


/*
    Setting descriptor tables up. It is perhaps not wise to embed it into the function. Most likely the more
    effective solution would be to make a local copy of new descriptor table instead. But the code is more clear
    and takes effectively *no place* since it is loaded at bootup and removed later.
*/
static void setup_tables()
{
    kprintf("Setting descriptor tables up.\n");

    /* Supervisor segments */
    GDT.super_cs.type=0x18;	/* code segment */
    GDT.super_cs.dpl=0;		/* supervisor level */
    GDT.super_cs.p=1;		/* present */
    GDT.super_cs.l=1;		/* long (64-bit) one */
    GDT.super_cs.d=0;		/* must be zero */
    
    GDT.super_ds.type=0x10;	/* data segment */
    GDT.super_ds.p=1;		/* present */

    /* User mode segments */
    GDT.user_cs.type=0x18;	/* code segment */
    GDT.user_cs.dpl=3;		/* User level */
    GDT.user_cs.p=1;		/* present */
    GDT.user_cs.l=1;		/* long mode */
    GDT.user_cs.d=0;		/* must be zero */

    GDT.user_cs32.type=0x18;	/* code segment for legacy 32-bit code. NOT USED YET! */
    GDT.user_cs32.dpl=3;	/* user elvel */
    GDT.user_cs32.p=1;		/* present */
    GDT.user_cs32.l=0;		/* 32-bit mode */
    GDT.user_cs32.d=1;		/* 32-bit code */
    
    GDT.user_ds.type=0x10;	/* data segment */
    GDT.user_ds.p=1;		/* present */
    
    /* Task State Segment */
    GDT.tss_low.type=0x09;	/* 64-bit TSS */
    GDT.tss_low.limit_low=sizeof(TSS)-1;
    GDT.tss_low.base_low=((unsigned int)&TSS) & 0xffff;
    GDT.tss_low.base_mid=(((unsigned int)&TSS) >> 16) & 0xff;
    GDT.tss_low.dpl=3;		/* User mode task */
    GDT.tss_low.p=1;		/* present */
    GDT.tss_low.limit_high=((sizeof(TSS)-1) >> 16) & 0x0f;
    GDT.tss_low.base_high=(((unsigned int)&TSS) >> 24) & 0xff;
    GDT.tss_high.base_ext = 0;	/* is within 4GB :-D */
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
    struct PDE2M *pdes[] = { PDE_1, PDE_2, PDE_3, PDE_4 };
    
    kprintf("Mapping first 4G area with MMU\n");

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
            PDE[j].pwt= 1;
            PDE[j].pcd= 1;
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
 * enables 64-bit mode, loads MMU tables and trhough paging it activates the 64-bit long mode.
 * 
 * After that it is perfectly safe to jump into the pure 64-bit kernel.
 */
void leave_32bit_mode()
{
    unsigned int v1, v2;
    asm volatile ("lgdt %0\n\tlidt %1"::"m"(GDT_sel),"m"(IDT_sel));

    /* Enable PAE */
    wrcr(cr4, rdcr(cr4) | _CR4_PAE);
    
    /* enable pages */
    wrcr(cr3, PML4);

    /* enable long mode */
    rdmsr(EFER, &v1, &v2);
    v1 |= _EFER_LME;
    wrmsr(EFER, v1, v2);

    /* enable paging and activate long mode */
    wrcr(cr0, rdcr(cr0) | _CR0_PG);   
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
 * independent and has to care about itself (it has to clear it's own .bss region self). Kernel knows
 * about all modules only thanks to the list of resident modules, which is created during a memory scan
 * over the whole kernel.
 * 
 * Theoretically, such behaviour can guarantee us, that the GPL'ed modules do not conflict with kenrel
 * and may be "part" of it, but since I am not a lawyer, I may be wrong.
 */

/*
 * Find the storage for module with given name. If such module already exists, return a pointer to it,
 * so that it can be overrided by the new one. Otherwise, alloc space for new module.
 */
static struct module *module_prepare(const char *s, const struct module *m, int *count)
{
    struct module *mo = (struct module *)m;
    int c = *count;
    
    /* Repeat for every module in the list */
    while (c>0)
    {
        /* Module exists? Break here to allow overriding it */
        if (strncmp(s, mo->name, strlen(s)) == 0)
            break;
        
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
        kprintf("GRUB has loaded %d modules\n", mb->mods_count);
              
        /* Go through the list of modules loaded by GRUB */
        for (i=0; i < mb->mods_count; i++, mod++)
        {
            char *p = (char *)mod->mod_start;
            
            if (p[0] == 0x7f && p[1] == 'E' && p[2] == 'L' && p[3] == 'F')
            {
                /* 
                 * The loaded file is an ELF object. It may be put directly into our list of modules
                 */
                const char *s = remove_path((char*)mod->string);
                struct module *mo = module_prepare(s, m, &count);
                
                mo->name = s;
                mo->address = (void*)mod->mod_start;

                kprintf("** module %s, addr=%p (direct)\n", mo->name, mo->address);
            }
            else if (p[0] == 'P' && p[1] == 'K' && p[2] == 'G' && p[3] == 0x00)
            {
                /* 
                 * The loaded file is an PKG\0 archive. Scan it to find all modules which are 
                 * stored here.
                 */
                void *file = p + 4;

                while (file < (void*)mod->mod_end)
                {
                    int len = LONG2BE(*(int *)file);
                    const char *s = remove_path(file+4);
                    struct module *mo = module_prepare(s, m, &count);
                        
                    file += 5+len;
                    len = LONG2BE(*(int *)file);
                    file += 4;
                    
                    mo->name    = s;
                    mo->address = file;
                    kprintf("** module %s, addr=%p (from package)\n", mo->name, mo->address);
                    
                    file += len;
                }
            }
        }
    }

    /* Return the real amount of modules to load */
    return count;
}

/*
    The entry point in C.
    
    The bootstrap routine has to load the kernel at 0x01000000, with RO sections growing up the memory and 
    RW sections stored beneath the 0x01000000 address. It is supposed to transfer the GRUB information further 
    into the 64-bit kernel.
    
    Thanks to smart linker scripts, the 64-bit kernel is pointed by the kernCode and kernData. They have to be
    copied to specified locations only.
    
    After kernel copying, the bootstrap has to check whether the modules have been loaded by GRUB. The modules 
    may be loaded separately, or as a collection in IFF file. This modules have to be linked with the kernel 
    together. If any file is specified in both IFF file and list of separate modules, the copy in IFF has to
    be skipped.
*/
static void __attribute__((used)) __bootstrap(unsigned int magic, unsigned int addr)
{  
    struct multiboot *mb = (struct multiboot *)addr;/* Multiboot structure from GRUB */
    int module_count = 0;
    struct module *mod = (struct module *)__stack;  /* The list of modules at the bottom of stack */
    
    clr();
    kprintf("AROS Bootstrap.\n");
    kprintf("GRUB data at %p\n", addr);
    
    /* Test playground - the code will be copied to safe area later */
    if (mb->flags && MB_FLAGS_MMAP)
    {
        	int len = mb->mmap_length;
        	struct mb_mmap *mmap = (struct mb_mmap *)(mb->mmap_addr);
        	kprintf("GRUB provides the following memory map:\n");
        	
        	while(len >= sizeof(struct mb_mmap))
        	{
        	    kprintf("  %p%p - %p%p: %d\n", mmap->addr_high, mmap->addr_low, mmap->len_high, mmap->len_low, mmap->type);
        	    len -= mmap->size+4;
        	    mmap = (struct mb_mmap *)(mmap->size + (unsigned int)mmap+4);
        }
    }
    
    /* Setup stage - prepare 64-bit environment */
    setup_tables();
    setup_mmu();
    
    /* Load the first ELF relocable object - the kernel itself */
    load_elf_file(&_binary_aros_o_start);
    
    /* Search for external modules loaded by GRUB */
    module_count = find_modules(mb, mod);
    
    /* If any external modules are found, load them now */
    if (module_count > 0)
    {
        struct module *m;
        for (m = mod; module_count > 0; module_count--, m++)
        {
            load_elf_file(m->address);
        }
    }
    
    /*
     * Prepare machine to leave the 32-bit mode. Activate 64-bit mode.
     */
    leave_32bit_mode(); 
    /*
     * And do a necessary long jump into the 64-bit kernel
     */
    ljmp(SEG_SUPER_CS, KERNEL_TARGET_ADDRESS);
    
    /*
     * This code should not be reached at all.  
     */
    kprintf("HALT!\n");
    
    for(;;) asm volatile("hlt");
}
