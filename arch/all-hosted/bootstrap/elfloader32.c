/*
 Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: ELF loader extracted from our internal_load_seg_elf in dos.library.
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
#include <libraries/debug.h>

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
    int err;

    err = fseek(file, offset, SEEK_SET);
    if (err) return 0;

    err = fread(dest,(size_t)length, 1, file);
    if (err == 0) return 0;


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

static void *copy_data(void *src, void *addr, unsigned long len)
{
    memcpy(addr, src, len);
    return addr + len;
}

/* Perform relocations of given section */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx)
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
  
  DREL(fprintf(stderr, "[ELF Loader] performing %d relocations\n", numrel));
  
  for (i=0; i<numrel; i++, rel++)
  {
	struct symbol *sym = &symtab[ELF_R_SYM(rel->info)];
	unsigned long *p = (unsigned long *)&section[rel->offset];
	uintptr_t s;
	const char *name = sh[shsymtab->link].addr + sym->name;

#ifdef __arm__
	/*
	 * R_ARM_V4BX are actually special marks for the linker.
	 * They even never have a target (shindex == SHN_UNDEF),
	 * so we simply ignore them before doing any checks.
	 */
	if (ELF_R_TYPE(rel->info) == R_ARM_V4BX)
	    continue;
#endif

	switch (sym->shindex)
	{
	case SHN_UNDEF:
	    DREL(fprintf(stderr, "[ELF Loader] Undefined symbol '%s'\n", name));
	    return 0;

	case SHN_COMMON:
	    DREL(fprintf(stderr, "[ELF Loader] COMMON symbol '%s'\n", name));

	    return 0;

	case SHN_ABS:
	    if (SysBase_sym == NULL)
	    {
	        if (strncmp(name, "SysBase", 8) == 0)
	        {
		    DREL(fprintf(stderr, "[ELF Loader] got SysBase\n"));
		    SysBase_sym = sym;
		    goto SysBase_yes;
		}
		else
		    goto SysBase_no;
	    }
	    else if (SysBase_sym == sym)
	    {
SysBase_yes:    s = (uintptr_t)&SysBase;
	    }
	    else
	    {
SysBase_no:     s = sym->value;
	    }
	    break;
		
	default:
	    s = (uintptr_t)sh[sym->shindex].addr + sym->value;
	}

        DREL(fprintf(stderr, "[ELF Loader] Relocating symbol %s type ", sym->name ? name : "<unknown>"));
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
	    fprintf(stderr, "[ELF Loader] Unrecognized relocation type %d %ld\n", i, (long)ELF_R_TYPE(rel->info));
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

	/*
	 * Debug data for the module includes:
	 * - Module descriptor (struct ELF_ModuleInfo)
	 * - ELF file header
	 * - ELF section header
	 * - File name
	 * - One empty pointer for alignment
	 */
	ksize += (sizeof(struct ELF_ModuleInfo) + sizeof(struct elfheader) + n->eh.shnum * n->eh.shentsize  +
		  strlen(n->NamePtr) + sizeof(void *));

	/* Go through all sections and calculate kernel size */
	for(i = 0; i < n->eh.shnum; i++)
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
		size_t s = n->sh[i].size + n->sh[i].addralign - 1;

		if (n->sh[i].flags & SHF_WRITE)
		    rwsize += s;
		else
		    ksize += s;
	    }
	}
    }

    *ro_size = ksize;
    *rw_size = rwsize;

    return 1;
}

int LoadKernel(void *ptr_ro, void *ptr_rw, struct KernelBSS *tracker, kernel_entry_fun_t *kernel_entry, struct ELF_ModuleInfo **kernel_debug)
{
    struct ELFNode *n;
    FILE *file;
    unsigned int i;
    unsigned char need_entry = 1;
    struct ELF_ModuleInfo *mod;
    /* Address of the first module descriptor will automatically go where we need it */
    struct ELF_ModuleInfo *prev_mod = (struct ELF_ModuleInfo *)kernel_debug;

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

	    if ((sh[i].flags & SHF_ALLOC) || (sh[i].type == SHT_STRTAB) || (sh[i].type == SHT_SYMTAB))
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

		/* Remember address of the first code section, this is our entry point */
		if ((sh[i].flags & SHF_EXECINSTR) && need_entry)
		{
		    *kernel_entry = sh[i].addr;
		    need_entry = 0;
		}
	    }
		D(else fprintf(stderr, "Ignored\n");)
	    D(fprintf(stderr, "[ELF Loader] Section address: %p, size: %lu\n", sh[i].addr, sh[i].size));
	}

	/* For every loaded section perform relocations */
	D(fprintf(stderr, "[ELF Loader] Relocating...\n"));
	for (i=0; i < n->eh.shnum; i++)
	{
	    struct sheader *sh = n->sh;

	    if ((sh[i].type == AROS_ELF_REL) && sh[sh[i].info].addr)
	    {
		sh[i].addr = load_block(file, sh[i].offset, sh[i].size);

		if (!sh[i].addr || !relocate(&n->eh, sh, i))
		{
		    DisplayError("%s: Relocation error in hunk %u!\n", n->Name, i);
		    return 0;
		}

		free(sh[i].addr);
		sh[i].addr = NULL;
	    }
	}

	D(fprintf(stderr, "[ELF Loader] Adding module debug information...\n"));

	/* Align our pointer */
	ptr_ro = (void *)(((unsigned long)ptr_ro + sizeof(void *)) & ~(sizeof(void *) - 1));

	/* Allocate module descriptor */
	mod = ptr_ro;
	ptr_ro += sizeof(struct ELF_ModuleInfo);
	mod->Next = NULL;
	mod->Type = DEBUG_ELF;

	/* Copy ELF header */
	mod->eh  = ptr_ro;
	ptr_ro = copy_data(&n->eh, ptr_ro, sizeof(struct elfheader));

	/* Copy section header */
	mod->sh = ptr_ro;
	ptr_ro = copy_data(n->sh, ptr_ro, n->eh.shnum * n->eh.shentsize);

	/* Copy module name */
	mod->Name = ptr_ro;
	ptr_ro = copy_data(n->NamePtr, ptr_ro, strlen(n->NamePtr) + 1);

	/* Link the module descriptor with previous one */
	prev_mod->Next = mod;
	prev_mod = mod;

	free(n->sh);
    }

    return 1;
}
