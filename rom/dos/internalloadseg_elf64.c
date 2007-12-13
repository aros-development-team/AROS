/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id: internalloadseg_elf.c 26730 2007-09-19 20:33:25Z schulz $

    Desc: Code to dynamically load 64-bit ELF executables
    Lang: english
*/

#define DEBUG 0

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <aros/asmcall.h>
#include "internalloadseg.h"
#include "dos_intern.h"

#include <aros/debug.h>
#include <string.h>
#include <stddef.h>

#include <aros/macros.h>

#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_NOBITS      8
#define SHT_REL         9
#define SHT_SYMTAB_SHNDX 18

#define ET_REL          1

#define EM_X86_64       62      /* AMD x86-64 */

/* AMD x86-64 relocations.  */
#define R_X86_64_NONE   0       /* No reloc */
#define R_X86_64_64     1       /* Direct 64 bit  */
#define R_X86_64_PC32   2       /* PC relative 32 bit signed */
#define R_X86_64_32     10
#define R_X86_64_32S    11

#define STT_OBJECT      1
#define STT_FUNC        2

#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_XINDEX      0xffff
#define SHN_HIRESERVE   0xffff

#define SHF_ALLOC            (1 << 1)
#define SHF_EXECINSTR        (1 << 2)

#define ELF64_ST_TYPE(i)    ((i) & 0x0F)

#define EI_VERSION      6
#define EV_CURRENT      1

#define EI_DATA         5
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

#define EI_CLASS        4
#define ELFCLASS32      1
#define ELFCLASS64      2               /* 64-bit objects */

#define ELF64_R_SYM(i)      (ULONG)((i) >> 32)
#define ELF64_R_TYPE(i)     (ULONG)((i) & 0xffffffffULL)
#define ELF64_R_INFO(sym, type) (((UQUAD)(sym) << 32) + (type))

struct elfheader
{
    UBYTE       ident[16];
    UWORD       type;
    UWORD       machine;
    ULONG       version;
    APTR        entry;
    UQUAD       phoff;
    UQUAD       shoff;
    ULONG       flags;
    UWORD       ehsize;
    UWORD       phentsize;
    UWORD       phnum;
    UWORD       shentsize;
    UWORD       shnum;
    UWORD       shstrndx;

    /* these are internal, and not part of the header proper. they are wider
     * versions of shnum and shstrndx for when they don't fit in the header
     * and we need to get them from the first section header. see
     * load_header() for details
     */
    ULONG int_shnum;
    ULONG int_shstrndx;
};

struct sheader {
    ULONG       name;
    ULONG       type;
    UQUAD       flags;
    APTR        addr;
    UQUAD       offset;
    UQUAD       size;
    ULONG       link;
    ULONG       info;
    UQUAD       addralign;
    UQUAD       entsize;
};

struct symbol {
    ULONG       name;     /* Offset of the name string in the string table */
    UBYTE       info;     /* What kind of symbol is this ? (global, variable, etc) */
    UBYTE       other;    /* undefined */
    UWORD       shindex;  /* In which section is the symbol defined ? */
    UQUAD       value;    /* Varies; eg. the offset of the symbol in its hunk */
    UQUAD       size;     /* How much memory does the symbol occupy */
};

struct relo {
    UQUAD       offset;   /* Address of the relocation relative to the section it refers to */
    UQUAD       info;     /* Type of the relocation */
    QUAD        addend;   /* Constant addend used to compute value */
};

struct hunk
{
    ULONG size;
    BPTR  next;
    char  data[0];
} __attribute__((packed));

#define BPTR2HUNK(bptr) ((struct hunk *)((char *)BADDR(bptr) - offsetof(struct hunk, next)))
#define HUNK2BPTR(hunk) MKBADDR(&hunk->next)

/* convert section header number to array index */
#define SHINDEX(n) \
    ((n) < SHN_LORESERVE ? (n) : ((n) <= SHN_HIRESERVE ? 0 : (n) - (SHN_HIRESERVE + 1 - SHN_LORESERVE)))

/* convert section header array index to section number */
#define SHNUM(i) \
    ((i) < SHN_LORESERVE ? (i) : (i) + (SHN_HIRESERVE + 1 - SHN_LORESERVE))

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

#if defined (__x86_64__)

static int read_block
(
    BPTR               file,
    ULONG              offset,
    APTR               buffer,
    ULONG              size,
    SIPTR              *funcarray,
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
    SIPTR             *funcarray,
    struct DosLibrary *DOSBase
)
{
    D(bug("[ELF Loader] Load Block\n"));
    D(bug("[ELF Loader] (size=%d)\n",size));
    D(bug("[ELF Loader] (funcarray=0x%x)\n",funcarray));
    D(bug("[ELF Loader] (funcarray[1]=0x%x)\n",funcarray[1]));
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

static int load_header(BPTR file, struct elfheader *eh, SIPTR *funcarray, struct DosLibrary *DOSBase) {
   if (!read_block(file, 0, eh, offsetof(struct elfheader, int_shnum), funcarray, DOSBase))
       return 0;

   if (eh->ident[0] != 0x7f || eh->ident[1] != 'E'  ||
       eh->ident[2] != 'L'  || eh->ident[3] != 'F') {
	D(bug("[ELF Loader] Not an ELF object\n"));
        SetIoErr(ERROR_NOT_EXECUTABLE);
        return 0;
    }
    D(bug("[ELF Loader] ELF object\n"));

    eh->int_shnum = eh->shnum;
    eh->int_shstrndx = eh->shstrndx;

    /* the ELF header only uses 16 bits to store the count of section headers,
     * so it can't handle more than 65535 headers. if the count is 0, and an
     * offset is defined, then the real count can be found in the first
     * section header (which always exists).
     *
     * similarly, if the string table index is SHN_XINDEX, then the actual
     * index is found in the first section header also.
     *
     * see the System V ABI 2001-04-24 draft for more details.
     */
    if (eh->int_shnum == 0 || eh->int_shstrndx == SHN_XINDEX) {
        if (eh->shoff == 0) {
            SetIoErr(ERROR_NOT_EXECUTABLE);
            return 0;
        }

        struct sheader sh;
        if (!read_block(file, eh->shoff, &sh, sizeof(sh), funcarray, DOSBase))
            return 0;

        /* wider section header count is in the size field */
        if (eh->int_shnum == 0)
            eh->int_shnum = sh.size;

        /* wider string table index is in the link field */
        if (eh->int_shstrndx == SHN_XINDEX)
            eh->int_shstrndx = sh.link;

        /* sanity, if they're still invalid then this isn't elf */
        if (eh->int_shnum == 0 || eh->int_shstrndx == SHN_XINDEX) {
            SetIoErr(ERROR_NOT_EXECUTABLE);
            return 0;
        }
    }

    if
    (
            eh->ident[EI_CLASS]   != ELFCLASS64  ||
        eh->ident[EI_VERSION] != EV_CURRENT  ||
        eh->type              != ET_REL      ||

            eh->ident[EI_DATA] != ELFDATA2LSB ||
	    eh->machine        != EM_X86_64
	    
    )
    {
        D(bug("[ELF Loader] Object is of wrong type\n"));
            D(bug("[ELF Loader] EI_CLASS   is %d - should be %d\n", eh->ident[EI_CLASS],   ELFCLASS64));
        D(bug("[ELF Loader] EI_VERSION is %d - should be %d\n", eh->ident[EI_VERSION], EV_CURRENT));
        D(bug("[ELF Loader] type       is %d - should be %d\n", eh->type,              ET_REL));
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA],ELFDATA2LSB));
        D(bug("[ELF Loader] machine    is %d - should be %d\n", eh->machine, EM_X86_64));

        SetIoErr(ERROR_NOT_EXECUTABLE);
        return 0;
    }

    return 1;
}

static int load_hunk
(
    BPTR                 file,
    BPTR               **next_hunk_ptr,
    struct sheader      *sh,
    SIPTR               *funcarray,
    BOOL                 do_align,
    struct DosLibrary   *DOSBase
)
{
    struct hunk *hunk;
    ULONG   hunk_size;

    D(bug("[dos:ELF64] load_hunk. Do align=%d\n", do_align));
    
    if (!sh->size)
        return 1;
    
    D(bug("[dos:ELF64] sh->size=%d, sh->addraligh=%d\n", sh->size, sh->addralign));

    /* The size of the hunk is the size of the section, plus
       the size of the hunk structure, plus the size of the alignment (if necessary)*/
    hunk_size = sh->size + sizeof(struct hunk);

    if (do_align)
    {
        hunk_size += sh->addralign;

        /* Also create space for a trampoline, if necessary */
        if (sh->flags & SHF_EXECINSTR)
            hunk_size += sizeof(struct FullJumpVec);
    }

    hunk = MyAlloc(hunk_size, MEMF_ANY | (sh->type == SHT_NOBITS) ? MEMF_CLEAR : 0);
    
    D(bug("[dos:ELF64] hunk=%012p\n", hunk));
    
    if (hunk)
    {
        hunk->next = 0;
        hunk->size = hunk_size;

        /* In case we are required to honour alignment, and If this section contains
	   executable code, create a trampoline to its beginning, so that even if the
	   alignment requirements make the actual code go much after the end of the
	   hunk structure, the code can still be reached in the usual way.  */
        if (do_align)
        {
            if (sh->flags & SHF_EXECINSTR)
            {
                sh->addr = (char *)AROS_ROUNDUP2
                (
                 (ULONG)hunk->data + sizeof(struct FullJumpVec), sh->addralign
                );
                __AROS_SET_FULLJMP((struct FullJumpVec *)hunk->data, sh->addr);
            }
            else
                sh->addr = (char *)AROS_ROUNDUP2((IPTR)hunk->data, sh->addralign);
                
            D(bug("[dos:ELF64] align. %012p -> %012p\n", hunk->data, sh->addr));
        }
        else
            sh->addr = hunk->data;

        (bug("[dos:ELF64] sh->addr = %012lx - %012lx\n", sh->addr, sh->addr + sh->size - 1));
        
        /* Link the previous one with the new one */
        BPTR2HUNK(*next_hunk_ptr)->next = HUNK2BPTR(hunk);

        /* Update the pointer to the previous one, which is now the current one */
        *next_hunk_ptr = HUNK2BPTR(hunk);

        if (sh->type != SHT_NOBITS)
            return read_block(file, sh->offset, sh->addr, sh->size, funcarray, DOSBase);

        return 1;
    }

    SetIoErr(ERROR_NO_FREE_STORE);

    return 0;
}

static int relocate
(
    struct elfheader  *eh,
    struct sheader    *sh,
    ULONG              shrel_idx,
    struct sheader    *symtab_shndx,
    struct DosLibrary *DOSBase
)
{
    struct sheader *shrel    = &sh[shrel_idx];
    struct sheader *shsymtab = &sh[SHINDEX(shrel->link)];
    struct sheader *toreloc  = &sh[SHINDEX(shrel->info)];

    struct symbol *symtab   = (struct symbol *)shsymtab->addr;
    struct relo   *rel      = (struct relo *)shrel->addr;
    char          *section  = toreloc->addr;

    /* this happens if the target section has no allocation. that can happen
     * eg. with a .debug PROGBITS and a .rel.debug section */
    if (section == NULL)
        return 1;

    D(bug("[dos:ELF64] Relocating section at %012p\n", section));
    
    ULONG numrel = shrel->size / shrel->entsize;
    ULONG i;

    struct symbol *SysBase_sym = NULL;
    
    for (i=0; i<numrel; i++, rel++)
    {
        struct symbol *sym = &symtab[ELF64_R_SYM(rel->info)];
        ULONG *p = (ULONG *)&section[rel->offset];
  	UQUAD  s;
        ULONG shindex;

        if (sym->shindex != SHN_XINDEX)
            shindex = sym->shindex;

        else {
            if (symtab_shndx == NULL) {
                D(bug("[ELF Loader] got symbol with shndx 0xfff, but there's no symtab shndx table\n"));
                SetIoErr(ERROR_BAD_HUNK);
                return 0;
            }
            shindex = ((ULONG *)symtab_shndx->addr)[ELF64_R_SYM(rel->info)];
        }

        switch (shindex)
        {

            case SHN_UNDEF:
                D(bug("[ELF Loader] Undefined symbol '%s' while relocating the section '%s'\n",
                      (STRPTR)sh[SHINDEX(shsymtab->link)].addr + sym->name,
                      (STRPTR)sh[SHINDEX(eh->int_shstrndx)].addr + toreloc->name));
                      SetIoErr(ERROR_BAD_HUNK);
                return 0;

            case SHN_COMMON:
                D(bug("[ELF Loader] COMMON symbol '%s' while relocating the section '%s'\n",
                      (STRPTR)sh[SHINDEX(shsymtab->link)].addr + sym->name,
                      (STRPTR)sh[SHINDEX(eh->int_shstrndx)].addr + toreloc->name));
                      SetIoErr(ERROR_BAD_HUNK);
		      
                return 0;

            case SHN_ABS:
	        if (SysBase_sym == NULL)
		{
                    if (strncmp((STRPTR)sh[SHINDEX(shsymtab->link)].addr + sym->name, "SysBase", 8) == 0)
		    {
		        SysBase_sym = sym;
			goto SysBase_yes;
	            }
		    else
		        goto SysBase_no;
	        }
		else
		if (SysBase_sym == sym)
		{
		    SysBase_yes: s = (IPTR)&SysBase;
		}
		else
		    SysBase_no:  s = sym->value;
                break;

  	    default:
                s = (UQUAD)sh[SHINDEX(shindex)].addr + sym->value;
 	}

        switch (ELF64_R_TYPE(rel->info))
        {
            #if defined(__x86_64__)
		/* These weren't tested */
            case R_X86_64_64: /* 64bit direct/absolute */
                *(UQUAD *)p = s + rel->addend;
                break;

            case R_X86_64_PC32: /* PC relative 32 bit signed */
                *p = s + rel->addend - (ULONG)p;
                break;
                
            case R_X86_64_32:
                *(ULONG *)p = (UQUAD)s + (UQUAD)rel->addend;
                break;
                
            case R_X86_64_32S:
                *(LONG *)p = (QUAD)s + (QUAD)rel->addend;
                break;

            case R_X86_64_NONE: /* No reloc */
                break;	

            #endif

            default:
                D(bug("[ELF Loader] Unrecognized relocation type %d %d\n", i, ELF64_R_TYPE(rel->info)));
                SetIoErr(ERROR_BAD_HUNK);
		return 0;
        }
    }

    return 1;
}

#endif

BPTR InternalLoadSeg_ELF64
(
    BPTR               file,
    BPTR               table __unused,
    SIPTR             *funcarray,
    SIPTR             *stack __unused,
    struct DosLibrary *DOSBase
)
{
    struct elfheader  eh;
    struct sheader   *sh;
    struct sheader   *symtab_shndx = NULL;
    BPTR   hunks         = 0;
    BPTR  *next_hunk_ptr = &hunks;
    ULONG  i;
    BOOL   exec_hunk_seen = FALSE;

#if defined (__x86_64__)

    /* load and validate ELF header */
    if (!load_header(file, &eh, funcarray, DOSBase))
        return 0;

    /* load section headers */
    if (!(sh = load_block(file, eh.shoff, eh.int_shnum * eh.shentsize, funcarray, DOSBase)))
        return 0;

    /* Iterate over the section headers in order to do some stuff... */
    for (i = 0; i < eh.int_shnum; i++)
    {
        /*
           Load the symbol and string table(s).

           NOTICE: the ELF standard, at the moment (Nov 2002) explicitely states
                   that only one symbol table per file is allowed. However, it
                   also states that this may change in future... we already handle it.
        */
        if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB || sh[i].type == SHT_SYMTAB_SHNDX)
        {
            sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, DOSBase);
            if (!sh[i].addr)
                goto error;

            if (sh[i].type == SHT_SYMTAB_SHNDX) {
                if (symtab_shndx == NULL)
                    symtab_shndx = &sh[i];
                else
                    D(bug("[ELF Loader] file contains multiple symtab shndx tables. only using the first one\n"));
            }
        }
        else
        /* Load the section in memory if needed, and make an hunk out of it */
        if (sh[i].flags & SHF_ALLOC)
        {
	    if (sh[i].size)
	    {
	        /* Only allow alignment if this is an executable hunk
		   or if an executable hunk has been loaded already,
		   so to avoid the situation in which a data hunk has its
		   content displaced from the hunk's header in case it's the
		   first hunk (this happens with Keymaps, for instance).  */
	        if (sh[i].flags & SHF_EXECINSTR)
		    exec_hunk_seen = TRUE;

                if (!load_hunk(file, &next_hunk_ptr, &sh[i], funcarray, exec_hunk_seen, DOSBase))
                    goto error;
	    }
        }

    }

    /* Relocate the sections */
    for (i = 0; i < eh.int_shnum; i++)
    {
        if
        (
            #if defined(__x86_64__)

	    sh[i].type == SHT_RELA &&

            #endif

            /* Does this relocation section refer to a hunk? If so, addr must be != 0 */
            sh[sh[i].info].addr
        )
        {
	    sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, DOSBase);
            if (!sh[i].addr || !relocate(&eh, sh, i, symtab_shndx, DOSBase))
                goto error;

            MyFree(sh[i].addr, sh[i].size);
            sh[i].addr = NULL;
        }
    }


    goto end;

error:

    /* There were some errors, deallocate The hunks */

    InternalUnLoadSeg(hunks, (VOID_FUNC)funcarray[2]);
    hunks = 0;

end:
    
    /* Clear the caches to let the CPU see the new data and instructions */
    {
        BPTR curr = hunks;
        while (curr)
        {
             struct hunk *hunk = BPTR2HUNK(curr);
             
	     CacheClearE(hunk->data, hunk->size, CACRF_ClearD | CACRF_ClearI);
             
             curr = hunk->next;
        }
    }
    
    /* deallocate the symbol tables */
    for (i = 0; i < eh.int_shnum; i++)
    {
        if (((sh[i].type == SHT_SYMTAB) || (sh[i].type == SHT_STRTAB)) && (sh[i].addr != NULL))
            MyFree(sh[i].addr, sh[i].size);
    }

    /* Free the section headers */
    MyFree(sh, eh.int_shnum * eh.shentsize);

#else
    SetIoErr(ERROR_NOT_EXECUTABLE);
#endif

    return hunks;
}

#undef MyRead1
#undef MyAlloc
#undef MyFree
