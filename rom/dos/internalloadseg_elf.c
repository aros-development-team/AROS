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
#include <aros/machine.h>
#include "dos_intern.h"
#include "internalloadseg.h"
#include <aros/debug.h>
#include <string.h>
#include <stddef.h>

extern struct DosLibrary * DOSBase;


/* Debugging */
#define PRINT_SECTION_NAMES     0
#define PRINT_STRINGTAB         0
#define PRINT_SYMBOLTABLE       0
#define PRINT_SYMBOLS           0
#define LOAD_DEBUG_HUNKS        0
#define PRINT_SECTIONS          0
#define PRINT_HUNKS             0
#define DEBUG_HUNKS             0

#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_NOBITS      8
#define SHT_REL         9

#define ET_REL          1

#define EM_386          3
#define EM_68K          4

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2

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
    UWORD  shindex;  /* In which section is the symbol defined ? */
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

/*

We will follow this pattern:

Load Elf Header
{
    Load Sections Headers
    {
        Load Symbol Table // Assuming that there's always a symbol table
        {
            For Each PROGBITS and NOBITS section (sec)
            {
                If sec has to be allocated
                {
                    if sec is PROGBITS Load Section sec
                    else Allocate enough space and put it in sec

	            Add sec to a list of hunks.
                }
            }

            For Each REL and RELA section (sec)
            {
                Load sec
                Relocate sec.sec_to_relocate
		Unload sec
  	    }

            Free Symbol Table

            return the list of hunks
	}
    }
}

*/

static __inline__ int check_header(struct elfheader *eh, struct DosLibrary *DOSBase)
{
    if
    (
        eh->ident[0] != 0x7f ||
        eh->ident[1] != 'E'  ||
        eh->ident[2] != 'L'  ||
        eh->ident[3] != 'F'
    )
    {
        SetIoErr(ERROR_NOT_EXECUTABLE);
        return 0;
    }

    if
    (
        eh->ident[EI_CLASS]   != ELFCLASS32  ||
        eh->ident[EI_VERSION] != EV_CURRENT  ||
        eh->type              != ET_REL      ||

        #if defined(__mc68000__)
            eh->ident[EI_DATA] != ELFDATA2MSB ||
            eh->machine        != EM_68K
        #elif defined(__i386__)
            eh->ident[EI_DATA] != ELFDATA2LSB ||
            eh->machine        != EM_386
        #endif
    )
    {
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

    return 0;
}

static int relocate
(
    struct sheader *sh,
    int             shrel_idx,
    struct DosLibrary  *DOSBase
)
{
    struct sheader *shrel    = &sh[shrel_idx];
    struct sheader *shsymtab = &sh[shrel->link];
    struct sheader *toreloc  = &sh[shrel->info];

    struct symbol *symtab   = (struct symbol *)shsymtab->addr;
    struct relo   *rel      = (struct relo *)shrel->addr;
    char          *section  = toreloc->addr;

    ULONG numrel = shrel->size / shrel->entsize;
    int i;

    //kprintf("Section %d has %d relocation entries\n", shrel_idx, numrel);
    for (i=0; i<numrel; i++, rel++)
    {
        struct symbol *sym = &symtab[ELF32_R_SYM(rel->info)];
        ULONG *p = (ULONG *)&section[rel->offset];
	ULONG  s;

        #if 0
        kprintf("Processing relocation %d\n", i);
        kprintf("rel->offset = 0x%08x - %d\n", rel->offset, rel->offset);
	kprintf("rel->info   = 0x%08x - Type = %d - Sym = %d\n", rel->info, ELF32_R_TYPE(rel->info), ELF32_R_SYM(rel->info));
        kprintf("sym->value  = 0x%08x - %d\n", sym->value, sym->value);
        kprintf("sym->index  = %d\n", sym->shindex);
        kprintf("dest section base   = %p\n", toreloc->addr);
        kprintf("source section base = %p\n", sh[sym->shindex].addr);
        #endif
        
        switch (sym->shindex)
        {
            case SHN_UNDEF:
                kprintf("There are undefined symbols\n");
                return 0;

            case SHN_COMMON:
                kprintf("Found a COMMON symbol.\n");
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
                
            #else
            #    error Your architecture is not supported
            #endif

            default:
                kprintf("\007 Unrecognized relocation type %d\n", i, ELF32_R_TYPE(rel->info));
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
    struct elfheader eh;
    BPTR hunks = 0;

    /* Load Elf Header */
    if
    (
        read_block(file, 0, &eh, sizeof(eh), funcarray, DOSBase) &&
        check_header(&eh, DOSBase)
    )
    {
        /* Load Section Headers. Also allocate space for a probable common section */
        struct sheader *sh = load_block(file, eh.shoff, (eh.shnum+1) * eh.shentsize, funcarray, DOSBase);
        if (sh)
        {
            int i;

            /* Load Symbol Table(s) */
            for (i = 0; i < eh.shnum; i++)
            {
                if (sh[i].type == SHT_SYMTAB)
                {
                    sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, DOSBase);
                    if (!sh[i].addr)
                        break;
                }
            }

            if (i == eh.shnum)
            {
                BPTR  *next_hunk_ptr = &hunks;

                /* Load all loadable PROGBITS and NOBITS sections */
                for (i = 0; i < eh.shnum; i++)
                {
                    if
                    (
                        (sh[i].type == SHT_PROGBITS || sh[i].type == SHT_NOBITS) &&
                        (sh[i].flags & SHF_ALLOC)
                    )
                    {
                        if (!load_hunk(file, &next_hunk_ptr, &sh[i], funcarray, DOSBase))
                            break;
                    }
                }

                {
                    ULONG offset = 0;

                    /* Allocate space for common symbols */
	            for (i = 0; i < eh.shnum; i++)
                    {
                        if (sh[i].type == SHT_SYMTAB)
                        {
                            struct symbol *sym = (struct symbol *)sh[i].addr;
                            ULONG numsyms = sh[i].size / sh[i].entsize;
                            ULONG j;

                            for (j = 0; j < numsyms; j++, sym++)
                            {
                                if (sym->shindex == SHN_COMMON)
                                {
                                    offset       = (offset + sym->value-1) & ~(sym->value-1);
                                    sym->value   = offset;
                                    sym->shindex = eh.shnum; /* The common section's index */

                                    offset += sym->size;
                                }
                            }
                        }
                    }

                    sh[eh.shnum].size = offset;
                    sh[eh.shnum].type = SHT_NOBITS;
		    sh[eh.shnum].addr = NULL;
                    if (!load_hunk(0, &next_hunk_ptr, &sh[eh.shnum], funcarray, DOSBase))
                    {
                        /* force an error */
                        i = 0;
                    }

                }

                if (i == eh.shnum)
                {
                    /* Relocate the sections */
                    for (i = 0; i < eh.shnum; i++)
                    {
                        if ((sh[i].type == SHT_RELA || sh[i].type == SHT_REL) && sh[sh[i].info].addr)
                        {
			    sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, DOSBase);
                            if (sh[i].addr)
                            {
                                if (!relocate(sh, i, DOSBase))
                                    break;
                            }

                            MyFree(sh[i].addr, sh[i].size);
                            sh[i].addr = NULL;
                        }
	            }
                }
            }

            /* There were some errors, deallocate all the allocated sections */
            if (i < eh.shnum)
            {
                for (i = 0; i < eh.shnum; i++)
                {
                    if (sh[i].addr)
                    {
                        if (sh[i].type == SHT_PROGBITS || sh[i].type == SHT_NOBITS)
                            MyFree(sh[i].addr - sizeof(struct hunk), sh[i].size + sizeof(struct hunk));
                        else
                            MyFree(sh[i].addr, sh[i].size);
                    }
                }

 		hunks = 0;
	    }
            /* No errors, deallocate only the symbol tables */
            else
            {
                for (i = 0; i < eh.shnum; i++)
                {
                    if (sh[i].addr && sh[i].type == SHT_SYMTAB)
                        MyFree(sh[i].addr, sh[i].size);
                }
            }

            /* Free the section headers */
            MyFree(sh, (eh.shnum+1) * eh.shentsize);
        }
    }

    return hunks;
}

#undef MyRead
#undef MyAlloc
#undef MyFree
