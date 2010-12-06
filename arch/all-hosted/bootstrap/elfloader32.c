/*
 Copyright (C) 2006-2010 The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: ELF32 loader extracted from our internal_load_seg_elf in dos.library.
 Lang: English
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* These macros are defined in both UNIX and AROS headers. Get rid of warnings. */
#undef __pure
#undef __const
#undef __pure2
#undef __deprecated

#include <aros/config.h>
#include <aros/kernel.h>
#include <exec/execbase.h>
#include <dos/bptr.h>
#include <dos/elf.h>

#include "elfloader32.h"
#include "support.h"
#include "ui.h"

#define D(x)
#define DREL(x)
#define DSYM(x)

struct ELFNode
{
    struct ELFNode   *Next;
    struct sheader   *sh;
    struct elfheader  eh;
    char	     *NamePtr;
    char	      Name[1];
};

struct ELFNode *FirstELF = NULL;
struct ELFNode *LastELF = (struct ELFNode *)&FirstELF;

/* ***** This is the global SysBase ***** */
struct ExecBase *SysBase;

/*
 * read_block interface. we want to read from files here
 */
static int read_block(void *file, long offset, void *dest, long length)
{
    fseek(file, offset, SEEK_SET);
    fread(dest,(size_t)length, 1, file);

    return 1;
}

/*
 * load_block also allocates the memory
 */
static void *load_block(void *file, long offset, long length)
{
    void *dest = malloc(length);
    
    if (dest)
    {
	if (!read_block(file, offset, dest, length)) {
	    free(dest);
	    return NULL;
	}
    }

    return dest;
}

/*
 * Test for correct ELF header here
 */
static char *check_header(struct elfheader *eh)
{
    if (eh->ident[0] != 0x7f || eh->ident[1] != 'E'  ||
        eh->ident[2] != 'L'  || eh->ident[3] != 'F')
	return "Not a ELF file";

    if (eh->type != ET_REL || eh->machine != AROS_ELF_MACHINE)
	return "Wrong object type or wrong architecture";

    /* No error */
    return NULL;
}

/*
 * Get the memory for chunk and load it
 */
void *load_hunk(void *file, struct sheader *sh, void *addr, struct KernelBSS **bss_tracker)
{ 
    /* empty chunk? Who cares :) */
    if (!sh->size)
	return addr;

    D(fprintf(stderr, "[ELF Loader] Chunk (%ld bytes, align=%ld (%p) @ ", sh->size, sh->addralign, (void *)sh->addralign));
    addr = (char *)(((IPTR)addr + sh->addralign - 1) & ~(sh->addralign-1));

    D(fprintf(stderr, "%p\n", addr));
    sh->addr = addr;

    /* copy block of memory from ELF file if it exists */
    if (sh->type != SHT_NOBITS)
    {
	if (!read_block(file, sh->offset, sh->addr, sh->size))
	    return NULL;
    }
    else
    {
	memset(addr, 0, sh->size);

	(*bss_tracker)->addr = addr;
	(*bss_tracker)->len = sh->size;
	(*bss_tracker)++;
    }

    return addr + sh->size;
}

/* Perform relocations of given section */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx, uintptr_t virt)
{
  struct sheader *shrel    = &sh[shrel_idx];
  struct sheader *shsymtab = &sh[shrel->link];
  struct sheader *toreloc  = &sh[shrel->info];
  
  struct symbol *symtab   = (struct symbol *)shsymtab->addr;
  struct relo   *rel      = (struct relo *)shrel->addr;
  char          *section  = (char *)toreloc->addr;
  
  unsigned int numrel = shrel->size / shrel->entsize;
  unsigned int i;
  
  struct symbol *SysBase_sym = NULL;
  
  DREL(fprintf(stderr, "[ELF Loader] performing %d relocations, virtual address %p\n", numrel, virt));
  
  for (i=0; i<numrel; i++, rel++)
  {
	struct symbol *sym = &symtab[ELF_R_SYM(rel->info)];
	unsigned long *p = (unsigned long *)&section[rel->offset];
	uintptr_t s;
	const char *name = sh[shsymtab->link].addr + sym->name;

	switch (sym->shindex)
	{
	case SHN_UNDEF:
	    DREL(fprintf(stderr, "[ELF Loader] Undefined symbol '%s'\n", name));
	    return 0;

	case SHN_COMMON:
	    DREL(fprintf(stderr, "[ELF Loader] COMMON symbol '%s'\n", name));

	    return 0;

	case SHN_ABS:
	    if (SysBase_sym == NULL) {
	        if (strncmp(name, "SysBase", 8) == 0) {
		    DREL(fprintf(stderr, "[ELF Loader] got SysBase\n"));
			SysBase_sym = sym;
			goto SysBase_yes;
		} else
		    goto SysBase_no;
	    } else if (SysBase_sym == sym) {
SysBase_yes:    s = (uintptr_t)&SysBase;
	    } else
SysBase_no:     s = sym->value;
	    break;
		
	default:
	    s = (uintptr_t)sh[sym->shindex].addr + sym->value;
	}

	s += virt;

        DREL(fprintf(stderr, "[ELF Loader] Relocating symbol "));
        DREL(if (sym->name) fprintf(stderr, "%s", name); else fprintf(stderr, "<unknown>"));
        DREL(fprintf(stderr, " type "));
	switch (ELF_R_TYPE(rel->info))
	{
#ifdef __i386__
	case R_386_32: /* 32bit absolute */
            DREL(fprintf(stderr, "R_386_32"));
	    *p += s;
	    break;

	case R_386_PC32: /* 32bit PC relative */
            DREL(fprintf(stderr, "R_386_PC32"));
	    *p += (s - (uintptr_t)p);
	    break;

	case R_386_NONE:
            DREL(fprintf(stderr, "R_386_NONE"));
	    break;
#endif
#ifdef __x86_64__
        case R_X86_64_64: /* 64bit direct/absolute */
            *(uint64_t *)p = s + rel->addend;
            break;

        case R_X86_64_PC32: /* PC relative 32 bit signed */
            *(uint32_t *)p = s + rel->addend - (uintptr_t) p;
            break;

        case R_X86_64_32:
            *(uint32_t *)p = (uint64_t)s + (uint64_t)rel->addend;
            break;
                
        case R_X86_64_32S:
            *(int32_t *)p = (int64_t)s + (int64_t)rel->addend;
            break;

        case R_X86_64_NONE: /* No reloc */
            break;
#endif
#ifdef __mc68000__
        case R_68K_32:
            *p = s + rel->addend;
            break;

        case R_68K_PC32:
            *p = s + rel->addend - (uint32_t)p;
            break;

        case R_68k_NONE:
            break;
#endif
#if defined(__ppc__) || defined(__powerpc__)
        case R_PPC_ADDR32:
            *p = s + rel->addend;
            break;
	
	case R_PPC_ADDR16_LO:
	{
	    unsigned short *c = (unsigned short *) p;
	    *c = (s + rel->addend) & 0xffff;
	}
	break;

	case R_PPC_ADDR16_HA:
	{
	    unsigned short *c = (unsigned short *) p;
	    uint32_t temp = s + rel->addend;
	    *c = temp >> 16;
	    if ((temp & 0x8000) != 0)
		(*c)++;
	}
	break;

        case R_PPC_REL16_LO:
	{
	    unsigned short *c = (unsigned short *) p;
	    *c = (s + rel->addend - (uint32_t)p) & 0xffff;
	}
	break;

        case R_PPC_REL16_HA:
	{
	    unsigned short *c = (unsigned short *) p;
	    uint32_t temp = s + rel->addend - (uint32_t)p;
	    *c = temp >> 16;
	    if ((temp & 0x8000) != 0)
			(*c)++;
	}
	break;

        case R_PPC_REL24:
	    *p &= ~0x3fffffc;
            *p |= (s + rel->addend - (uint32_t)p) & 0x3fffffc;
            break;

	case R_PPC_REL32:
	    *p = s + rel->addend - (uint32_t)p;
	    break;

        case R_PPC_NONE:
            break;
#endif
#ifdef __arm__
	case R_ARM_CALL:
	case R_ARM_JUMP24:
        case R_ARM_PC24:
	    {
		/* On ARM the 24 bit offset is shifted by 2 to the right */
		signed long offset = (*p & 0x00ffffff) << 2;
		/* If highest bit set, make offset negative */
		if (offset & 0x02000000)
		    offset -= 0x04000000;
		
		offset += s - (uint32_t)p;
		
		offset >>= 2;
		*p &= 0xff000000;
		*p |= offset & 0x00ffffff;
            }
            break;

	case R_ARM_MOVW_ABS_NC:
	case R_ARM_MOVT_ABS:
	    {
		signed long offset = *p;
		offset = ((offset & 0xf0000) >> 4) | (offset & 0xfff);
		offset = (offset ^ 0x8000) - 0x8000;
		
		offset += s;
		
		if (ELF_R_TYPE(rel->info) == R_ARM_MOVT_ABS)
		    offset >>= 16;
		    
		*p &= 0xfff0f000;
		*p |= ((offset & 0xf000) << 4) | (offset & 0x0fff);
	    }
	    break;
	    
        case R_ARM_ABS32:
            *p += s;
            break;

        case R_ARM_NONE:
            break;
#endif
	default:
	    fprintf(stderr, "[ELF Loader] Unrecognized relocation type %d %ld\n", i, ELF_R_TYPE(rel->info));
	    return 0;
	}
	DREL(fprintf(stderr, " -> %p\n", *p));
  }
  return 1;
}

int AddKernelFile(char *name)
{
    struct ELFNode *n;

    n = malloc(sizeof(struct ELFNode) + strlen(name));
    if (!n)
        return 0;
    
    n->Next  = NULL;
    strcpy(n->Name, name);
#if AROS_MODULES_DEBUG
    n->NamePtr = n->Name;
#else
    n->NamePtr = namepart(n->Name);
#endif

    LastELF->Next = n;
    LastELF = n;

    return 1;
}

void FreeKernelList(void)
{
    struct ELFNode *n, *n2;
    
    for (n = FirstELF; n; n = n2) {
	n2 = n->Next;
	free(n);
    }
    /* We do not reset list pointers because the list will never be reused */
}

int GetKernelSize(size_t *ro_size, size_t *rw_size)
{
    struct ELFNode *n;
    FILE *file;
    char *err;
    size_t ksize = 0;
    size_t rwsize = 0;
    unsigned short i;

    D(fprintf(stderr, "[ELF Loader] Calculating kernel size...\n"));

    for (n = FirstELF; n; n = n->Next)
    {
	D(fprintf(stderr, "[ELF Loader] Checking file %s\n", n->Name));
	file = fopen(n->Name, "rb");
	if (!file) {
	    DisplayError("Failed to open file %s!\n", n->Name);
	    return 0;
	}

	/* Check the header of ELF file */
	read_block(file, 0, &n->eh, sizeof(struct elfheader));
	err = check_header(&n->eh);
	if (err)
	    n->sh = NULL;
	else {
	    n->sh = load_block(file, n->eh.shoff, n->eh.shnum * n->eh.shentsize);
	    if (!n->sh)
	        err = "Failed to read file";
	}

	fclose(file);
	if (err)
	{
	    DisplayError("%s: %s\n", n->Name, err);
	    return 0;
	}

	/* Module descriptor for the debug info */
	ksize += (sizeof(dbg_mod_t) + strlen(n->NamePtr));

	/* Go through all sections and calculate kernel size */
	for(i = 0; i < n->eh.shnum; i++)
	{
	    /* Ignore sections with zero lengths */
	    if (!n->sh[i].size)
	    	continue;
	
	    /* We include also string tables for debug info */
	    if ((n->sh[i].flags & SHF_ALLOC) || (n->sh[i].type == SHT_STRTAB))
	    {
		/* Add maximum space for alignment */
		size_t s = n->sh[i].size + n->sh[i].addralign - 1;

		if (n->sh[i].flags & SHF_WRITE)
		    rwsize += s;
		else
		    ksize += s;
	    }

	    /* Every loadable section gets segment descriptor in the debug info */
	    if (n->sh[i].flags & SHF_ALLOC)
		ksize += sizeof(dbg_seg_t);

	    /* Debug info also includes symbols array */
	    if (n->sh[i].type == SHT_SYMTAB)
		ksize += (n->sh[i].size / sizeof(struct symbol) * sizeof(dbg_sym_t));
	}
    }

    *ro_size = ksize;
    *rw_size = rwsize;

    return 1;
}

int LoadKernel(volatile void *ptr_ro, volatile void *ptr_rw, struct KernelBSS *tracker, kernel_entry_fun_t *kernel_entry, dbg_seg_t **kernel_debug)
{
    struct ELFNode *n;
    FILE *file;
    unsigned int i;
    volatile dbg_mod_t *mod;
    volatile dbg_seg_t *seg = NULL;

    D(fprintf(stderr, "[ELF Loader] Loading kernel...\n"));

    for (n = FirstELF; n; n = n->Next)
    {
	D(fprintf(stderr, "[ELF Loader] Loading file %s\n", n->Name));

	file = fopen(n->Name, "rb");
	if (!file)
	{
	    DisplayError("Failed to open file %s!\n", n->Name);
	    return 0;
	}

	/* Iterate over the section header in order to load some hunks */
	for (i=0; i < n->eh.shnum; i++)
	{
	    struct sheader *sh = n->sh;

	    D(fprintf(stderr, "[ELF Loader] Section %u... ", i));
            /* Load symbol tables */
	    if (sh[i].type == SHT_SYMTAB)
	    {
		D(fprintf(stderr, "Symbol table\n"));
		sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
	    }
	    else if ((sh[i].flags & SHF_ALLOC) || (sh[i].type == SHT_STRTAB))
	    {
		/* Does the section require memory allcation? */
		D(fprintf(stderr, "Allocated section\n"));

		if (sh[i].flags & SHF_WRITE)
		{
		    ptr_rw = load_hunk(file, &sh[i], (void *)ptr_rw, &tracker);
		    if (!ptr_rw)
		    {
			DisplayError("%s: Error loading hunk %u!\n", n->Name, i);
			return 0;
		    }
		}
		else
		{
		    ptr_ro = load_hunk(file, &sh[i], (void *)ptr_ro, &tracker);
		    if (!ptr_ro)
		    {
			DisplayError("%s: Error loading hunk %u!\n", n->Name, i);
			return 0;
		    }
		}
	    }
		D(else fprintf(stderr, "Ignored\n");)
	    D(fprintf(stderr, "[ELF Loader] Section address: %p, size: %lu\n", sh[i].addr, sh[i].size));
	}

	D(fprintf(stderr, "[ELF Loader] Adding module debug information...\n"));
	mod = (dbg_mod_t *)ptr_ro;
	ptr_ro += (sizeof(dbg_mod_t) + strlen(n->NamePtr));

	strcpy((char *)mod->m_name, n->NamePtr);

	/* For every loaded section perform relocations and add debug info */
	D(fprintf(stderr, "[ELF Loader] Relocating and adding section debug information...\n"));
	for (i=0; i < n->eh.shnum; i++)
	{
	    struct sheader *sh = n->sh;

	    if ((sh[i].type == AROS_ELF_REL) && sh[sh[i].info].addr)
	    {
		sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
		if (!sh[i].addr || !relocate(&n->eh, sh, i, 0))
		{
		    DisplayError("%s: Relocation error in hunk %u!\n", n->Name, i);
		    return 0;
		}

		free(sh[i].addr);
	    }

	    if ((sh[i].flags & SHF_ALLOC) && sh[i].size)
	    {
		/* Link new segment descriptor with the previous one */
		if (seg)
		    seg->s_next = (dbg_seg_t *)ptr_ro;
		else {
		    /* Remember start of code and debug info for the first segment */
		    *kernel_entry = sh[i].addr;
		    *kernel_debug = (dbg_seg_t *)ptr_ro;
		}

		seg = (dbg_seg_t *)ptr_ro;
		ptr_ro += sizeof(dbg_seg_t);

		seg->s_next    = NULL;
		seg->s_lowest  = sh[i].addr;
		seg->s_highest = sh[i].addr + sh[i].size - 1;
		seg->s_module  = (dbg_mod_t *)mod;
		seg->s_name    = sh[n->eh.shstrndx].addr + sh[i].name;
		seg->s_num     = i;
		
		D(fprintf(stderr, "[ELF Loader] Listed section %u (%s, %p - %p)\n", seg->s_num, seg->s_name, seg->s_lowest, seg->s_highest));
	    }
	}

	/* Copy symbols to the debug info and free symbol tables */
	for (i = 0; i < n->eh.shnum; i++)
	{
	    struct sheader *sh = n->sh;

	    if (sh[i].type == SHT_SYMTAB)
	    {
		struct symbol *st = (struct symbol *)sh[i].addr;
		unsigned int syms = sh[i].size / sizeof(struct symbol);
		unsigned int j;

		DSYM(fprintf(stderr, "[ELF Loader] Listing symbols (total of %u) from section %u at %p\n", syms, i, st));

		mod->m_symbols = (dbg_sym_t *)ptr_ro;
		mod->m_symcnt  = syms;

		for (j=0; j < syms; j++)
		{
		    unsigned long idx = st[j].shindex;

		    DSYM(fprintf(stderr, "[ELF Loader] Symbol %u index: %lu\n", j, idx));
		    if (idx == SHN_XINDEX)
			continue;

		    if ((idx == SHN_ABS) || (sh[idx].flags & SHF_ALLOC))
		    {
			dbg_sym_t *sym = (dbg_sym_t *)ptr_ro;

			ptr_ro += sizeof(dbg_sym_t);

			sym->s_name    = n->sh[n->sh[i].link].addr + st[j].name;

			if (idx == SHN_ABS)
			    sym->s_lowest = (void *)st[j].value;
			else
			    sym->s_lowest  = n->sh[idx].addr + st[j].value;

			if (st[j].size)
			    sym->s_highest = sym->s_lowest + st[j].size - 1;
			else
			    sym->s_highest = NULL;

			DSYM(fprintf(stderr, "[ELF Loader] Listed symbol %s (%p - %p)\n", sym->s_name, sym->s_lowest, sym->s_highest));
		    }
		}

		free(sh[i].addr);
	    }
	}

	free(n->sh);
    }
    return 1;
}
