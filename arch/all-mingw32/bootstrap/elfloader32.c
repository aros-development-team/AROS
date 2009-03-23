/*
 Copyright (C) 2006-2009 The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: ELF32 loader extracted from our internal_load_seg_elf in dos.library.
 Lang: English
 */

#include <windows.h>
#include "elfloader32.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define D(X) X
#define DREL(x)

#define kprintf printf

/*
 * These two pointers are used by the ELF loader to claim for memory ranges for both
 * the RW sections (.data, .bss and such) and RO sections (.text, .rodata....) of executable.
 */
char *kbase;
char *ptr_ro;
void ** SysBaseAddr;

struct _bss_tracker {
  void *addr;
  size_t len;
} *bss_tracker;

void *kernel_lowest()
{
    return kbase;
}

void *kernel_highest()
{
    return ptr_ro;
}

void set_base_address(void *tracker, void ** sysbaseaddr)
{
  D(printf("[ELF Loader] set_base_address %p %p\n", tracker, sysbaseaddr));
  bss_tracker = (struct _bss_tracker *)tracker;
  SysBaseAddr = sysbaseaddr;
}

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
 * load_block also allocates teh memory
 */
static void *load_block(void *file, long offset, long length)
{
  void * dest = malloc(length);
  fseek(file, offset, SEEK_SET);
  fread(dest, (size_t)length, 1, file);
  return dest;
}

static void free_block(void * block)
{
  free(block);
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
	D(kprintf("[ELF Loader] Not an ELF object\n"));
	return 0;
  }
  
  if (eh->type != ET_REL || eh->machine != EM_386)
  {
	D(kprintf("[ELF Loader] Wrong object type or wrong architecture\n"));
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

    D(kprintf("[ELF Loader] Chunk (%d bytes, align=%d) @ ", (unsigned int)sh->size, (unsigned int)sh->addralign));
    ptr_ro = (char *)(((unsigned long)ptr_ro + (unsigned long)sh->addralign - 1) & ~((unsigned long)sh->addralign-1));
    ptr = ptr_ro;
    ptr_ro = ptr_ro + sh->size;
    D(kprintf("%p\n", (unsigned int)ptr));
  
    sh->addr = ptr;
  
    /* copy block of memory from ELF file if it exists */
    if (sh->type != SHT_NOBITS)
	return read_block(file, sh->offset, (void *)((unsigned long)sh->addr), sh->size);
    else
    {
	memset(ptr, 0, sh->size);
	bss_tracker->addr = ptr;
	bss_tracker->len = sh->size;
	bss_tracker++;
    }
    return 1;
}

/* Perform relocations of given section */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx, ULONG_PTR virt)
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
  
  DREL(kprintf("[ELF Loader] performing %d relocations, virtual address %p\n", numrel, virt));
  
  for (i=0; i<numrel; i++, rel++)
  {
	struct symbol *sym = &symtab[ELF32_R_SYM(rel->info)];
	unsigned long *p = (unsigned long *)&section[rel->offset];
	ULONG_PTR s;
	const char * name = (char *)((unsigned long)sh[shsymtab->link].addr) + sym->name;
	switch (sym->shindex)
	{
	case SHN_UNDEF:
	    DREL(kprintf("[ELF Loader] Undefined symbol '%s' while relocating the section '%s'\n",
				  (char *)((unsigned long)sh[shsymtab->link].addr) + sym->name,
				  (char *)((unsigned long)sh[eh->shstrndx].addr) + toreloc->name));
	    return 0;
		
	case SHN_COMMON:
	    DREL(kprintf("[ELF Loader] COMMON symbol '%s' while relocating the section '%s'\n",
				  (char *)((unsigned long)sh[shsymtab->link].addr) + sym->name,
				  (char *)((unsigned long)sh[eh->shstrndx].addr) + toreloc->name));
		
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
SysBase_yes:    s = (ULONG_PTR)SysBaseAddr;
	    } else
SysBase_no:     s = sym->value;
	    break;
		
	default:
	    s = (unsigned long)sh[sym->shindex].addr + sym->value;
	}

	s += virt;

        DREL(printf("[ELF Loader] Relocating symbol "));
        DREL(if (sym->name) printf("%s", name); else printf("<unknown>"));
        DREL(printf("type "));
	switch (ELF32_R_TYPE(rel->info))
	{
	case R_386_32: /* 32bit absolute */
            DREL(printf("R_386_32"));
	    *p += s;
        if (*p < kbase) {
            exit(0);
        }

	    break;
		
	case R_386_PC32: /* 32bit PC relative */
            DREL(printf("R_386_PC32"));
	    *p += (s - (ULONG_PTR)p);
	    break;
		
	case R_386_NONE:
            DREL(printf("R_386_NONE"));
	    break;
		
	default:
	    printf("[ELF Loader] Unrecognized relocation type %d %d\n", i, (unsigned int)ELF32_R_TYPE(rel->info));
	    return 0;
	}
	DREL(printf(" -> 0x%p\n", *p));
  }
  return 1;
}

int load_elf_file(void *file, ULONG_PTR virt)
{
    struct elfheader eh;
    struct sheader *sh;
    unsigned long i;
    size_t ksize = 0;

    D(kprintf("[ELF Loader] Loading ELF module from virtual address %p\n", virt));
  
    /* Check the header of ELF file */
    if (!read_block(file, 0, &eh, sizeof(eh)) ||
	!check_header(&eh) ||
	!(sh = load_block(file, eh.shoff, eh.shnum * eh.shentsize))) {
	kprintf("[ELF Loader] Wrong module header, aborting.\n");
	return;
    }
  
    for(i = 0; i < eh.shnum; i++) {
        if (sh[i].flags & SHF_ALLOC)
            ksize += (sh[i].size + sh[i].addralign - 1);
    }
    
    kbase = malloc(ksize);
    ptr_ro = kbase;
    if (!kbase) {
        printf("[ELF Loader] Failed to allocate %lu bytes for kernel\n", ksize);
        return 0;
    }
    D(printf("[ELF Loader] Kernel memory allocated: %p-%p (%lu bytes)\n", kbase, kbase + ksize, ksize));
  
    /* Iterate over the section header in order to prepare memory and eventually load some hunks */
    for (i=0; i < eh.shnum; i++)
    {
        /* Load the symbol and string tables */
	if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB)
	{
            D(printf("[ELF Loader] Symbol table\n"));
	    sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
	}
	/* Does the section require memoy allcation? */
	else if (sh[i].flags & SHF_ALLOC)
	{
            D(printf("[ELF Loader] Allocated section\n"));
	    /* Yup, it does. Load the hunk */
	    if (!load_hunk(file, &sh[i]))
	    {
		kprintf("[ELF Loader] Error at loading of the hunk!\n");
	    }
	        D(else printf("[ELF Loader] shared mem@0x%x\n", sh[i].addr));
	}
    }
  
    /* For every loaded section perform the relocations */
    for (i=0; i < eh.shnum; i++)
    {
	if ((sh[i].type == SHT_RELA || sh[i].type == SHT_REL) && sh[sh[i].info].addr)
	{
	  sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
	  if (!sh[i].addr || !relocate(&eh, sh, i, virt))
	  {
		kprintf("[ELF Loader] Relocation error!\n");
	  }
	  free_block(sh[i].addr);
	}
	else if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB)
	  free_block(sh[i].addr);
    }
    free_block(sh);
    return 1;
}

