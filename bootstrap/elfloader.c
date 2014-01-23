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

/* Define this wrapper here, before loading AROS headers,
 * so that the defines in <aros/system.h> do not
 * confuse GCC's built-in substitutions for strcmp().
 */
static inline int Strcmp(const char *a, const char *b) { return strcmp(a, b); }

#include <dos/elf.h>
#include <libraries/debug.h>

#include <elfloader.h>
#include <runtime.h>

#define D(x)
#define DREL(x)
#define DSYM(x)

/* Use own definitions because we may be compiled as 32-bit code but build structures for 64-bit code */
struct ELF_ModuleInfo_t
{
    elf_uintptr_t  Next;
    elf_uintptr_t  Name;
    unsigned short Type;
    unsigned short Pad0;        /* On i386 we have different alignment, so do explicit padding */
#ifdef ELF_64BIT
    unsigned int   Pad1;
#endif
    elf_uintptr_t  eh;
    elf_uintptr_t  sh;
};

/* Our own definition of struct KernelBSS, to avoid excessive castings */
struct KernelBSS_t
{
    elf_uintptr_t addr;
    elf_uintptr_t len;
};

static elf_uintptr_t SysBase_ptr = 0;

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
static void *load_hunk(void *file, struct sheader *sh, void *addr, struct KernelBSS_t **bss_tracker)
{ 
    uintptr_t align;

    /* empty chunk? Who cares :) */
    if (!sh->size)
        return addr;

    D(kprintf("[ELF Loader] Chunk (%ld bytes, align=%ld (%p) @ ", sh->size, sh->addralign, (void *)sh->addralign));
    align = sh->addralign - 1;
    addr = (char *)(((uintptr_t)addr + align) & ~align);

    D(kprintf("%p\n", addr));
    sh->addr = (elf_ptr_t)(uintptr_t)addr;

    /* copy block of memory from ELF file if it exists */
    if (sh->type != SHT_NOBITS)
    {
        if (read_block(file, sh->offset, (void *)(uintptr_t)sh->addr, sh->size))
            return NULL;
    }
    else
    {
        memset(addr, 0, sh->size);

        (*bss_tracker)->addr = (uintptr_t)addr;
        (*bss_tracker)->len = sh->size;
        (*bss_tracker)++;
    }

    return addr + sh->size;
}

static void *copy_data(void *src, void *addr, uintptr_t len)
{
    memcpy(addr, src, len);
    return addr + len;
}

/* Perform relocations of given section */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx, elf_uintptr_t DefSysBase)
{
    struct sheader *shrel    = &sh[shrel_idx];
    struct sheader *shsymtab = &sh[shrel->link];
    struct sheader *toreloc  = &sh[shrel->info];

    struct symbol *symtab   = (struct symbol *)(uintptr_t)shsymtab->addr;
    struct relo   *rel      = (struct relo *)(uintptr_t)shrel->addr;
    /* Early cast to uintptr_t omits __udivdi3 call in x86-64 native bootstrap */
    unsigned int numrel = (uintptr_t)shrel->size / (uintptr_t)shrel->entsize;
    unsigned int i;

    struct symbol *SysBase_sym = NULL;

    /*
     * Ignore relocs if the target section has no allocation. that can happen
     * eg. with a .debug PROGBITS and a .rel.debug section
     */
    if (!(toreloc->flags & SHF_ALLOC))
        return 1;

    DREL(kprintf("[ELF Loader] performing %d relocations\n", numrel));
  
    for (i=0; i<numrel; i++, rel++)
    {
        struct symbol *sym = &symtab[ELF_R_SYM(rel->info)];
        uintptr_t *p = (void *)(uintptr_t)toreloc->addr + rel->offset;
        const char *name = (const char *)(uintptr_t)sh[shsymtab->link].addr + sym->name;
        elf_uintptr_t s;

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
            if (Strcmp(name, "SysBase") == 0) {
                if (!SysBase_ptr)
                {
                    SysBase_ptr = DefSysBase;
                    D(kprintf("[ELF Loader] SysBase pointer set to default %p\n", (void *)SysBase_ptr));
                }

                s = SysBase_ptr;
            } else {
                kprintf("[ELF Loader] Undefined symbol '%s'\n", name);
                return 0;
            }
            break;

        case SHN_COMMON:
            kprintf("[ELF Loader] COMMON symbol '%s'\n", name);
            return 0;

        case SHN_ABS:
            if (SysBase_sym == NULL)
            {
                if (Strcmp(name, "SysBase") == 0)
                {
                    DREL(kprintf("[ELF Loader] got SysBase\n"));
                    SysBase_sym = sym;
                }
            }

            if (SysBase_sym == sym)
            {
                if (!SysBase_ptr)
                {
                    SysBase_ptr = DefSysBase;
                    D(kprintf("[ELF Loader] SysBase pointer set to default %p\n", (void *)SysBase_ptr));
                }

                s = SysBase_ptr;
            }
            else
                s = sym->value;
            break;
                
        default:
            s = (uintptr_t)sh[sym->shindex].addr + sym->value;

            if (!SysBase_ptr)
            {
                /*
                 * The first global data symbol named SysBase becomes global SysBase.
                 * The idea behind: the first module (kernel.resource) contains global
                 * SysBase variable and all other modules are linked to it.
                 */
                if (sym->info == ELF_S_INFO(STB_GLOBAL, STT_OBJECT))
                {
                    if (Strcmp(name, "SysBase") == 0)
                    {
                        SysBase_ptr = s;
                        D(kprintf("[ELF Loader] SysBase pointer set to %p\n", (void *)SysBase_ptr));
                    }
                }
            }
        }

        DREL(kprintf("[ELF Loader] Relocating symbol %s type ", sym->name ? name : "<unknown>"));
        switch (ELF_R_TYPE(rel->info))
        {
#ifdef ELF_64BIT
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
#else
#ifdef __i386__
        case R_386_32: /* 32bit absolute */
            DREL(kprintf("R_386_32"));
            *p += s;
            break;

        case R_386_PC32: /* 32bit PC relative */
            DREL(kprintf("R_386_PC32"));
            *p += (s - (uintptr_t)p);
            break;

        case R_386_NONE:
            DREL(kprintf("R_386_NONE"));
            break;
#endif
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
        case R_ARM_PREL31:
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
            kprintf("[ELF Loader] Unrecognized relocation type %d %ld\n", i, (long)ELF_R_TYPE(rel->info));
            return 0;
        }
        DREL(kprintf(" -> %p\n", *p));
    }
    return 1;
}

int GetKernelSize(struct ELFNode *FirstELF, unsigned long *ro_size, unsigned long *rw_size, unsigned long *bss_size)
{
    struct ELFNode *n;
    unsigned long ksize = 0;
    unsigned long rwsize = 0;
    unsigned long bsize = sizeof(struct KernelBSS_t);
    unsigned short i;

    kprintf("[ELF Loader] Calculating kickstart size...\n");

    for (n = FirstELF; n; n = n->Next)
    {
        void *file;
        char *errstr = NULL;
        unsigned int err;

        D(kprintf("[ELF Loader] Checking file %s\n", n->Name));
        
        file = open_file(n, &err);
        if (err)
        {
            DisplayError("Failed to open file %s!\n", n->Name);
            return 0;
        }

        /* Check the header of ELF file */
        n->eh = load_block(file, 0, sizeof(struct elfheader), &err);
        if (err)
        {
            errstr = "Failed to read file header";
        }
        else
        {
            errstr = check_header(n->eh);
            if (!errstr)
            {
                n->sh = load_block(file, n->eh->shoff, n->eh->shnum * n->eh->shentsize, &err);
                if (err)
                {
                    errstr = "Failed to read section headers";
                }
            }
        }

        close_file(file);
        if (errstr)
        {
            DisplayError("%s: %s\n", n->Name, errstr);
            return 0;
        }

        /*
         * Debug data for the module includes:
         * - Module descriptor (struct ELF_ModuleInfo_t)
         * - ELF file header
         * - ELF section header
         * - File name
         * - One empty pointer for alignment
         */
        ksize += (sizeof(struct ELF_ModuleInfo_t) + sizeof(struct elfheader) + n->eh->shnum * n->eh->shentsize  +
                  strlen(n->Name) + sizeof(void *));

        /* Go through all sections and calculate kernel size */
        for(i = 0; i < n->eh->shnum; i++)
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
                
                if (n->sh[i].type == SHT_NOBITS)
                    bsize += sizeof(struct KernelBSS_t);
            }
        }
    }

    *ro_size = ksize;
    *rw_size = rwsize;

    if (bss_size)
        *bss_size = bsize;

    kprintf("[ELF Loader] Code %lu bytes, data %lu bytes, BSS array %lu bytes\n", ksize, rwsize, bsize);

    return 1;
}

/*
 * This function loads the listed modules.
 * It expects that ELF and section header pointers in the list are already set up by GetKernelSize().
 *
 * (elf_ptr_t)(uintptr_t) double-casting is needed because in some cases elf_ptr_t is an UQUAD,
 * while in most cases it's a pointer (see dos/elf.h).
 */
int LoadKernel(struct ELFNode *FirstELF, void *ptr_ro, void *ptr_rw, char *tracker_p, uintptr_t DefSysBase,
               void **kick_end, kernel_entry_fun_t *kernel_entry, struct ELF_ModuleInfo **kernel_debug)
{
    struct ELFNode *n;
    unsigned int i;
    unsigned char need_entry = 1;
    struct ELF_ModuleInfo_t *mod;
    struct ELF_ModuleInfo_t *prev_mod = NULL;
    struct KernelBSS_t *tracker = (struct KernelBSS_t *)tracker_p;

    kprintf("[ELF Loader] Loading kickstart...\n");

    for (n = FirstELF; n; n = n->Next)
    {
        void *file;
        unsigned int err;

        kprintf("[ELF Loader] Code %p, Data %p, Module %s...\n", ptr_ro, ptr_rw, n->Name);

        file = open_file(n, &err);
        if (err)
        {
            DisplayError("Failed to open file %s!\n", n->Name);
            return 0;
        }

        /* Iterate over the section header in order to load some hunks */
        for (i=0; i < n->eh->shnum; i++)
        {
            struct sheader *sh = n->sh;

            D(kprintf("[ELF Loader] Section %u... ", i));

            if ((sh[i].flags & SHF_ALLOC) || (sh[i].type == SHT_STRTAB) || (sh[i].type == SHT_SYMTAB))
            {
                /* Does the section require memory allcation? */
                D(kprintf("Allocated section\n"));

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
                    *kernel_entry = (void *)(uintptr_t)sh[i].addr;
                    need_entry = 0;
                }
            }
                D(else kprintf("Ignored\n");)

            D(kprintf("[ELF Loader] Section address: %p, size: %lu\n", sh[i].addr, sh[i].size));
        }

        /* For every loaded section perform relocations */
        D(kprintf("[ELF Loader] Relocating...\n"));
        for (i=0; i < n->eh->shnum; i++)
        {
            struct sheader *sh = n->sh;

            if ((sh[i].type == AROS_ELF_REL) && sh[sh[i].info].addr)
            {
                sh[i].addr = (elf_ptr_t)(uintptr_t)load_block(file, sh[i].offset, sh[i].size, &err);
                if (err)
                {
                    DisplayError("%s: Failed to load relocation section %u\n", n->Name, i);
                    return 0;
                }

                if (!relocate(n->eh, sh, i, (uintptr_t)DefSysBase))
                {
                    DisplayError("%s: Relocation error in section %u!\n", n->Name, i);
                    return 0;
                }

                free_block((void *)(uintptr_t)sh[i].addr);
                sh[i].addr = (elf_ptr_t)0;
            }
        }

        close_file(file);

        D(kprintf("[ELF Loader] Adding module debug information...\n"));

        /* Align our pointer */
        ptr_ro = (void *)(((uintptr_t)ptr_ro + sizeof(void *)) & ~(sizeof(void *) - 1));

        /* Allocate module descriptor */
        mod = ptr_ro;
        ptr_ro += sizeof(struct ELF_ModuleInfo_t);
        mod->Next = 0;
        mod->Type = DEBUG_ELF;

        /* Copy ELF header */
        mod->eh  = (uintptr_t)ptr_ro;
        ptr_ro = copy_data(n->eh, ptr_ro, sizeof(struct elfheader));

        /* Copy section header */
        mod->sh = (uintptr_t)ptr_ro;
        ptr_ro = copy_data(n->sh, ptr_ro, n->eh->shnum * n->eh->shentsize);

        /* Copy module name */
        mod->Name = (uintptr_t)ptr_ro;
        ptr_ro = copy_data(n->Name, ptr_ro, strlen(n->Name) + 1);

        /* Link the module descriptor with previous one */
        if (prev_mod)
            prev_mod->Next = (uintptr_t)mod;
        else
            *kernel_debug = (struct ELF_ModuleInfo *)mod;
        prev_mod = mod;

        free_block(n->sh);
        free_block(n->eh);
    }

    /* Terminate the array of BSS sections */
    tracker->addr = 0;
    tracker->len  = 0;

    /* Return end of kickstart read-only area if requested */
    if (kick_end)
        *kick_end = ptr_ro;

    return 1;
}
