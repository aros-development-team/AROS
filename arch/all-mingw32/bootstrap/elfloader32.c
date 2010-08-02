/*
 Copyright (C) 2006-2010 The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: ELF32 loader extracted from our internal_load_seg_elf in dos.library.
 Lang: English
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <windows.h>

#define EXEC_TYPES_H

typedef BYTE     UBYTE;
typedef WORD     UWORD;
typedef UINT_PTR IPTR;
typedef INT_PTR  SIPTR;
typedef void *   APTR;

#include <dos/elf.h>

#include "elfloader32.h"

#define D(x)
#define DREL(x)

#define kprintf printf

/*
 * This pointer is used by the ELF loader to claim for memory ranges for executables.
 */
char *ptr_ro;
void ** SysBaseAddr;

struct _bss_tracker
{
  void *addr;
  size_t len;
} *bss_tracker;

struct ELFNode
{
    struct ELFNode   *Next;
    struct sheader   *sh;
    struct elfheader  eh;
    char	      Name[1];
};

struct ELFNode *FirstELF = NULL;
struct ELFNode *LastELF = (struct ELFNode *)&FirstELF;

void *kernel_highest()
{
    return ptr_ro - 1;
}

void set_base_address(void *kstart, void *tracker, void ** sysbaseaddr)
{
    D(printf("[ELF Loader] set_base_address %p %p\n", tracker, sysbaseaddr));

    ptr_ro = kstart;
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

/*
 * Test for correct ELF header here
 */
static char *check_header(struct elfheader *eh)
{
    if (eh->ident[0] != 0x7f || eh->ident[1] != 'E'  ||
        eh->ident[2] != 'L'  || eh->ident[3] != 'F')
	return "Not a ELF object";
  
    if (eh->type != ET_REL || eh->machine != EM_386)
	return "Wrong object type or wrong architecture";

    /* No error */
    return NULL;
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
	struct symbol *sym = &symtab[ELF_R_SYM(rel->info)];
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
        DREL(printf(" type "));
	switch (ELF_R_TYPE(rel->info))
	{
	case R_386_32: /* 32bit absolute */
            DREL(printf("R_386_32"));
	    *p += s;
	    break;
		
	case R_386_PC32: /* 32bit PC relative */
            DREL(printf("R_386_PC32"));
	    *p += (s - (ULONG_PTR)p);
	    break;
		
	case R_386_NONE:
            DREL(printf("R_386_NONE"));
	    break;
		
	default:
	    printf("[ELF Loader] Unrecognized relocation type %d %d\n", i, (unsigned int)ELF_R_TYPE(rel->info));
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
    
    n->Next = NULL;
    strcpy(n->Name, name);

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

size_t GetKernelSize(void)
{
    struct ELFNode *n;
    FILE *file;
    char *err;
    size_t ksize = 0;
    unsigned short i;

    D(printf("[ELF Loader] Calculating kernel size...\n"));

    for (n = FirstELF; n; n = n->Next) {
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

	for(i = 0; i < n->eh.shnum; i++) {
	    if (n->sh[i].flags & SHF_ALLOC)
                ksize += (n->sh[i].size + n->sh[i].addralign - 1);
	}
    }

    return ksize;
}

int LoadKernel(void)
{
    struct ELFNode *n;
    FILE *file;
    unsigned short i;

    D(printf("[ELF Loader] Loading kernel...\n"));

    for (n = FirstELF; n; n = n->Next) {
	D(printf("[ELF Loader] Loading file %s\n", n->Name));
	file = fopen(n->Name, "rb");
	if (!file) {
	    printf("Failed to open file %s!\n", n->Name);
	    return 0;
	}
	
	/* Iterate over the section header in order to prepare memory and eventually load some hunks */
	for (i=0; i < n->eh.shnum; i++) {
	    struct sheader *sh = n->sh;

            /* Load the symbol and string tables */
	    if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB) {
		D(printf("[ELF Loader] Symbol table\n"));
		sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
	    } else if (sh[i].flags & SHF_ALLOC) {
		/* Does the section require memory allcation? */
		D(printf("[ELF Loader] Allocated section\n"));
		if (!load_hunk(file, &sh[i])) {
		    printf("%s: Error loading hunk %u!\n", n->Name, i);
		    return 0;
		}
	        D(printf("[ELF Loader] shared mem@0x%x\n", sh[i].addr));
	    }
	}
  
	/* For every loaded section perform relocations */
	for (i=0; i < n->eh.shnum; i++) {
	    struct sheader *sh = n->sh;

	    if ((sh[i].type == SHT_RELA || sh[i].type == SHT_REL) && sh[sh[i].info].addr) {
		sh[i].addr = load_block(file, sh[i].offset, sh[i].size);
		if (!sh[i].addr || !relocate(&n->eh, sh, i, 0)) {
		    printf("%s: Relocation error in hunk %u!\n", n->Name, 0);
		    return 0;
		}

		free(sh[i].addr);
	    } else if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB)
		free(sh[i].addr);
	}

	free(n->sh);
    }
    return 1;
}
