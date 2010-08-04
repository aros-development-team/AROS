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

    if (eh->type != ET_REL || eh->machine != EM_386)
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

    D(kprintf("[ELF Loader] Chunk (%d bytes, align=%d) @ ", sh->size, sh->addralign));
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

	default:
	    printf("[ELF Loader] Unrecognized relocation type %d %ld\n", i, ELF_R_TYPE(rel->info));
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

int GetKernelSize(size_t *KernelSize, size_t *DebugSize)
{
    struct ELFNode *n;
    FILE *file;
    char *err;
    size_t ksize = 0;
    size_t dbgsize = sizeof(struct MinList);
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

	dbgsize += (sizeof(dbg_mod_t) + strlen(n->NamePtr));

	/* Go through all sections and calculate kernel size */
	for(i = 0; i < n->eh.shnum; i++)
	{
	    /* We include also string tables for debug info */
	    if ((n->sh[i].flags & SHF_ALLOC) || (n->sh[i].type == SHT_STRTAB))
		/* Add maximum space for alignment */
                ksize += (n->sh[i].size + n->sh[i].addralign - 1);

	    if (n->sh[i].type == SHT_SYMTAB)
		dbgsize += (n->sh[i].size / sizeof(struct symbol) * sizeof(dbg_sym_t));
	}
    }

    *KernelSize = ksize;
    *DebugSize  = dbgsize;

    return 1;
}

int LoadKernel(void *ptr_ro, void *ptr_dbg, struct KernelBSS *tracker)
{
    struct ELFNode *n;
    FILE *file;
    unsigned short i;
    dbg_mod_t *mod;

    D(printf("[ELF Loader] Loading kernel...\n"));

    for (n = FirstELF; n; n = n->Next)
    {
    	void *start = NULL;
	void *end   = NULL;

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

		/* Remember start and end addresses of the module */
		if (sh[i].flags & SHF_ALLOC) {
		    /* Start address is taken only once */
		    if (!start)
			start = sh[i].addr;

		    end = sh[i].addr + sh[i].size - 1;
		}
	    }
	}

	/* For every loaded section perform relocations */
	D(printf("[ELF Loader] Performing relocations...\n"));
	for (i=0; i < n->eh.shnum; i++) {
	    struct sheader *sh = n->sh;

	    if ((sh[i].type == SHT_RELA || sh[i].type == SHT_REL) && sh[sh[i].info].addr) {
		sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
		if (!sh[i].addr || !relocate(&n->eh, sh, i, 0)) {
		    printf("%s: Relocation error in hunk %u!\n", n->Name, 0);
		    return 0;
		}

		free(sh[i].addr);
	    }
	}

	D(printf("[ELF Loader] Adding debug information...\n"));
	mod = (dbg_mod_t *)ptr_dbg;

	NEWLIST(&mod->m_symbols);
	mod->m_lowest  = start;
	mod->m_highest = end;
	strcpy(mod->m_name, n->NamePtr);

	ptr_dbg += (sizeof(dbg_mod_t) + strlen(n->NamePtr));

	/* Free symbol tables not needed any more */
	for (i = 0; i < n->eh.shnum; i++) {
	    struct sheader *sh = n->sh;

	    if (sh[i].type == SHT_SYMTAB)
	    {
		int j;
		struct symbol *st = (struct symbol *)n->sh[i].addr;

		for (j=0; j < (sh[i].size / sizeof(struct symbol)); j++)
		{
		    unsigned long idx;

		    if (st[j].shindex != SHN_XINDEX) {
			idx = st[j].shindex;

			if (sh[idx].flags & SHF_ALLOC) {
			    dbg_sym_t *sym = (dbg_sym_t *)ptr_dbg;

			    sym->s_name    = n->sh[n->sh[i].link].addr + st[j].name;
			    sym->s_lowest  = n->sh[idx].addr + st[j].value;
			    sym->s_highest = sym->s_lowest + st[j].size - 1;
			    DSYM(printf("[ELF Loader] Listed symbol %s (0x%p - 0x%p)\n", sym->s_name, sym->s_lowest, sym->s_highest));

			    ADDTAIL(&mod->m_symbols, sym);
			    ptr_dbg += sizeof(dbg_sym_t);
			}
		    }
		}

		free(sh[i].addr);
	    }
	}

	free(n->sh);
    }
    return 1;
}
