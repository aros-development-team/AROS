/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: ELF64 loader extracted from our internal_load_seg_elf in dos.library.
    Lang: English
*/

/* #define DEBUG */

#include <aros/kernel.h>
#include <dos/elf.h>
#include <libraries/debug.h>

#include <string.h>

#include "bootstrap.h"
#include "elfloader.h"
#include "support.h"

/*
 * These two pointers are used by the ELF loader to claim memory ranges for both
 * the RW sections (.data, .bss and such) and RO sections (.text, .rodata....) of executable.
 * Keeping both areas of memory separate reduces the memory waste when more modules are
 * loaded. Moreover, the whole RO range may be marked for MMU as read-only at once.
 */
void *ptr_ro = (void *)KERNEL_TARGET_ADDRESS;
void *ptr_rw = (void *)KERNEL_TARGET_ADDRESS;
unsigned long long DebugInfo_ptr = 0;

static unsigned long long SysBase_ptr = 0;
static struct ELF_ModuleInfo64 *prev_mod = (struct ELF_ModuleInfo64 *)&DebugInfo_ptr;

struct _bss_tracker {
    unsigned long long addr;
    unsigned long long len;
} *bss_tracker;

void *kernel_lowest()
{
    return ptr_rw;
}

void *kernel_highest()
{
    return ptr_ro;
}

void set_base_address(void *ptr, void *tracker)
{
    ptr_ro = ptr_rw = ptr;    
    bss_tracker = (struct _bss_tracker *)tracker;
}

/*
 * read_block function copies the range of memory within ELF file to any specified location.
 */
static int read_block(void *file, long offset, void *dest, long length)
{
    __bs_memcpy(dest, file + offset, length);
    return 1;
}

/*
 * Test for correct ELF header here
 */
static const char *check_header(struct elfheader *eh)
{
    if (eh->ident[0] != 0x7f || eh->ident[1] != 'E' ||
        eh->ident[2] != 'L'  || eh->ident[3] != 'F')
        return "Not an ELF object";

    if (eh->type != ET_REL || eh->machine != AROS_ELF_MACHINE)
        return "Wrong object type or wrong architecture";

    return NULL;
}

/*
 * Get the memory for chunk and load it
 */
static int load_hunk(void *file, struct sheader *sh)
{
    void *ptr;

    /* empty chunk? Who cares :) */
    if (!sh->size)
    	return 1;

    /* Allocate a chunk with write access - take aligned memory beneath the RO kernel */
    if (sh->flags & SHF_WRITE)
    {
        D(kprintf("[ELF Loader] RW chunk (%d bytes, align=%d) @ ", (unsigned int)sh->size, (unsigned int)sh->addralign));
        ptr = (void*)(((unsigned long)ptr_rw - (unsigned long)sh->size - (unsigned long)sh->addralign + 1) & ~((unsigned long)sh->addralign-1));
        ptr_rw = ptr;
    }
    else
    {
        /* Read-Only mode? Get the memory from the kernel space, align it accorting to the demand */
        D(kprintf("[ELF Loader] RO chunk (%d bytes, align=%d) @ ", (unsigned int)sh->size, (unsigned int)sh->addralign));
        ptr_ro = (char *)(((unsigned long)ptr_ro + (unsigned long)sh->addralign - 1) & ~((unsigned long)sh->addralign-1));
        ptr = ptr_ro;
        ptr_ro = ptr_ro + sh->size;
    }
    D(kprintf("%p\n", (unsigned int)ptr));

    sh->addr = (long)ptr;
    
    /* copy block of memory from ELF file if it exists */
    if (sh->type != SHT_NOBITS)
        return read_block(file, sh->offset, (void *)((unsigned long)sh->addr), sh->size);
    else
    {
        memset(ptr, 0, sh->size);
        bss_tracker->addr = KERNEL_OFFSET | (unsigned long)ptr;
        bss_tracker->len = sh->size;
        bss_tracker++;
        bss_tracker->addr = 0;
        bss_tracker->len = 0;
    }
    
    return 1;
}

static void *copy_data(const void *src, void *addr, unsigned long len)
{
    __bs_memcpy(addr, src, len);
    return addr + len;
}

/* Perform relocations of given section */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx, struct sheader *symtab_shndx, unsigned long long virt)
{
    struct sheader *shsymtab;
    struct symbol *symtab;
    struct relo *rel;
    unsigned int numrel, i;
    struct sheader *shrel   = &sh[shrel_idx];
    struct sheader *toreloc = &sh[SHINDEX(shrel->info)];
    struct symbol *SysBase_sym = NULL;

    /*
     * Ignore relocs if the target section has no allocation. that can happen
     * eg. with a .debug PROGBITS and a .rel.debug section
     */
    if (!(toreloc->flags & SHF_ALLOC))
    	return 1;

    shsymtab = &sh[SHINDEX(shrel->link)];
    symtab   = (struct symbol *)((unsigned long)shsymtab->addr);
    rel      = (struct relo *)((unsigned long)shrel->addr);
    numrel   = (unsigned long)shrel->size / (unsigned long)shrel->entsize;

    D(kprintf("[ELF Loader] performing %d relocations, target address 0x%016llX%08X\n", numrel, virt));

    for (i=0; i<numrel; i++, rel++)
    {
        struct symbol *sym = &symtab[ELF_R_SYM(rel->info)];
        unsigned int shindex = sym->shindex;
        char *name = (char *)(unsigned long)sh[shsymtab->link].addr + sym->name;
        void *p = (void *)(unsigned long)toreloc->addr + rel->offset;
        unsigned long long s;

        if (shindex == SHN_XINDEX)
        {
            if (symtab_shndx == NULL)
            {
                kprintf("[ELF Loader] got symbol with shndx 0xfff, but there's no symtab shndx table\n");
                return 0;
            }
            shindex = ((unsigned int *)(unsigned long)symtab_shndx->addr)[ELF_R_SYM(rel->info)];
        }

        switch (sym->shindex)
        {
            case SHN_UNDEF:
                kprintf("[ELF Loader] Undefined symbol '%s' while relocating the section '%s'\n",
			name, (unsigned long)sh[eh->shstrndx].addr + toreloc->name);
                return 0;

            case SHN_COMMON:
                kprintf("[ELF Loader] COMMON symbol '%s' while relocating the section '%s'\n",
                      	name, (unsigned long)sh[eh->shstrndx].addr + toreloc->name);

                return 0;

            case SHN_ABS:
                if (SysBase_sym == NULL)
                {
                    if (strcmp(name, "SysBase") == 0)
                        SysBase_sym = sym;
                }

                if (SysBase_sym == sym)
                {
		    if (!SysBase_ptr)
		    {
		    	SysBase_ptr = 8; /* Default global SysBase address is 8 on 64-bit machines */
		    	D(kprintf("[ELF Loader] SysBase pointer set to default 0x%016llx\n", SysBase_ptr));
		    }

	    	    s = SysBase_ptr;
		}
                else
		    s = sym->value;
                break;

            default:
                s = (unsigned long long)sh[sym->shindex].addr + virt + sym->value;

		if (!SysBase_ptr)
		{
		    /*
		     * The first global data symbol named SysBase becomes global SysBase.
		     * The idea behind: the first module (kernel.resource) contains global
		     * SysBase variable and all other modules are linked to it.
		     */
                    if (sym->info == ELF_S_INFO(STB_GLOBAL, STT_OBJECT))
                    {
                    	if (strcmp(name, "SysBase") == 0)
                    	{
                    	    SysBase_ptr = s;
                    	    D(kprintf("[ELF Loader] SysBase pointer set to 0x%016llx\n", SysBase_ptr));
                    	}
                    }
                }
        }

        switch (ELF_R_TYPE(rel->info))
        {
            case R_X86_64_64: /* 64bit direct/absolute */
                *(unsigned long long *)p = s + rel->addend;
                break;

            case R_X86_64_PC32: /* PC relative 32 bit signed */
                *(unsigned long *)p = s + rel->addend - ((unsigned long)p + virt);
                break;

            case R_X86_64_32:
                *(unsigned long *)p = (unsigned long long)s + (unsigned long long)rel->addend;
                break;

            case R_X86_64_32S:
                *(signed long *)p = (signed long long)s + (signed long long)rel->addend;
                break;

            case R_X86_64_NONE: /* No reloc */
                break;

            default:
                kprintf("[ELF Loader] Unrecognized relocation type %d %d\n", i, ELF_R_TYPE(rel->info));
                return 0;
        }
    }
    return 1;
}

int count_elf_size(struct module *n, unsigned long *ro_size, unsigned long *rw_size)
{
    unsigned long ksize = 0;
    unsigned long rwsize = 0;
    unsigned short i;
    const char *err;

    D(kprintf("[ELF Loader] Checking module %s...\n", n->name));

    /* Check the header of ELF file */
    err = check_header(n->eh);
    if (err)
    {
    	kprintf("[ELF Loader] %s: %s\n", n->name, err);
    	return 0;
    }

    /* Locate section headers */
    n->sh = (void *)n->eh + n->eh->shoff;

    /*
     * Debug data for the module includes:
     * - Module descriptor (struct ELF_ModuleInfo)
     * - ELF file header
     * - ELF section header
     * - File name
     * - One empty pointer for alignment
     */
    ksize += (sizeof(struct ELF_ModuleInfo64) + sizeof(struct elfheader) + n->eh->shnum * n->eh->shentsize
    	     + strlen(n->name) + sizeof(unsigned long long));

    /* Go through all sections and calculate kernel size */
    for (i = 0; i < n->eh->shnum; i++)
    {
	/* Ignore sections with zero lengths */
	if (!n->sh[i].size)
	    continue;
	
	/*
	 * We will load:
	 * - Actual code and data (allocated sections)
	 * - String tables (for debug data)
	 * - Symbol tables (for debug data)
	 */
	if ((n->sh[i].flags & SHF_ALLOC) || (n->sh[i].type == SHT_STRTAB) || (n->sh[i].type == SHT_SYMTAB))
	{
	    /* Add maximum space for alignment */
	    unsigned long s = n->sh[i].size + n->sh[i].addralign - 1;

	    if (n->sh[i].flags & SHF_WRITE)
		rwsize += s;
	    else
		ksize += s;
	}
    }

    /* We sum up lengths */
    *ro_size += ksize;
    *rw_size += rwsize;

    return 1;
}

int load_elf_file(struct module *n, unsigned long long virt)
{
    struct elfheader *eh = n->eh;
    struct sheader *sh = n->sh;
    long i;
    int addr_displayed = 0;
    struct ELF_ModuleInfo64 *mod;

    D(kprintf("[ELF Loader] Loading ELF module from address %p\n", file));

    /* Iterate over the section header in order to prepare memory and eventually load some hunks */
    for (i=0; i < eh->shnum; i++)
    {
        /* Load allocated sections, symbol and string tables */
        if ((sh[i].flags & SHF_ALLOC)  || (sh[i].type == SHT_STRTAB) || (sh[i].type == SHT_SYMTAB))
        {
            /* Yup, it does. Load the hunk */
            if (!load_hunk(n->eh, &sh[i]))
            {
                kprintf("[ELF Loader] Error at loading of the hunk!\n");
            }
#ifndef DEBUG
            else if (!addr_displayed)
            {
                kprintf("0x%016llX\n", sh[i].addr);
                addr_displayed = 1;
            }
#endif
        }
    }

    /* For every loaded section perform the relocations */
    for (i=0; i < eh->shnum; i++)
    {
        if (sh[i].type == AROS_ELF_REL && sh[sh[i].info].addr)
        {
            sh[i].addr = (unsigned long)eh + sh[i].offset;

            /* FIXME: Implement full support for SHN_XINDEX (locate the appropriate section) */
            if (!sh[i].addr || !relocate(eh, sh, i, NULL, virt))
            {
                kprintf("[ELF Loader] Relocation error!\n");
                return 0;
            }
            /*
             * Flush relocs after they are processed, in order not to pass
             * bogus sections to debug.library
             */
            sh[i].addr = 0;
        }
    }

    /* Align our pointer */
    ptr_ro = (char *)(((unsigned long)ptr_ro + sizeof(unsigned long long)) & ~(sizeof(unsigned long long) - 1));

    /* Allocate module descriptor */
    mod = ptr_ro;
    ptr_ro += sizeof(struct ELF_ModuleInfo64);
    mod->Next = 0;
    mod->Type = DEBUG_ELF;

    /* Copy ELF header */
    mod->eh  = (unsigned long)ptr_ro;
    ptr_ro = copy_data(&eh, ptr_ro, sizeof(struct elfheader));

    /* Copy section header */
    mod->sh = (unsigned long)ptr_ro;
    ptr_ro = copy_data(sh, ptr_ro, eh->shnum * eh->shentsize);

    /* Copy module name */
    mod->Name = (unsigned long)ptr_ro;
    ptr_ro = copy_data(n->name, ptr_ro, strlen(n->name) + 1);

    /* Link the module descriptor with previous one */
    prev_mod->Next = (unsigned long)mod;
    prev_mod = mod;

    return 1;
}
