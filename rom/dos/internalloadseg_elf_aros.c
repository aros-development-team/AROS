/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code to dynamically load ELF executables
    Lang: english

    1997/12/13: Changed filename to internalloadseg_elf.c
                Original file was created by digulla.
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <aros/asmcall.h>
#include "dos_intern.h"
#include "internalloadseg.h"
#define DEBUG 0
#include <aros/debug.h>
#include <string.h>
#include <stddef.h>

#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_NOBITS      8
#define SHT_REL         9

#define SHT_LOOS        0x60000000
#define SHT_AROS_REL32  (SHT_LOOS)

#define ET_REL          1
#define ET_EXEC         2

#define EM_386          3
#define EM_68K          4
#define EM_PPC         20
#define EM_ARM         40
#define EM_X86_64       62

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2

#define R_X86_64_NONE   0 
#define R_X86_64_64     1
#define R_X86_64_PC32   2

#define R_68k_NONE      0
#define R_68K_32        1
#define R_68K_PC32      4

#define STT_OBJECT      1
#define STT_FUNC        2

#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_UNDEF       0

#define SHF_ALLOC            (1 << 1)

#define ELF32_ST_TYPE(i)    ((i) & 0x0F)

#define EI_VERSION      6
#define EV_CURRENT      1

#define EI_DATA         5
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

#define EI_CLASS        4
#define ELFCLASS32      1
#define ELFCLASS64      2

#define EI_OSABI        7
#define EI_ABIVERSION   8

#define ELFOSABI_AROS   15

#define PF_X            (1 << 0)


#define ELF32_R_SYM(val)        ((val) >> 8)
#define ELF32_R_TYPE(val)       ((val) & 0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))


struct elfheader
{
    UBYTE ident[16];
    UWORD type;
    UWORD machine;
    ULONG version;
    APTR  entry;
    ULONG phoff;
    ULONG shoff;
    ULONG flags;
    UWORD ehsize;
    UWORD phentsize;
    UWORD phnum;
    UWORD shentsize;
    UWORD shnum;
    UWORD shstrndx;
};

struct sheader
{
    ULONG name;
    ULONG type;
    ULONG flags;
    APTR  addr;
    ULONG offset;
    ULONG size;
    ULONG link;
    ULONG info;
    ULONG addralign;
    ULONG entsize;
};

struct pheader
{
    ULONG type;                 
    ULONG offset;               
    APTR  vaddr;                
    APTR  paddr;                
    ULONG filesz;               
    ULONG memsz;                
    ULONG flags;                
    ULONG align;                
};

#define PT_LOAD 1

struct hunk
{
    ULONG size;
    BPTR  next;
    char  data[0];
} __attribute__((packed));

#define BPTR2HUNK(bptr) ((struct hunk *)((char *)BADDR(bptr) - offsetof(struct hunk, next)))
#define HUNK2BPTR(hunk) MKBADDR(&hunk->next)

#undef MyRead
#undef MyAlloc
#undef MyFree


#define MyRead(file, buf, size)      \
    AROS_CALL3                       \
    (                                \
        LONG, funcarray[0],          \
        AROS_LCA(BPTR,   file, D1),  \
        AROS_LCA(void *, buf,  D2),  \
        AROS_LCA(LONG,   size, D3),  \
        struct DosLibrary *, DOSBase \
    )


#define MyAlloc(size, flags)        \
    AROS_CALL2                      \
    (                               \
        void *, funcarray[1],       \
        AROS_LCA(ULONG, size,  D0), \
        AROS_LCA(ULONG, flags, D1), \
        struct ExecBase *, SysBase  \
    )


#define MyFree(addr, size)          \
    AROS_CALL2                      \
    (                               \
        void, funcarray[2],         \
        AROS_LCA(void *, addr, A1), \
        AROS_LCA(ULONG,  size, D0), \
        struct ExecBase *, SysBase  \
    )

static int read_block
(
    BPTR               file,
    ULONG              offset,
    APTR               buffer,
    ULONG              size,
    LONG              *funcarray,
    struct DosLibrary *DOSBase
)
{
    UBYTE *buf = (UBYTE *)buffer;
    LONG   subsize;

    if (Seek(file, offset, OFFSET_BEGINNING) < 0)
        return 0;

    while (size)
    {
        subsize = MyRead(file, buf, size);

        if (subsize <= 0)
        {
            if (subsize == 0)
            {
                D(bug("[ELF Loader] Error while reading from file.\n"));
                D(bug("[ELF Loader] Offset = %ld - Size = %ld\n", offset, size));
                SetIoErr(ERROR_BAD_HUNK);
            }

            return 0;
        }

        buf  += subsize;
        size -= subsize;
    }

    return 1;
}

static void * load_block
(
    BPTR               file,
    ULONG              offset,
    ULONG              size,
    LONG              *funcarray,
    struct DosLibrary *DOSBase
)
{
    void *block = MyAlloc(size, MEMF_ANY);
    if (block)
    {
        if (read_block(file, offset, block, size, funcarray, DOSBase))
            return block;

        MyFree(block, size);
    }
    else
        SetIoErr(ERROR_NO_FREE_STORE);

    return NULL;
}

static int check_header(struct elfheader *eh, struct DosLibrary *DOSBase)
{
    if
    (
        eh->ident[0] != 0x7f ||
        eh->ident[1] != 'E'  ||
        eh->ident[2] != 'L'  ||
        eh->ident[3] != 'F'
    )
    {
        D(bug("[ELF Loader] Not an elf object\n"));
        SetIoErr(ERROR_NOT_EXECUTABLE);
        return 0;
    }

    if
    (
        eh->ident[EI_CLASS]      != (ELFCLASS32 || ELFCLASS64)    ||
        eh->ident[EI_VERSION]    != EV_CURRENT    ||
        eh->ident[EI_OSABI]      != ELFOSABI_AROS ||
        eh->ident[EI_ABIVERSION] != 0             ||
        eh->type                 != ET_EXEC       ||

        #if defined(__i386__)

            eh->ident[EI_DATA] != ELFDATA2LSB ||
            eh->machine        != EM_386

        #elif defined(__x86_64__)

            eh->ident[EI_DATA] != ELFDATA2LSB ||
            eh->machine        != EM_X86_64

        #elif defined(__mc68000__)

            eh->ident[EI_DATA] != ELFDATA2MSB ||
            eh->machine        != EM_68K

        #elif defined(__ppc__) || defined(__powerpc__)
            eh->ident[EI_DATA] != ELFDATA2MSB ||
            eh->machine        != EM_PPC

        #elif defined(__arm__)
            
            eh->ident[EI_DATA] != ELFDATA2LSB ||
            eh->machine        != EM_ARM

        #else
        #    error Your architecture is not supported
        #endif
    )
    {
        D(bug("[ELF Loader] Object is of wrong type\n"));
        D(bug("[ELF Loader] EI_CLASS   is %d - should be %d\n", eh->ident[EI_CLASS],   ELFCLASS32));
        D(bug("[ELF Loader] EI_VERSION is %d - should be %d\n", eh->ident[EI_VERSION], EV_CURRENT));
        D(bug("[ELF Loader] type       is %d - should be %d\n", eh->type,              ET_REL));

#if defined (__i386__)
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA], ELFDATA2LSB));
#elif defined(__mc68000__)
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA], ELFDATA2MSB));
#elif defined(__ppc__) || defined(__powerpc__)
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA],ELFDATA2MSB));
#elif defined(__arm__)
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA], ELFDATA2MSB));
#endif

#if defined (__i386__)
        D(bug("[ELF Loader] machine    is %d - should be %d\n", eh->machine, EM_386));
#elif defined(__mc68000__)
        D(bug("[ELF Loader] machine    is %d - should be %d\n", eh->machine, EM_68K));
#elif defined(__ppc__) || defined(__powerpc__)
        D(bug("[ELF Loader] machine    is %d - should be %d\n", eh->machine, EM_PPC));
#elif defined(__arm__)
        D(bug("[ELF Loader] machine    is %d - should be %d\n", eh->machine, EM_ARM));
#endif

        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        return 0;
    }

    return 1;
}

static int load_hunk
(
    BPTR                file,
    BPTR              **next_hunk_ptr,
    struct pheader     *ph,
    LONG               *funcarray,
    struct DosLibrary  *DOSBase
)
{
    struct hunk *hunk;

    /* Sanity check */
    if (ph->memsz < ph->filesz)
    {
      SetIoErr(ERROR_BAD_HUNK);
      return 0;
    }
    
    if (!ph->memsz)
        return 1;

    hunk = MyAlloc(ph->memsz + sizeof(struct hunk), MEMF_ANY);
    if (hunk)
    {
	hunk->size = ph->memsz + sizeof(struct hunk);

        ph->paddr = hunk->data;

        /* Link the new hunk with the old next one. This makes it possible
           handle insertion */
        hunk->next = BPTR2HUNK(*next_hunk_ptr)->next;
        
        /* Link the previous one with the new one */
        BPTR2HUNK(*next_hunk_ptr)->next = HUNK2BPTR(hunk);

        /* Update the pointer to the previous one, which is now the current one */
        *next_hunk_ptr = HUNK2BPTR(hunk);

        /* Clear out the memory that is not filled with file contents */
        memset(hunk->data + ph->filesz, 0, ph->memsz - ph->filesz);
        
        /* Finally read the segment from the file into memory */
        return read_block(file, ph->offset, hunk->data, ph->filesz, funcarray, DOSBase);
    }

    SetIoErr(ERROR_NO_FREE_STORE);

    return 0;
}

union aros_rel_entry
{
    ULONG segment;
    ULONG offset;
    ULONG num_entries;
};

static int relocate
(
    struct pheader       *ph,
    union aros_rel_entry *rel,
    ULONG                 toreloc_idx
)
{
    const char *contents = ph[toreloc_idx].paddr;

    int num_segments = (rel++)->num_entries;
    
    while (num_segments--)
    {
        const struct pheader *fromreloc   = &ph[(rel++)->segment];
        const ULONG           addr_to_add = fromreloc->paddr - fromreloc->vaddr;
        ULONG                 num_relocs  = (rel++)->num_entries;

        while (num_relocs--)
            *((ULONG *)&contents[rel++->offset]) += addr_to_add;  
    }
    
    return 1;
}

BPTR InternalLoadSeg_ELF_AROS
(
    BPTR               file,
    BPTR               table __unused,
    LONG              *funcarray,
    LONG              *stack __unused,
    struct DosLibrary *DOSBase
)
{
    struct elfheader  eh;
    struct sheader   *sh = NULL;
    struct pheader   *ph = NULL;
    
    BPTR   hunks         = 0;
    BPTR  *next_hunk_ptr = &hunks;
    ULONG  i;
    BOOL   exec_segment_found = FALSE;

    /* Load Elf Header and Section Headers */
    if
    (
        !read_block(file, 0, &eh, sizeof(eh), funcarray, DOSBase) ||
        !check_header(&eh, DOSBase) ||
	!(sh = load_block(file, eh.shoff, eh.shnum * eh.shentsize, funcarray, DOSBase)) ||
	!(ph = load_block(file, eh.phoff, eh.phnum * eh.phentsize, funcarray, DOSBase))
    )
    {
        goto end;
    }

    /* Iterate over the program headers in order to do some stuff... */
    for (i = 0; i < eh.phnum; i++)
    {
        /* Load the segment in memory if needed, and make an hunk out of it */
        if (ph[i].type == PT_LOAD)
        {
            if (!load_hunk(file, &next_hunk_ptr, &ph[i], funcarray, DOSBase))
                goto error;
                
            /* If this segment holds executable code, build a trampoline hunk
               which points to the entry location into the object */
            if (ph[i].flags & PF_X)
            {
	        BPTR  *next_hunk_ptr2 = &hunks;
                struct pheader ph_trampoline;
                
                if (!exec_segment_found)
                    exec_segment_found = TRUE;
                else
                {
                    /* We allow only one executable segment per object */
                    SetIoErr(ERROR_BAD_HUNK);
                    goto error;
                }
                
                if
                (            
                    !((eh.entry >= ph[i].vaddr) &&
                      (eh.entry <= (ph[i].vaddr + ph[i].memsz)))
                )
                {
                    /* The entry point must fall into the range of the executable
                       segment */
                    SetIoErr(ERROR_BAD_HUNK);
                    goto error;
                }
                
                /* Build a fake program header */
                ph_trampoline.filesz = 0;
                ph_trampoline.memsz  = sizeof (struct FullJumpVec);
                
                /* Now allocate the hunk relative to the fake ph */
                if (!load_hunk(file, &next_hunk_ptr2, &ph_trampoline, funcarray, DOSBase))
                    goto error;
            
                /* Finally, build the trampoline */
                __AROS_SET_FULLJMP
                (
                    ph_trampoline.paddr,
                    (ULONG)eh.entry + (ULONG)ph[i].paddr - (ULONG)ph[i].vaddr
                );        
            }
        }
    }

    /* Relocate the segments */
    for (i = 0; i < eh.shnum; i++)
    {
        if (sh[i].type == SHT_AROS_REL32)
        {
            sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, DOSBase);
            if (!sh[i].addr || !relocate(ph, sh[i].addr, sh[i].info))
                goto error;
            
            MyFree(sh[i].addr, sh[i].size);
            sh[i].addr = NULL;
        }
    }

    /* No errors */
    
    goto end;

error:

    /* There were some errors, deallocate The hunks */
    
    InternalUnLoadSeg(hunks, (VOID_FUNC)funcarray[2]);
    hunks = 0;

end:

    /* Free the section headers */
    if (sh)
      MyFree(sh, eh.shnum * eh.shentsize);
      
    /* Free the program header */
    if (ph)
      MyFree(ph, eh.phnum * eh.phentsize);
 
    return hunks;
}

#undef MyRead
#undef MyAlloc
#undef MyFree
