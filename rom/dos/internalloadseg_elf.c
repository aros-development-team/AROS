/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code to dynamically load ELF executables
    Lang: english

    1997/12/13: Changed filename to internalloadseg_elf.c
                Original file was created by digulla.
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

#define EM_386          3
#define EM_68K          4
#define EM_PPC         20
#define EM_ARM         40
#define EM_X86_64       62      /* AMD x86-64 */

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2

/* AMD x86-64 relocations.  */
#define R_X86_64_NONE   0       /* No reloc */
#define R_X86_64_64     1       /* Direct 64 bit  */
#define R_X86_64_PC32   2       /* PC relative 32 bit signed */

#define R_68k_NONE      0
#define R_68K_32        1
#define R_68K_PC32      4

#define R_PPC_NONE      0
#define R_PPC_ADDR32    1
#define R_PPC_ADDR16_LO 4
#define R_PPC_ADDR16_HA 6
#define R_PPC_REL24     10
#define R_PPC_REL32	26

#define R_ARM_NONE      0
#define R_ARM_PC24      1
#define R_ARM_ABS32     2

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

#define ELF32_ST_TYPE(i)    ((i) & 0x0F)

#define EI_VERSION      6
#define EV_CURRENT      1

#define EI_DATA         5
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

#define EI_CLASS        4
#define ELFCLASS32      1
#define ELFCLASS64      2               /* 64-bit objects */

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

    /* these are internal, and not part of the header proper. they are wider
     * versions of shnum and shstrndx for when they don't fit in the header
     * and we need to get them from the first section header. see
     * load_header() for details
     */
    ULONG int_shnum;
    ULONG int_shstrndx;
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
#if defined(__mc68000__) || defined (__x86_64__) || defined (__ppc__) || defined (__powerpc__) || defined(__arm__)
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
    AROS_CALL2NR                    \
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
            eh->ident[EI_CLASS]   != ELFCLASS32  ||
        eh->ident[EI_VERSION] != EV_CURRENT  ||
        eh->type              != ET_REL      ||

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
#warning ARM has not been tested, yet!

        #else
        #    error Your architecture is not supported
        #endif
    )
    {
        D(bug("[ELF Loader] Object is of wrong type\n"));
        #if defined(__x86_64__)
            D(bug("[ELF Loader] EI_CLASS   is %d - should be %d\n", eh->ident[EI_CLASS],   ELFCLASS64));
	#else
            D(bug("[ELF Loader] EI_CLASS   is %d - should be %d\n", eh->ident[EI_CLASS],   ELFCLASS32));
	#endif
        D(bug("[ELF Loader] EI_VERSION is %d - should be %d\n", eh->ident[EI_VERSION], EV_CURRENT));
        D(bug("[ELF Loader] type       is %d - should be %d\n", eh->type,              ET_REL));
#if defined (__i386__)
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA],ELFDATA2LSB));
#elif defined (__mc68000__)
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA],ELFDATA2MSB));
#elif defined(__ppc__) || defined(__powerpc__)
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA],ELFDATA2MSB));
#elif defined (__arm__)
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA],ELFDATA2MSB));
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

    if (!sh->size)
        return 1;

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
                sh->addr = (char *)AROS_ROUNDUP2((ULONG)hunk->data, sh->addralign);
	}
	else
	    sh->addr = hunk->data;

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

    ULONG numrel = shrel->size / shrel->entsize;
    ULONG i;

    struct symbol *SysBase_sym = NULL;
    
    for (i=0; i<numrel; i++, rel++)
    {
        struct symbol *sym = &symtab[ELF32_R_SYM(rel->info)];
        ULONG *p = (ULONG *)&section[rel->offset];
	ULONG  s;
        ULONG shindex;

        if (sym->shindex != SHN_XINDEX)
            shindex = sym->shindex;

        else {
            if (symtab_shndx == NULL) {
                D(bug("[ELF Loader] got symbol with shndx 0xfff, but there's no symtab shndx table\n"));
                SetIoErr(ERROR_BAD_HUNK);
                return 0;
            }
            shindex = ((ULONG *)symtab_shndx->addr)[ELF32_R_SYM(rel->info)];
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
		    SysBase_yes: s = (ULONG)&SysBase;
		}
		else
		    SysBase_no:  s = sym->value;
                break;

  	    default:
		s = (ULONG)sh[SHINDEX(shindex)].addr + sym->value;
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

            #elif defined(__x86_64__)
		/* These weren't tested */
            case R_X86_64_64: /* 64bit direct/absolute */
                *p = s + rel->addend;
                break;

            case R_X86_64_PC32: /* PC relative 32 bit signed */
                *p = s + rel->addend - (ULONG)p;
                break;

            case R_X86_64_NONE: /* No reloc */
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

            #elif defined(__ppc__) || defined(__powerpc__)

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
		    ULONG temp = s + rel->addend;
		    *c = temp >> 16;
		    if ((temp & 0x8000) != 0)
			(*c)++;
		}
		break;
	    
	    case R_PPC_REL24:
		*p &= ~0x3fffffc;
                *p |= (s + rel->addend - (ULONG) p) & 0x3fffffc;
                break;

	    case R_PPC_REL32:
		*p = s + rel->addend - (ULONG) p;
		break;
	    
            case R_PPC_NONE:
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
                D(bug("[ELF Loader] Unrecognized relocation type %d %d\n", i, ELF32_R_TYPE(rel->info)));
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
    SIPTR             *funcarray,
    SIPTR             *stack __unused,
    struct MinList    *seginfos,
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

    /* load and validate ELF header */
    if (!load_header(file, &eh, funcarray, DOSBase))
        return 0;

    /* load section headers */
    if (!(sh = load_block(file, eh.shoff, eh.int_shnum * eh.shentsize, funcarray, DOSBase)))
        return 0;

    /* load the string table */
    STRPTR st = NULL;
    struct sheader *shstr = sh + SHINDEX(eh.int_shstrndx);
    if (shstr->size != 0)
    {
	st = MyAlloc(shstr->size, MEMF_ANY | MEMF_CLEAR);
        read_block(file, shstr->offset, st, shstr->size, funcarray, DOSBase);
    }

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

		if (seginfos)
		{
		    STRPTR name = st + sh[i].name;
		    ULONG size = sizeof(struct seginfo);
		    struct seginfo *si = MyAlloc(size, MEMF_ANY);

		    D(bug("[ELF Loader] seg %s at 0x%x\n", name, sh[i].addr));

		    si->addr = sh[i].addr;
		    size = sizeof(si->name) - 1;
		    strncpy(si->name, name, size);
		    si->name[size] = '\0';

		    ADDTAIL(seginfos, &si->node);
		}
	    }
        }

    }

    /* Relocate the sections */
    for (i = 0; i < eh.int_shnum; i++)
    {
        if
        (
            #if defined(__i386__)

            sh[i].type == SHT_REL &&

            #elif defined(__x86_64__)

	    sh[i].type == SHT_RELA &&

            #elif defined(__mc68000__)

            sh[i].type == SHT_RELA &&

            #elif defined(__ppc__) || defined(__powerpc__)

            sh[i].type == SHT_RELA &&

            #elif defined(__arm__)
            #warning Missing code for ARM            
//            sh[i].type = SHT_

            #else
            #    error Your architecture is not supported
            #endif

            /* Does this relocation section refer to a hunk? If so, addr must be != 0 */
            sh[SHINDEX(sh[i].info)].addr
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

    /* Free the string table */
    MyFree(st, shstr->size);

    return hunks;
}

#undef MyRead1
#undef MyAlloc
#undef MyFree
