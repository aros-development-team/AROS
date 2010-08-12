/*
 Copyright (C) 2006-2010 The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: ELF32 loader extracted from our internal_load_seg_elf in dos.library.
 Lang: English
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <aros/kernel.h>
#include <dos/elf.h>

#include "elfloader32.h"
#include "support.h"

#define D(x)
#define DREL(x)
#define DSYM(x)

#define kprintf printf

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
void *SysBase;

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

    D(kprintf("[ELF Loader] Chunk (%ld bytes, align=%ld (0x%p) @ ", sh->size, sh->addralign, (void *)sh->addralign));
    addr = (char *)(((IPTR)addr + sh->addralign - 1) & ~(sh->addralign-1));

    D(kprintf("%p\n", addr));
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
  
  DREL(kprintf("[ELF Loader] performing %d relocations, virtual address %p\n", numrel, virt));
  
  for (i=0; i<numrel; i++, rel++)
  {
	struct symbol *sym = &symtab[ELF_R_SYM(rel->info)];
	unsigned long *p = (unsigned long *)&section[rel->offset];
	uintptr_t s;
	const char *name = sh[shsymtab->link].addr + sym->name;

	switch (sym->shindex)
	{
	case SHN_UNDEF:
	    DREL(kprintf("[ELF Loader] Undefined symbol '%s'\n", name));
	    return 0;

	case SHN_COMMON:
	    DREL(kprintf("[ELF Loader] COMMON symbol '%s'\n", name));

	    return 0;

	case SHN_ABS:
	    if (SysBase_sym == NULL) {
	        if (strncmp(name, "SysBase", 8) == 0) {
		    DREL(kprintf("[ELF Loader] got SysBase\n"));
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

        DREL(printf("[ELF Loader] Relocating symbol "));
        DREL(if (sym->name) printf("%s", name); else printf("<unknown>"));
        DREL(printf(" type "));
	switch (ELF_R_TYPE(rel->info))
	{
#ifdef __i386__
	case R_386_32: /* 32bit absolute */
            DREL(printf("R_386_32"));
	    *p += s;
	    break;

	case R_386_PC32: /* 32bit PC relative */
            DREL(printf("R_386_PC32"));
	    *p += (s - (uintptr_t)p);
	    break;

	case R_386_NONE:
            DREL(printf("R_386_NONE"));
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
        case R_ARM_PC24:
            *p = s + rel->addend - (uint32_t)p;
            break;

        case R_ARM_ABS32:
            *p = s + rel->addend;
            break;

        case R_ARM_NONE:
            break;
#endif
	default:
	    printf("[ELF Loader] Unrecognized relocation type %d %d\n", i, ELF_R_TYPE(rel->info));
	    return 0;
	}
	DREL(printf(" -> 0x%p\n", *p));
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
    n->NamePtr = namepart(n->Name);

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

int GetKernelSize(size_t *KernelSize)
{
    struct ELFNode *n;
    FILE *file;
    char *err;
    size_t ksize = 0;
    unsigned short i;

    D(printf("[ELF Loader] Calculating kernel size...\n"));

    for (n = FirstELF; n; n = n->Next)
    {
	D(printf("[ELF Loader] Checking file %s\n", n->Name));
	file = fopen(n->Name, "rb");
	if (!file) {
	    printf("Failed to open file %s!\n", n->Name);
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
	if (err) {
	    printf("%s: %s\n", n->Name, err);
	    return 0;
	}

	/* Module descriptor for the debug info */
	ksize += (sizeof(dbg_mod_t) + strlen(n->NamePtr));

	/* Go through all sections and calculate kernel size */
	for(i = 0; i < n->eh.shnum; i++)
	{
	    /* We include also string tables for debug info */
	    if ((n->sh[i].flags & SHF_ALLOC) || (n->sh[i].type == SHT_STRTAB))
		/* Add maximum space for alignment */
                ksize += (n->sh[i].size + n->sh[i].addralign - 1);

	    /* Every loadable section gets segment descriptor in the debug info */
	    if (n->sh[i].flags & SHF_ALLOC)
		ksize += sizeof(dbg_seg_t);

	    /* Debug info also includes symbols array */
	    if (n->sh[i].type == SHT_SYMTAB)
		ksize += (n->sh[i].size / sizeof(struct symbol) * sizeof(dbg_sym_t));
	}
    }

    *KernelSize = ksize;

    return 1;
}

int LoadKernel(void *ptr_ro, struct KernelBSS *tracker, kernel_entry_fun_t *kernel_entry, void **kernel_debug)
{
    struct ELFNode *n;
    FILE *file;
    unsigned int i;
    dbg_mod_t *mod;
    dbg_seg_t *seg = NULL;

    D(printf("[ELF Loader] Loading kernel...\n"));

    for (n = FirstELF; n; n = n->Next)
    {
	D(printf("[ELF Loader] Loading file %s\n", n->Name));

	file = fopen(n->Name, "rb");
	if (!file) {
	    printf("Failed to open file %s!\n", n->Name);
	    return 0;
	}

	/* Iterate over the section header in order to load some hunks */
	for (i=0; i < n->eh.shnum; i++)
	{
	    struct sheader *sh = n->sh;

            /* Load symbol tables */
	    if (sh[i].type == SHT_SYMTAB)
	    {
		D(printf("[ELF Loader] Symbol table\n"));
		sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
	    }
	    else if ((sh[i].flags & SHF_ALLOC) || (sh[i].type == SHT_STRTAB))
	    {
		/* Does the section require memory allcation? */
		D(printf("[ELF Loader] Allocated section\n"));

		ptr_ro = load_hunk(file, &sh[i], ptr_ro, &tracker);

		if (!ptr_ro) {
		    printf("%s: Error loading hunk %u!\n", n->Name, i);
		    return 0;
		}
	        D(printf("[ELF Loader] Section address: 0x%p\n", sh[i].addr));
	    }
	}

	D(printf("[ELF Loader] Adding module debug information...\n"));
	mod = (dbg_mod_t *)ptr_ro;
	ptr_ro += (sizeof(dbg_mod_t) + strlen(n->NamePtr));

	strcpy(mod->m_name, n->NamePtr);

	/* For every loaded section perform relocations and add debug info */
	D(printf("[ELF Loader] Relocating and adding section debug information...\n"));
	for (i=0; i < n->eh.shnum; i++)
	{
	    struct sheader *sh = n->sh;

	    if ((sh[i].type == AROS_ELF_REL) && sh[sh[i].info].addr)
	    {
		sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
		if (!sh[i].addr || !relocate(&n->eh, sh, i, 0)) {
		    printf("%s: Relocation error in hunk %u!\n", n->Name, i);
		    return 0;
		}

		free(sh[i].addr);
	    }

	    if (sh[i].flags & SHF_ALLOC)
	    {
		/* Link new segment descriptor with the previous one */
		if (seg)
		    seg->s_next = ptr_ro;
		else {
		    /* Remember start of code and debug info for the first segment */
		    *kernel_entry = sh[i].addr;
		    *kernel_debug = seg;
		}

		seg = (dbg_seg_t *)ptr_ro;
		ptr_ro += sizeof(dbg_seg_t);

		seg->s_next    = NULL;
		seg->s_lowest  = sh[i].addr;
		seg->s_highest = sh[i].addr + sh[i].size - 1;
		seg->s_module  = mod;
		seg->s_name    = sh[n->eh.shstrndx].addr + sh[i].name;
		seg->s_num     = i;
		
		D(printf("[ELF Loader] Listed section %u (%s, 0x%p - 0x%p)\n", seg->s_num, seg->s_name, seg->s_lowest, seg->s_highest));
	    }
	}

	/* Copy symbols to the debug info and free symbol tables */
	for (i = 0; i < n->eh.shnum; i++) {
	    struct sheader *sh = n->sh;

	    if (sh[i].type == SHT_SYMTAB)
	    {
		struct symbol *st = (struct symbol *)n->sh[i].addr;
		unsigned int syms = sh[i].size / sizeof(struct symbol);
		unsigned int j;

		mod->m_symbols = ptr_ro;
		mod->m_symcnt  = syms;

		for (j=0; j < syms; j++)
		{
		    unsigned long idx;

		    if (st[j].shindex == SHN_XINDEX)
			continue;

		    idx = st[j].shindex;

		    if (sh[idx].flags & SHF_ALLOC)
		    {
			dbg_sym_t *sym = (dbg_sym_t *)ptr_ro;

			ptr_ro += sizeof(dbg_sym_t);

			sym->s_name    = n->sh[n->sh[i].link].addr + st[j].name;
			sym->s_lowest  = n->sh[idx].addr + st[j].value;
			sym->s_highest = sym->s_lowest + st[j].size - 1;

			DSYM(printf("[ELF Loader] Listed symbol %s (0x%p - 0x%p)\n", sym->s_name, sym->s_lowest, sym->s_highest));
		    }
		}

		free(sh[i].addr);
	    }
	}

	free(n->sh);
    }
    return 1;
}
