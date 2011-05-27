/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: ELF64 loader extracted from our internal_load_seg_elf in dos.library.
    Lang: English
*/

/* #define DEBUG */

#include <aros/kernel.h>
#include <dos/elf.h>

#include <string.h>

#include "elfloader.h"
#include "bootstrap.h"
#include "support.h"

/*
 * This two pointers are used by the ELF loader to claim for memory ranges for both
 * the RW sections (.data, .bss and such) and RO sections (.text, .rodata....) of executable.
 * Keeping both areas of memory separate reduces the memory waste when more modules are
 * loaded. Moreover, the whole RO range may be marked for MMU as read-only at once. 
 */
char *ptr_ro = (char*)KERNEL_TARGET_ADDRESS;
char *ptr_rw = (char*)KERNEL_TARGET_ADDRESS;
unsigned long long SysBase_ptr = 0;

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
    __bs_memcpy(dest, (void *)((long)file + offset), length);
    return 1;
}

/*
 * load_block returns a pointer to the memory location within ELF file.
 */
static void *load_block(void *file, long offset, long length)
{
    return (void*)((long)file + offset);
}

/*
 * Test for correct ELF header here
 */
static int check_header(struct elfheader *eh)
{
    if
    (
        eh->ident[0] != 0x7f ||
        eh->ident[1] != 'E'  ||
        eh->ident[2] != 'L'  ||
        eh->ident[3] != 'F'
    )
    {
        kprintf("[ELF Loader] Not an ELF object\n");
        return 0;
    }

    if (eh->type != ET_REL || eh->machine != AROS_ELF_MACHINE)
    {
        kprintf("[ELF Loader] Wrong object type or wrong architecture\n");
        return 0;
    }
    return 1;
}

/*
 * Get the memory for chunk and load it
 */
static int load_hunk(void *file, struct sheader *sh)
{
    void *ptr=(void*)0;

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

/* Perform relocations of given section */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx, unsigned long long virt)
{
    struct sheader *shrel    = &sh[shrel_idx];
    struct sheader *shsymtab = &sh[shrel->link];
    struct sheader *toreloc  = &sh[shrel->info];

    struct symbol *symtab   = (struct symbol *)((unsigned long)shsymtab->addr);
    struct relo   *rel      = (struct relo *)((unsigned long)shrel->addr);
    char          *section  = (char *)((unsigned long)toreloc->addr);

    unsigned int numrel = (unsigned long)shrel->size / (unsigned long)shrel->entsize;
    unsigned int i;

    struct symbol *SysBase_sym = NULL;

    D(kprintf("[ELF Loader] performing %d relocations, target address %p%p\n", 
              numrel, (unsigned long)(virt >> 32), (unsigned long)virt));

    for (i=0; i<numrel; i++, rel++)
    {
        struct symbol *sym = &symtab[ELF_R_SYM(rel->info)];
        char *name = (char *)(unsigned long)sh[shsymtab->link].addr + sym->name;
        unsigned long *p = (unsigned long *)&section[rel->offset];
        unsigned long long s;

        switch (sym->shindex)
        {
            case SHN_UNDEF:
                D(kprintf("[ELF Loader] Undefined symbol '%s' while relocating the section '%s'\n",
			  name, (unsigned long)sh[eh->shstrndx].addr + toreloc->name));
                return 0;

            case SHN_COMMON:
                D(kprintf("[ELF Loader] COMMON symbol '%s' while relocating the section '%s'\n",
                      	  name, (unsigned long)sh[eh->shstrndx].addr + toreloc->name));

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
                *p = s + rel->addend - ((unsigned long)p + virt);
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

void load_elf_file(void *file, unsigned long long virt)
{
    struct elfheader eh;
    struct sheader *sh;
    long i;
    int addr_displayed = 0;
    
    D(kprintf("[ELF Loader] Loading ELF module from address %p\n", file));
    
    /* Check the header of ELF file */
    if
    (
        !read_block(file, 0, &eh, sizeof(eh)) ||
        !check_header(&eh) ||
        !(sh = load_block(file, eh.shoff, eh.shnum * eh.shentsize))
    )
    {
        kprintf("[ELF Loader] Wrong module header, aborting.\n");
        return;
    }
    
    /* Iterate over the section header in order to prepare memory and eventually load some hunks */
    for (i=0; i < eh.shnum; i++)
    {
        /* Load the symbol and string tables */
        if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB)
        {
            sh[i].addr = (unsigned long)load_block(file, sh[i].offset, sh[i].size);
        }
        /* Does the section require memoy allcation? */
        else if (sh[i].flags & SHF_ALLOC)
        {
            /* Yup, it does. Load the hunk */
            if (!load_hunk(file, &sh[i]))
            {
                kprintf("[ELF Loader] Error at loading of the hunk!\n");
            }
#ifndef DEBUG
            else if (!addr_displayed)
            {
                kprintf("0x%016X", sh[i].addr);
                addr_displayed = 1;
            }
#endif
        }	
    }

    /* For every loaded section perform the relocations */
    for (i=0; i < eh.shnum; i++)
    {
        if (sh[i].type == AROS_ELF_REL && sh[sh[i].info].addr)
        {
            sh[i].addr = (unsigned long)load_block(file, sh[i].offset, sh[i].size);
            if (!sh[i].addr || !relocate(&eh, sh, i, virt))
            {
                kprintf("[ELF Loader] Relocation error!\n");
            }
        }
    }
}
