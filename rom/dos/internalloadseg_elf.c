/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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
#include <aros/machine.h>
#include "dos_intern.h"
#include "internalloadseg.h"
#include <aros/debug.h>
#include <string.h>
#include <stddef.h>

#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_NOBITS      8
#define SHT_REL         9

#define ET_REL          1

#define EM_386          3
#define EM_68K          4
#define EM_ARM         40

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2

#define R_68k_NONE      0
#define R_68K_32        1
#define R_68K_PC32      4

#define R_ARM_NONE      0
#define R_ARM_PC24      1
#define R_ARM_ABS32     2

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

struct symbol
{
    ULONG name;     /* Offset of the name string in the string table */
    ULONG value;    /* Varies; eg. the offset of the symbol in its hunk */
    ULONG size;     /* How much memory does the symbol occupy */
    UBYTE info;     /* What kind of symbol is this ? (global, variable, etc) */
    UBYTE other;    /* undefined */
    UWORD shindex;  /* In which section is the symbol defined ? */
};

struct relo
{
    ULONG offset;   /* Address of the relocation relative to the section it refers to */
    ULONG info;     /* Type of the relocation */
#ifdef __mc68000__
    LONG  addend;   /* Constant addend used to compute value */
#endif
};

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
                SetIoErr(ERROR_BAD_HUNK);

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
        kprintf("[ELF Loader] Not an elf object\n");
        SetIoErr(ERROR_NOT_EXECUTABLE);
        return 0;
    }

    if
    (
        eh->ident[EI_CLASS]   != ELFCLASS32  ||
        eh->ident[EI_VERSION] != EV_CURRENT  ||
        eh->type              != ET_REL      ||

        #if defined(__i386__)

            eh->ident[EI_DATA] != ELFDATA2LSB ||
            eh->machine        != EM_386

        #elif defined(__mc68000__)

            eh->ident[EI_DATA] != ELFDATA2MSB ||
            eh->machine        != EM_68K
        #elif defined(__arm__)
            eh->ident[EI_DATA] != ELFDATA2LSB ||
            eh->machine        != EM_ARM
#warning ARM has not been tested, yet!
        #else
        #    error Your architecture is not supported
        #endif
    )
    {
        kprintf("[ELF Loader] Object is of wrong type\n");
        kprintf("[ELF Loader] EI_CLASS   is %d - should be %d\n", eh->ident[EI_CLASS],   ELFCLASS32);
        kprintf("[ELF Loader] EI_VERSION is %d - should be %d\n", eh->ident[EI_VERSION], EV_CURRENT);
        kprintf("[ELF Loader] type       is %d - should be %d\n", eh->type,              ET_REL);

        kprintf("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA],
        #if defined (__i386__)
            ELFDATA2LSB);
        #elif defined(__mc68000__)
            ELFDATA2MSB);
        #elif defined(__arm__)
            ELFDATA2MSB);
        #endif

        kprintf("[ELF Loader] machine    is %d - should be %d\n", eh->machine,
        #if defined (__i386__)
            EM_386);
        #elif defined(__mc68000__)
            EM_68K);
        #elif defined(__arm__)
            EM_ARM);
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
    struct sheader     *sh,
    LONG               *funcarray,
    struct DosLibrary  *DOSBase
)
{
    struct hunk *hunk;

    if (!sh->size)
        return 1;

    hunk = MyAlloc(sh->size + sizeof(struct hunk), MEMF_ANY | (sh->type == SHT_NOBITS) ? MEMF_CLEAR : 0);
    if (hunk)
    {
        hunk->next = 0;
	hunk->size = sh->size + sizeof(struct hunk);

        sh->addr = hunk->data;

        /* Link the previous one with the new one */
        BPTR2HUNK(*next_hunk_ptr)->next = HUNK2BPTR(hunk);

        /* Update the pointer to the previous one, which is now the current one */
        *next_hunk_ptr = HUNK2BPTR(hunk);

        if (sh->type != SHT_NOBITS)
            return read_block(file, sh->offset, hunk->data, sh->size, funcarray, DOSBase);

        return 1;

    }

    SetIoErr(ERROR_NO_FREE_STORE);

    return 0;
}

static int relocate
(
    struct sheader    *sh,
    ULONG              shrel_idx,
    struct DosLibrary *DOSBase
)
{
    struct sheader *shrel    = &sh[shrel_idx];
    struct sheader *shsymtab = &sh[shrel->link];
    struct sheader *toreloc  = &sh[shrel->info];

    struct symbol *symtab   = (struct symbol *)shsymtab->addr;
    struct relo   *rel      = (struct relo *)shrel->addr;
    char          *section  = toreloc->addr;

    ULONG numrel = shrel->size / shrel->entsize;
    ULONG i;

    for (i=0; i<numrel; i++, rel++)
    {
        struct symbol *sym = &symtab[ELF32_R_SYM(rel->info)];
        ULONG *p = (ULONG *)&section[rel->offset];
	ULONG  s;

        switch (sym->shindex)
        {
            case SHN_UNDEF:
                kprintf("[ELF Loader] There are undefined symbols\n");
                return 0;

            case SHN_COMMON:
                kprintf("[ELF Loader] There are COMMON symbols. This should't happen\n");
                return 0;

            case SHN_ABS:
		s = sym->value;
                break;

  	    default:
		s = (ULONG)sh[sym->shindex].addr + sym->value;
 	}

        switch (ELF32_R_TYPE(rel->info))
        {
            #if defined(__i386__)

            case R_386_32: /* 32bit absolute */
                *p += s;
                break;

            case R_386_PC32: /* 32bit PC relative */
                *p += s - (ULONG)p;
                break;

            case R_386_NONE:
                break;

            #elif defined(__mc68000__)

            case R_68K_32:
                *p = s + rel->addend;
                break;

            case R_68K_PC32:
                *p = s + rel->addend - (ULONG)p;
                break;

            case R_68k_NONE:
                break;
            
            #elif defined(__arm__)
            
            /*
             * This has not been tested. Taken from ARMELF.pdf
             * from arm.com page 33ff.
             */
            case R_ARM_PC24:
                *p = s + rel->addend - (ULONG)p;
                break;
                
            case R_ARM_ABS32:
                *p = s + rel->addend;
                break;
            
            case R_ARM_NONE:
                break;
            
            #else
            #    error Your architecture is not supported
            #endif

            default:
                kprintf("[ELF Loader] Unrecognized relocation type %d\n", i, ELF32_R_TYPE(rel->info));
                SetIoErr(ERROR_BAD_HUNK);
		return 0;
        }
    }

    return 1;
}

BPTR InternalLoadSeg_ELF
(
    BPTR               file,
    BPTR               table __unused,
    LONG              *funcarray,
    LONG              *stack __unused,
    struct DosLibrary *DOSBase
)
{
    struct elfheader  eh;
    struct sheader   *sh;
    BPTR   hunks         = 0;
    BPTR  *next_hunk_ptr = &hunks;
    ULONG offset = 0, i;

    /* Load Elf Header and Section Headers */
    if
    (
        !read_block(file, 0, &eh, sizeof(eh), funcarray, DOSBase) ||
        !check_header(&eh, DOSBase) ||
        !(sh = load_block(file, eh.shoff, eh.shnum * eh.shentsize, funcarray, DOSBase))
    )
    {
        return 0;
    }

    /* Iterate over the section headers in order to do some stuff... */
    for (i = 0; i < eh.shnum; i++)
    {
        /*
           Load the symbol table(s).

           NOTICE: the ELF standard, at the moment (Nov 2002) explicitely states
                   that only one symbol table per file is allowed. However, it
                   also states that this may change in future... we already handle it.
        */
        if (sh[i].type == SHT_SYMTAB)
        {
            sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, DOSBase);
            if (!sh[i].addr)
                goto error;
        }
        else
        /* Load the section in memory if needed, and make an hunk out of it */
        if (sh[i].flags & SHF_ALLOC)
        {
            if (!load_hunk(file, &next_hunk_ptr, &sh[i], funcarray, DOSBase))
                goto error;
        }

    }

    /* Relocate the sections */
    for (i = 0; i < eh.shnum; i++)
    {
        if
        (
            #if defined(__i386__)

            sh[i].type == SHT_REL &&

            #elif defined(__mc68000__)

            sh[i].type == SHT_RELA &&

            #elif defined(__arm__)
            #warning Missing code for ARM            
//            sh[i].type = SHT_

            #else
            #    error Your architecture is not supported
            #endif

            /* Does this relocation section refer to a hunk? If so, addr must be != 0 */
            sh[sh[i].info].addr
        )
        {
	    sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, DOSBase);
            if (!sh[i].addr || !relocate(sh, i, DOSBase))
                goto error;

            MyFree(sh[i].addr, sh[i].size);
            sh[i].addr = NULL;
        }
    }

    /* No errors, deallocate only the symbol tables */
    for (i = 0; i < eh.shnum; i++)
    {
        if (sh[i].type == SHT_SYMTAB)
            MyFree(sh[i].addr, sh[i].size);
    }

    goto end;

error:

    /* There were some errors, deallocate all the allocated sections */
    for (i = 0; i < eh.shnum; i++)
    {
        if (sh[i].addr)
        {
            if (sh[i].flags & SHF_ALLOC)
                MyFree(sh[i].addr - sizeof(struct hunk), sh[i].size + sizeof(struct hunk));
            else
                MyFree(sh[i].addr, sh[i].size);
        }
    }

    hunks = 0;

end:

    /* Free the section headers */
    MyFree(sh, eh.shnum * eh.shentsize);

    return hunks;
}

#undef MyRead
#undef MyAlloc
#undef MyFree
