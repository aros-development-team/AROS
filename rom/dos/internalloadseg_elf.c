/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Code to dynamically load ELF executables
*/

#include <aros/debug.h>

#include <proto/dos.h>
#include <proto/arossupport.h>
#include <proto/debug.h>
#include <proto/exec.h>

#include <aros/asmcall.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <dos/elf.h>
#include <dos/dosasl.h>
#include <libraries/debug.h>
#include <resources/processor.h>

#include <string.h>
#include <stddef.h>

#include "internalloadseg.h"
#include "dos_intern.h"
#include "include/loadseg.h"

#define DATTR(x)

struct hunk
{
    ULONG size;
    BPTR  next;
    char  data[0];
} __attribute__((packed));

#define BPTR2HUNK(bptr) ((struct hunk *)((void *)bptr - offsetof(struct hunk, next)))
#define HUNK2BPTR(hunk) MKBADDR(&hunk->next)

#define LOADSEG_SMALL_READ  4096 /* Size adjusted by profiling in Callgrind */

/* [S]mall [R]eads [Buffer]
 *
 * Due to a fact that Seek flushes buffers, FRead that is used to do small reads
 * cannot use the buffer. The problem is visible with C++ executables that
 * have a big number of small sections and thus do many (in tens of thousands
 * for Odyssey) very small reads.
 *
 * */
struct SRBuffer
{
    ULONG   srb_FileOffset;
    APTR    srb_Buffer;
};

static int elf_read_block
(
    BPTR               file,
    ULONG              offset,
    APTR               buffer,
    ULONG              size,
    SIPTR              *funcarray,
    struct SRBuffer    *srb,
    struct DosLibrary  *DOSBase
)
{
    D(bug("[ELF Loader] elf_read_block (offset=%d, size=%d)\n", offset, size));

    if (size <= LOADSEG_SMALL_READ && (offset >= srb->srb_FileOffset) &&
            ((offset + size) <= (srb->srb_FileOffset + LOADSEG_SMALL_READ)) &&
            srb->srb_Buffer != NULL)
    {
        CopyMem(srb->srb_Buffer + (offset - srb->srb_FileOffset), buffer, size);
        return 0;
    }
    else
    {
        if (ilsSeek(file, offset, OFFSET_BEGINNING) < 0)
            return 1;

        if (size <= LOADSEG_SMALL_READ)
        {
            if (srb->srb_Buffer == NULL)
                srb->srb_Buffer = AllocMem(LOADSEG_SMALL_READ, MEMF_ANY);

            srb->srb_FileOffset = offset;

            /* Fill the buffer */
            read_block(file, srb->srb_Buffer, LOADSEG_SMALL_READ, funcarray, DOSBase);
            /* Re-read, now srb will be used */
            return elf_read_block(file, offset, buffer, size, funcarray, srb, DOSBase);
        }
        else
        {
            return read_block(file, buffer, size, funcarray, DOSBase);
        }
    }
}

static void *load_block
(
    BPTR               file,
    ULONG              offset,
    ULONG              size,
    SIPTR             *funcarray,
    struct SRBuffer   *srb,
    struct DosLibrary *DOSBase
)
{
    D(bug("[ELF Loader] Load Block\n"));
    D(bug("[ELF Loader] (size=%d)\n",size));
    D(bug("[ELF Loader] (funcarray=0x%x)\n",funcarray));
    D(bug("[ELF Loader] (funcarray[1]=0x%x)\n",funcarray[1]));
    void *block = ilsAllocMem(size, MEMF_ANY);
    if (block)
    {
        if (!elf_read_block(file, offset, block, size, funcarray, srb, DOSBase))
            return block;

        ilsFreeMem(block, size);
    }
    else
        SetIoErr(ERROR_NO_FREE_STORE);

    return NULL;
}

static ULONG read_shnum(BPTR file, struct elfheader *eh, SIPTR *funcarray, struct SRBuffer *srb, struct DosLibrary *DOSBase)
{
    ULONG shnum = eh->shnum;

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
    if (eh->shnum == 0)
    {
        struct sheader sh;

        if (eh->shoff == 0) {
            D(bug("[ELF Loader] No section header\n"));
            SetIoErr(ERROR_NOT_EXECUTABLE);
            return 0;
        }

        if (elf_read_block(file, eh->shoff, &sh, sizeof(sh), funcarray, srb, DOSBase))
            return 0;

        /* wider section header count is in the size field */
        shnum = sh.size;

        /* sanity, if they're still invalid then this isn't elf */
        if (shnum == 0) {
            D(bug("[ELF Loader] Empty section header\n"));
            SetIoErr(ERROR_NOT_EXECUTABLE);
        }
    }

    return shnum;
}

static int load_header(BPTR file, struct elfheader *eh, SIPTR *funcarray, struct SRBuffer *srb, struct DosLibrary *DOSBase)
{
    if (elf_read_block(file, 0, eh, sizeof(struct elfheader), funcarray, srb, DOSBase))
        return 0;

    if (eh->ident[0] != 0x7f || eh->ident[1] != 'E'  ||
        eh->ident[2] != 'L'  || eh->ident[3] != 'F') {
        D(bug("[ELF Loader] Not an ELF object\n"));
        SetIoErr(ERROR_NOT_EXECUTABLE);
        return 0;
    }
    D(bug("[ELF Loader] ELF object\n"));

    /* WANT_CLASS should be defined for your target */
    if (eh->ident[EI_CLASS]   != AROS_ELF_CLASS  ||
        eh->ident[EI_VERSION] != EV_CURRENT      ||
        eh->type              != ET_REL          ||
        eh->ident[EI_DATA]    != AROS_ELF_DATA   ||
        eh->machine           != AROS_ELF_MACHINE)
    {
        D(bug("[ELF Loader] Object is of wrong type\n"));
        D(bug("[ELF Loader] EI_CLASS   is %d - should be %d\n", eh->ident[EI_CLASS]  , AROS_ELF_CLASS ));
        D(bug("[ELF Loader] EI_VERSION is %d - should be %d\n", eh->ident[EI_VERSION], EV_CURRENT     ));
        D(bug("[ELF Loader] type       is %d - should be %d\n", eh->type             , ET_REL         ));
        D(bug("[ELF Loader] EI_DATA    is %d - should be %d\n", eh->ident[EI_DATA]   , AROS_ELF_DATA  ));
        D(bug("[ELF Loader] machine    is %d - should be %d\n", eh->machine          , AROS_ELF_MACHINE));

        SetIoErr(ERROR_NOT_EXECUTABLE);
        return 0;
    }

    return 1;
}

/* Room a section needs inside a hunk, mirrored by the arena sizing pass.
   The size of the hunk is the size of the section, plus the size of the
   hunk structure, plus the size of the alignment (if necessary). */
static ULONG elf_hunk_size(struct sheader *sh, BOOL do_align)
{
    ULONG hunk_size = sh->size + sizeof(struct hunk);

    if (do_align)
    {
         hunk_size += sh->addralign;

         /* Also create space for a trampoline, if necessary */
         if (sh->flags & SHF_EXECINSTR)
             hunk_size += sizeof(struct FullJumpVec);
    }

    return hunk_size;
}

static int __attribute__ ((noinline)) load_hunk
(
    BPTR                 file,
    BPTR               **next_hunk_ptr,
    struct sheader      *sh,
    CONST_STRPTR         strtab,
    SIPTR               *funcarray,
    BOOL                 do_align,
    UBYTE              **arena_cur,
    ULONG                extra_memflags,
    struct SRBuffer     *srb,
    struct DosLibrary   *DOSBase
)
{
    struct hunk *hunk;
    ULONG  hunk_size;
    ULONG  memflags = extra_memflags;

    if (!sh->size)
        return 1;

    hunk_size = elf_hunk_size(sh, do_align);

    if (strtab) {
        CONST_STRPTR nameext;

        nameext = strrchr(strtab + sh->name, '.');
        if (nameext) {
            if (strcmp(nameext, ".MEMF_CHIP")==0) {
                memflags |= MEMF_CHIP;
            } else if (strcmp(nameext, ".MEMF_LOCAL")==0) {
                memflags |= MEMF_LOCAL;
            } else if (strcmp(nameext, ".MEMF_KICK")==0) {
                memflags |= MEMF_KICK;
            } else if (strcmp(nameext, ".MEMF_FAST")==0) {
                memflags |= MEMF_FAST;
            } else if (strcmp(nameext, ".MEMF_PUBLIC")==0) {
                memflags |= MEMF_PUBLIC;
            }
        }
    }

    /* Executable code must come from executable memory (matters on W^X hosts) */
    if (sh->flags & SHF_EXECINSTR)
        memflags |= MEMF_EXECUTABLE;

    if (arena_cur)
    {
        /* Carve the hunk out of the module's arena. The stored size of 0
           makes ilsFreeVec() skip this node on UnLoadSeg(); the memory
           belongs to the container hunk linked at the end of the chain. */
        hunk = (struct hunk *)AROS_ROUNDUP2((IPTR)*arena_cur, AROS_WORSTALIGN);
        *arena_cur = (UBYTE *)hunk + hunk_size;
        hunk->next = 0;
        hunk->size = 0;
    }
    else
        hunk = ilsAllocMem(hunk_size, memflags | MEMF_PUBLIC | (sh->type == SHT_NOBITS ? MEMF_CLEAR : 0));

    if (hunk)
    {
        hunk->next = 0;
        if (!arena_cur)
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
                    (IPTR)hunk->data + sizeof(struct FullJumpVec), sh->addralign
                );
                __AROS_SET_FULLJMP((struct FullJumpVec *)hunk->data, sh->addr);
            }
            else
                sh->addr = (char *)AROS_ROUNDUP2((IPTR)hunk->data, sh->addralign);
        }
        else
            sh->addr = hunk->data;

        /* Link the previous one with the new one */
        BPTR2HUNK(*next_hunk_ptr)->next = HUNK2BPTR(hunk);

        D(bug("[dos] hunk @ %p, size=%08x, addr @ %p\n", hunk, hunk->size, sh->addr));

        /* Update the pointer to the previous one, which is now the current one */
        *next_hunk_ptr = (APTR)((IPTR)hunk + offsetof(struct hunk, next));

        if (sh->type != SHT_NOBITS)
            return !elf_read_block(file, sh->offset, sh->addr, sh->size, funcarray, srb, DOSBase);

        return 1;

    }

    SetIoErr(ERROR_NO_FREE_STORE);

    return 0;
}

#ifdef __aarch64__
/*
 * Minimal GOT for dynamically loaded modules: ADR_GOT_PAGE/LD64_GOT_LO12_NC
 * need a GOT entry holding the symbol address. Entries live in hunk-backed
 * blocks linked into the seglist so UnLoadSeg frees them; blocks chain when
 * full, so there is no cap on distinct symbols. AArch64 has no arena (that is
 * x86_64-only), so each block is an individual ilsAllocMem hunk, matching how
 * load_hunk allocates the section hunks.
 *
 * The chain is owned by one relocate() call, ie. it is per relocation section
 * rather than per module. In practice a module has GOT relocations in a single
 * section, so this costs nothing; a symbol referenced from two sections just
 * gets an entry in each.
 */
#define AARCH64_DYN_GOT_BLOCKENTRIES 64

struct aarch64_got_block
{
    struct aarch64_got_block *next;
    IPTR  count;
    IPTR  entries[AARCH64_DYN_GOT_BLOCKENTRIES];
};

/*
 * Return the GOT entry for sym_addr, allocating it (and a block if needed) on
 * demand; NULL with IoErr set on failure. Relocs are unordered, so the ADRP
 * and LDR of a pair share one entry via this lookup.
 */
static IPTR *aarch64_got_entry
(
    struct aarch64_got_block **got_head,
    IPTR sym_addr,
    SIPTR *funcarray,       /* needed by the ilsAllocMem macro */
    BPTR **next_hunk_ptr,
    struct DosLibrary *DOSBase
)
{
    struct aarch64_got_block *blk, *last = NULL;

    for (blk = *got_head; blk; blk = blk->next)
    {
        IPTR gi;
        for (gi = 0; gi < blk->count; gi++)
        {
            if (blk->entries[gi] == sym_addr)
                return &blk->entries[gi];
        }
        last = blk;
    }

    if (!last || last->count >= AARCH64_DYN_GOT_BLOCKENTRIES)
    {
        struct hunk *got_hunk;
        struct aarch64_got_block *newblk;
        /* + 8 lets us 8-byte align the block inside the hunk: the
         * LD64_GOT_LO12_NC scaling (>>3) needs 8-byte entries. */
        ULONG got_size = sizeof(struct hunk)
                       + sizeof(struct aarch64_got_block) + 8;

        got_hunk = ilsAllocMem(got_size, MEMF_PUBLIC | MEMF_CLEAR);
        if (!got_hunk)
        {
            bug("[ELF Loader] Failed to allocate GOT block\n");
            SetIoErr(ERROR_NO_FREE_STORE);
            return NULL;
        }
        got_hunk->next = 0;
        got_hunk->size = got_size;

        /* Link onto the end of the seglist chain via the shared pointer. */
        BPTR2HUNK(*next_hunk_ptr)->next = HUNK2BPTR(got_hunk);
        *next_hunk_ptr = (APTR)((IPTR)got_hunk + offsetof(struct hunk, next));

        newblk = (struct aarch64_got_block *)AROS_ROUNDUP2((IPTR)got_hunk->data, 8);
        newblk->next = NULL;
        newblk->count = 0;

        if (last)
            last->next = newblk;
        else
            *got_head = newblk;
        last = newblk;
    }

    last->entries[last->count] = sym_addr;
    return &last->entries[last->count++];
}
#endif /* __aarch64__ */

static int relocate
(
    struct elfheader  *eh,
    struct sheader    *sh,
    ULONG              shrel_idx,
    struct sheader    *symtab_shndx,
    BOOL              *reloc_out_of_range,
    SIPTR             *funcarray,
    BPTR             **next_hunk_ptr,
    struct DosLibrary *DOSBase
)
{
    struct sheader *shrel    = &sh[shrel_idx];
    struct sheader *shsymtab = &sh[shrel->link];
    struct sheader *toreloc  = &sh[shrel->info];

    struct symbol *symtab   = (struct symbol *)shsymtab->addr;
    UBYTE         *relbase  = (UBYTE *)shrel->addr;
    /*
     * Some toolchains emit SHT_REL (8-byte: offset+info, addend in the
     * field) while others emit SHT_RELA (12-byte: offset+info+addend).
     */
#if defined(__arm__)
    BOOL is_rela = (shrel->type == SHT_RELA);
#endif

    /*
     * Ignore relocs if the target section has no allocation. that can happen
     * eg. with a .debug PROGBITS and a .rel.debug section
     */
    if (!(toreloc->flags & SHF_ALLOC))
        return 1;

    ULONG numrel = shrel->size / shrel->entsize;
    ULONG entsize = shrel->entsize;
    ULONG i;
#ifdef __aarch64__
    /* Chain of dynamically allocated GOT blocks, see aarch64_got_entry() */
    struct aarch64_got_block *aarch64_got = NULL;
#endif
#if defined(__i386__) || defined(__x86_64__)
    IPTR got_base = 0;

    for (int i = 0; i < eh->shnum; ++i) {
        const char *name = (const char *)((UBYTE *)sh[eh->shstrndx].addr + sh[i].name);
        if (strcmp(name, ".got") == 0) {
            got_base = (IPTR)sh[i].addr;
            break;
        }
    }
#endif

    for (i=0; i<numrel; i++)
    {
        struct relo *rel = (struct relo *)(relbase + i * entsize);
        struct symbol *sym;
        ULONG *p;
        IPTR s;
        ULONG shindex;

#ifdef __arm__
        /*
         * R_ARM_V4BX are actually special marks for the linker.
         * They even never have a target (shindex == SHN_UNDEF),
         * so we simply ignore them before doing any checks.
         */
        if (ELF_R_TYPE(rel->info) == R_ARM_V4BX)
            continue;
#endif

        sym = &symtab[ELF_R_SYM(rel->info)];
        p = toreloc->addr + rel->offset;

        DB2(bug("[ELF Loader] Processing symbol %s\n", sh[shsymtab->link].addr + sym->name));

        switch (sym->shindex)
        {
            case SHN_COMMON:
                D(bug("[ELF Loader] COMMON symbol '%s'\n",
                      (STRPTR)sh[shsymtab->link].addr + sym->name));
                      SetIoErr(ERROR_BAD_HUNK);
                return 0;

            case SHN_ABS:
                s = sym->value;
                break;

            case SHN_UNDEF:
                if (ELF_R_TYPE(rel->info) != 0) {
                    D(bug("[ELF Loader] Undefined symbol '%s'\n",
                      (STRPTR)sh[shsymtab->link].addr + sym->name));
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                /* fall through */

            default:
                if (sym->shindex != SHN_XINDEX)
                    shindex = sym->shindex;
                else {
                    if (symtab_shndx == NULL) {
                        D(bug("[ELF Loader] got symbol with shndx 0xfff, but there's no symtab shndx table\n"));
                        SetIoErr(ERROR_BAD_HUNK);
                        return 0;
                    }
                    shindex = ((ULONG *)symtab_shndx->addr)[ELF_R_SYM(rel->info)];
                }
                s = (IPTR)sh[shindex].addr + sym->value;
        }

        switch (ELF_R_TYPE(rel->info))
        {
            #if defined(__i386__)

            case R_386_32: /* 32bit absolute */
                *p += s;
                break;

            case R_386_PC32: /* 32bit PC relative */
                *p += s - (ULONG)p;
                break;

            case R_386_GOTPC: /* PC-relative offset to GOT */
                if (!got_base) {
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                *p = got_base - (ULONG)p;
                break;

            case R_386_NONE:
                break;

            #elif defined(__x86_64__)
            case R_X86_64_64: /* 64bit direct/absolute */
                *(UQUAD *)p = s + rel->addend;
                break;

            case R_X86_64_PLT32:
            case R_X86_64_PC32: /* PC relative 32 bit signed */
            {
                /* The result must fit in a signed 32 bit displacement. If the
                   hunks were allocated more than +/-2GB apart, refuse the load
                   instead of silently truncating (which crashes at run time). */
                QUAD val = (QUAD)s + (QUAD)rel->addend - (QUAD)(IPTR)p;
                if (val != (QUAD)(LONG)val)
                {
                    D(bug("[ELF Loader] R_X86_64_PC32/PLT32 out of range: '%s' %p -> %p\n",
                          (STRPTR)sh[shsymtab->link].addr + sym->name, p,
                          (void *)(s + rel->addend)));
                    if (reloc_out_of_range)
                        *reloc_out_of_range = TRUE;
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                *(ULONG *)p = val;
                break;
            }

            case R_X86_64_32:
            {
                UQUAD val = (UQUAD)s + (UQUAD)rel->addend;
                if (val != (UQUAD)(ULONG)val)
                {
                    D(bug("[ELF Loader] R_X86_64_32 out of range: '%s' -> %p\n",
                          (STRPTR)sh[shsymtab->link].addr + sym->name, (void *)val));
                    if (reloc_out_of_range)
                        *reloc_out_of_range = TRUE;
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                *(ULONG *)p = val;
                break;
            }

            case R_X86_64_32S:
            {
                QUAD val = (QUAD)s + (QUAD)rel->addend;
                if (val != (QUAD)(LONG)val)
                {
                    D(bug("[ELF Loader] R_X86_64_32S out of range: '%s' -> %p\n",
                          (STRPTR)sh[shsymtab->link].addr + sym->name, (void *)val));
                    if (reloc_out_of_range)
                        *reloc_out_of_range = TRUE;
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                *(LONG *)p = val;
                break;
            }

            case R_X86_64_PC64:
                *(UQUAD *)p = (UQUAD)s + (UQUAD)rel->addend - (IPTR) p;
                break;

            case R_X86_64_GOTOFF64:
                if (!got_base) {
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                *(UQUAD *)p = (UQUAD)s + (UQUAD)rel->addend - (UQUAD)got_base;
                break;

            case R_X86_64_NONE: /* No reloc */
                break;

            #elif defined(__mc68000__)

            case R_68K_32:
                *p = s + rel->addend;
                break;

            case R_68K_16:
                *(UWORD *)p = s + rel->addend;
                break;

            case R_68K_8:
                *(UBYTE *)p = s + rel->addend;
                break;

            case R_68K_PC32:
                *p = s + rel->addend - (ULONG)p;
                break;

            case R_68K_PC16:
                *(UWORD *)p = s + rel->addend - (ULONG)p;
                break;

            case R_68K_PC8:
                *(UBYTE *)p = s + rel->addend - (ULONG)p;
                break;

            case R_68K_NONE:
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

            case R_PPC_REL16_LO:
                {
                    unsigned short *c = (unsigned short *) p;
                    *c = (s + rel->addend - (ULONG) p) & 0xffff;
                }
                break;

            case R_PPC_REL16_HA:
                {
                    unsigned short *c = (unsigned short *) p;
                    ULONG temp = s + rel->addend - (ULONG) p;
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
            case R_ARM_CALL:
            case R_ARM_JUMP24:
            case R_ARM_PC24:
            {
                LONG addend;
                if (is_rela) {
                    addend = (LONG)((struct rela *)rel)->addend;
                } else {
                    /* SHT_REL: 24-bit signed offset (in units of 4 bytes)
                     * encoded in the BL/B instruction's low 24 bits. */
                    addend = (LONG)((*p & 0x00FFFFFFu) << 2);
                    if (addend & 0x02000000)
                        addend -= 0x04000000;
                }
                LONG temp = (LONG)(addend + s - (IPTR)p);

                if (temp & 3) {
                    kprintf("[ELF Loader] CALL/JUMP24/PC24 unaligned\n");
                    return 0;
                }

                LONG imm24 = temp / 4;

                // Range check for signed 24-bit value: [-2^23, 2^23 - 1].
                if (imm24 < -(1<<23) || imm24 > ((1<<23)-1)) {
                    kprintf("[ELF Loader] CALL/JUMP24/PC24 overflow\n");
                    return 0;
                }

                // Insert the 24-bit immediate into the instruction.
                *p = (*p & 0xFF000000u) | ((ULONG)imm24 & 0x00FFFFFFu);
            }
            break;

            case R_ARM_PREL31:
            {
                LONG addend;
                if (is_rela) {
                    addend = (LONG)((struct rela *)rel)->addend;
                } else {
                    /* SHT_REL: 31-bit signed offset stored in the low 31
                     * bits of the field; bit 31 is the cantUnwind flag. */
                    addend = (LONG)(*p << 1) >> 1;
                }
                LONG temp = (LONG)(addend + s - (IPTR)p);

                // Range check for signed 31 bits.
                if (temp < -(1<<30) || temp > ((1<<30)-1)) {
                    kprintf("[ELF Loader] PREL31 overflow\n");
                    return 0;
                }

                *p = (*p & 0x80000000u) | ((ULONG)temp & 0x7FFFFFFFu);
            }
            break;

            case R_ARM_THM_CALL:
            case R_ARM_THM_JUMP24:
            {
                ULONG upper,lower,sign,j1,j2;
                LONG offset;

                bug("Thumb relocations are currently not supported\n");
                SetIoErr(ERROR_BAD_HUNK);
                return 0;

                // The below is written for REL relocations but we use RELA for ARM now.
                /* upper = AROS_WORD2LE(*((UWORD *)p)); */
                /* lower = AROS_WORD2LE(*((UWORD *)p+1)); */

                /* sign = (upper >> 10) & 1; */
                /* j1 = (lower >> 13) & 1; */
                /* j2 = (lower >> 11) & 1; */

                /* offset = (sign << 24) | ((~(j1 ^ sign) & 1) << 23) | */
                /*                 ((~(j2 ^ sign) & 1) << 22) | */
                /*                 ((upper & 0x03ff) << 12) | */
                /*                 ((lower & 0x07ff) << 1); */

                /* if (offset & 0x01000000) */
                /*         offset -= 0x02000000; */

                /* if (offset >= 0x01000000 || */
                /*         offset <= -0x01000000) */
                /* { */
                /*         bug("[ELF Loader] Relocation type %d %d out of range!\n", i, ELF_R_TYPE(rel->info)); */
                /*         SetIoErr(ERROR_BAD_HUNK); */
                /*         return 0; */
                /* } */
                /* offset += s - (ULONG)p; */

                /* sign = (offset >> 24) & 1; */
                /* j1 = sign ^ (~(offset >> 23) & 1); */
                /* j2 = sign ^ (~(offset >> 22) & 1); */

                /* *(UWORD *)p = AROS_WORD2LE((UWORD)((upper & 0xf800) | (sign << 10) | */
                /*                 ((offset >> 12) & 0x03ff))); */
                /* *((UWORD *)p + 1) = AROS_WORD2LE((UWORD)((lower & 0xd000) | */
                /*                 (j1 << 13) | (j2 << 11) | ((offset >> 1) & 0x07ff))); */

            }
            break;

            case R_ARM_THM_MOVW_ABS_NC:
            case R_ARM_THM_MOVT_ABS:
            {
                ULONG upper,lower;
                LONG offset;

                bug("Thumb relocations are currently not supported\n");
                SetIoErr(ERROR_BAD_HUNK);
                return 0;

                // The below is written for REL relocations but we use RELA for ARM now.
                /* upper = AROS_LE2WORD(*((UWORD *)p)); */
                /* lower = AROS_LE2WORD(*((UWORD *)p+1)); */

                /* offset = ((upper & 0x000f) << 12) | */
                /*                 ((upper & 0x0400) << 1) | */
                /*                 ((lower & 0x7000) >> 4) | */
                /*                 (lower & 0x00ff); */

                /* offset = (offset ^ 0x8000) - 0x8000; */

                /* offset += s; */

                /* if (ELF_R_TYPE(rel->info) == R_ARM_THM_MOVT_ABS) */
                /*         offset >>= 16; */

                /* *(UWORD *)p = AROS_WORD2LE((UWORD)((upper & 0xfbf0) | */
                /*                 ((offset & 0xf000) >> 12) | */
                /*                 ((offset & 0x0800) >> 1))); */
                /* *((UWORD *)p + 1) = AROS_WORD2LE((UWORD)((lower & 0x8f00) | */
                /*                 ((offset & 0x0700)<< 4) | */
                /*                 (offset & 0x00ff))); */
            }
            break;

            case R_ARM_MOVW_ABS_NC:
            case R_ARM_MOVT_ABS:
            {
                LONG addend;
                if (is_rela) {
                    addend = (LONG)((struct rela *)rel)->addend;
                } else {
                    /* SHT_REL: 16-bit immediate is encoded split (bits
                     * 19:16 hold imm4, 11:0 hold imm12). For MOVT this
                     * is the upper half of the addend; for MOVW the lower.
                     * Treat the 16-bit value as unsigned when reassembling
                     * the full 32-bit addend so MOVT/MOVW pairs combine
                     * correctly. */
                    ULONG imm16 = ((*p & 0x000F0000u) >> 4) | (*p & 0x00000FFFu);
                    if (ELF_R_TYPE(rel->info) == R_ARM_MOVT_ABS)
                        addend = (LONG)(imm16 << 16);
                    else
                        addend = (LONG)imm16;
                }
                ULONG temp = (ULONG)s + (ULONG)addend;

                // Select the 16-bit half to encode.
                ULONG imm16 = (ELF_R_TYPE(rel->info) == R_ARM_MOVT_ABS)
                    ? ((temp >> 16) & 0xFFFFu)   // upper 16 bits for MOVT
                    : (temp & 0xFFFFu);          // lower 16 bits for MOVW

                // Clear bits 19-16 and 11-0, where the new value will go.
                *p &= 0xFFF0F000u;

                // In with the new.
                *p |= ((imm16 & 0xF000u) << 4) | (imm16 & 0x0FFFu);
            }
            break;

            case R_ARM_TARGET2: /* maps to R_ARM_ABS32 under EABI for AROS*/
            case R_ARM_TARGET1: /* use for constructors/destructors; maps to
                                   R_ARM_ABS32 */
            case R_ARM_ABS32:
                if (is_rela)
                    *p = (ULONG)((struct rela *)rel)->addend + s;
                else
                    *p = *p + s;  /* SHT_REL: addend is in *p */
                break;

            case R_ARM_REL32: /* PC-relative data (S + A - P), e.g. .eh_frame */
                if (is_rela)
                    *p = (ULONG)((struct rela *)rel)->addend + s - (ULONG)p;
                else
                    *p = *p + s - (ULONG)p;  /* SHT_REL: addend is in *p */
                break;

            case R_ARM_NONE:
                break;
            #elif defined(__aarch64__)

            case R_AARCH64_ABS64:
                *(UQUAD *)p = s + rel->addend;
                break;
            case R_AARCH64_ABS32:
                *(ULONG *)p = (ULONG)(s + rel->addend);
                break;
            case R_AARCH64_PREL64:
                *(UQUAD *)p = s + rel->addend - (IPTR)p;
                break;
            case R_AARCH64_PREL32:
                *(ULONG *)p = (ULONG)(s + rel->addend - (IPTR)p);
                break;

            /* movz/movk: replace the 16-bit imm (instruction bits 5-20) */
            case R_AARCH64_MOVW_UABS_G0:
            case R_AARCH64_MOVW_UABS_G0_NC:
                *p = (*p & 0xffe0001fu) | ((((s + rel->addend) >> 0)  & 0xffff) << 5);
                break;
            case R_AARCH64_MOVW_UABS_G1:
            case R_AARCH64_MOVW_UABS_G1_NC:
                *p = (*p & 0xffe0001fu) | ((((s + rel->addend) >> 16) & 0xffff) << 5);
                break;
            case R_AARCH64_MOVW_UABS_G2:
            case R_AARCH64_MOVW_UABS_G2_NC:
                *p = (*p & 0xffe0001fu) | ((((s + rel->addend) >> 32) & 0xffff) << 5);
                break;
            case R_AARCH64_MOVW_UABS_G3:
                *p = (*p & 0xffe0001fu) | ((((s + rel->addend) >> 48) & 0xffff) << 5);
                break;

            /* ADRP: 21-bit page offset, split immlo (bits 29-30) / immhi (5-23) */
            case R_AARCH64_ADR_PREL_PG_HI21:
            {
                IPTR x = (((s + rel->addend) & ~(IPTR)0xfff) - ((IPTR)p & ~(IPTR)0xfff)) >> 12;
                *p = (*p & 0x9f00001fu) | ((x & 0x3) << 29) | (((x >> 2) & 0x7ffff) << 5);
                break;
            }

            /* ADD/LDST imm12 (instruction bits 10-21), LDST scaled by size */
            case R_AARCH64_ADD_ABS_LO12_NC:
            case R_AARCH64_LDST8_ABS_LO12_NC:
                *p = (*p & 0xffc003ffu) | ((((s + rel->addend) & 0xfff) >> 0) << 10);
                break;
            case R_AARCH64_LDST16_ABS_LO12_NC:
                *p = (*p & 0xffc003ffu) | ((((s + rel->addend) & 0xfff) >> 1) << 10);
                break;
            case R_AARCH64_LDST32_ABS_LO12_NC:
                *p = (*p & 0xffc003ffu) | ((((s + rel->addend) & 0xfff) >> 2) << 10);
                break;
            case R_AARCH64_LDST64_ABS_LO12_NC:
                *p = (*p & 0xffc003ffu) | ((((s + rel->addend) & 0xfff) >> 3) << 10);
                break;
            case R_AARCH64_LDST128_ABS_LO12_NC:
                *p = (*p & 0xffc003ffu) | ((((s + rel->addend) & 0xfff) >> 4) << 10);
                break;

            /*
             * b/bl: 26-bit signed branch offset >> 2 (instruction bits 0-25).
             * The reach is +/-128MB; a target outside that range (or a
             * misaligned one) cannot be encoded and must be refused rather
             * than silently truncated.
             */
            case R_AARCH64_JUMP26:
            case R_AARCH64_CALL26:
            {
                SIPTR temp = (SIPTR)(s + rel->addend - (IPTR)p);
                SIPTR imm26;

                if (temp & 3) {
                    D(bug("[ELF Loader] CALL26/JUMP26 target misaligned\n"));
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }

                imm26 = temp >> 2;

                /* signed 26-bit immediate: [-2^25, 2^25 - 1] */
                if (imm26 < -((SIPTR)1 << 25) || imm26 > (((SIPTR)1 << 25) - 1)) {
                    D(bug("[ELF Loader] CALL26/JUMP26 target out of range\n"));
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }

                *p = (*p & 0xfc000000u) | ((ULONG)imm26 & 0x03ffffffu);
                break;
            }

            /* --- relocations needed by GOT-based / large-model code --- */

            /* ADRP without overflow check (companion of ADR_PREL_PG_HI21) */
            case R_AARCH64_ADR_PREL_PG_HI21_NC:
            {
                IPTR x = (((s + rel->addend) & ~(IPTR)0xfff) - ((IPTR)p & ~(IPTR)0xfff)) >> 12;
                *p = (*p & 0x9f00001fu) | ((x & 0x3) << 29) | (((x >> 2) & 0x7ffff) << 5);
                break;
            }

            case R_AARCH64_ADR_PREL_LO21: /* S + A - P, bits [20:0] */
            {
                QUAD val = (QUAD)s + (QUAD)rel->addend - (QUAD)(IPTR)p;
                if (val < -(1LL << 20) || val >= (1LL << 20)) {
                    bug("[ELF Loader] ADR_PREL_LO21 overflow\n");
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                ULONG imm = (ULONG)val & 0x1FFFFFU;
                *p = (*p & 0x9F00001Fu) | ((imm & 0x3u) << 29) | (((imm >> 2) & 0x7FFFFu) << 5);
            }
            break;

            case R_AARCH64_LD_PREL_LO19: /* S + A - P, bits [20:2] */
            {
                QUAD val = (QUAD)s + (QUAD)rel->addend - (QUAD)(IPTR)p;
                /* The literal must be word aligned, the low 2 bits are not
                 * encoded and would be dropped silently. */
                if (val & 3) {
                    bug("[ELF Loader] LD_PREL_LO19 target misaligned\n");
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                if (val < -(1LL << 20) || val >= (1LL << 20)) {
                    bug("[ELF Loader] LD_PREL_LO19 overflow\n");
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                *p = (*p & ~(0x7FFFFu << 5)) | ((((ULONG)(val >> 2)) & 0x7FFFFu) << 5);
            }
            break;

            case R_AARCH64_CONDBR19: /* S + A - P, bits [20:2] */
            {
                QUAD offset = (QUAD)s + (QUAD)rel->addend - (QUAD)(IPTR)p;
                if (offset < -(1LL << 20) || offset >= (1LL << 20)) {
                    bug("[ELF Loader] CONDBR19 overflow\n");
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                *p = (*p & ~(0x7FFFFUL << 5)) | ((((ULONG)(offset >> 2)) & 0x7FFFFUL) << 5);
            }
            break;

            case R_AARCH64_TSTBR14: /* S + A - P, bits [15:2] */
            {
                QUAD offset = (QUAD)s + (QUAD)rel->addend - (QUAD)(IPTR)p;
                if (offset < -(1LL << 15) || offset >= (1LL << 15)) {
                    bug("[ELF Loader] TSTBR14 overflow\n");
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                *p = (*p & ~(0x3FFFu << 5)) | ((((ULONG)(offset >> 2)) & 0x3FFFu) << 5);
            }
            break;

            case R_AARCH64_ADR_GOT_PAGE: /* Page(&GOT[n]) - Page(P), ADRP */
            {
                IPTR *got_entry = aarch64_got_entry(&aarch64_got, s + rel->addend,
                                                    funcarray, next_hunk_ptr, DOSBase);
                if (!got_entry)
                    return 0;   /* IoErr already set */

                /* ADRP page offset to the GOT entry; +/-4GB reach (never
                 * trips on <4GB systems, but keep the check symmetric). */
                QUAD val = (QUAD)((IPTR)got_entry & ~0xFFFULL) - (QUAD)((IPTR)p & ~0xFFFULL);
                if (val < -(1LL << 32) || val >= (1LL << 32)) {
                    bug("[ELF Loader] ADR_GOT_PAGE overflow\n");
                    SetIoErr(ERROR_BAD_HUNK);
                    return 0;
                }
                ULONG imm = (ULONG)(val >> 12) & 0x1FFFFFU;
                *p = (*p & 0x9F00001Fu) | ((imm & 0x3u) << 29) | (((imm >> 2) & 0x7FFFFu) << 5);
            }
            break;

            case R_AARCH64_LD64_GOT_LO12_NC: /* &GOT[n] page offset, LDR scaled by 8 */
            {
                IPTR *got_entry = aarch64_got_entry(&aarch64_got, s + rel->addend,
                                                    funcarray, next_hunk_ptr, DOSBase);
                if (!got_entry)
                    return 0;   /* IoErr already set */

                ULONG imm12 = ((ULONG)(IPTR)got_entry & 0xFFFu) >> 3;
                *p = (*p & ~(0xFFFu << 10)) | (imm12 << 10);
            }
            break;

            case R_AARCH64_NONE:
                break;
            #elif defined(__riscv)

            #else
            #    error Your architecture is not supported
            #endif

            default:
                bug("[ELF Loader] Unknown relocation #%d type %d\n", i, ELF_R_TYPE(rel->info));
                SetIoErr(ERROR_BAD_HUNK);
                return 0;
        }
    }

    return 1;
}

#ifdef __arm__

/*
 * On ARM < v6 all LONG accesses must be LONG-aligned
 * TODO: This is useful and can be moved to some public include file.
 */
#if (__ARM_ARCH__ > 5)

#define READLONG_UNALIGNED(src) src

#else

static inline ULONG readlong_unaligned(ULONG *src)
{
    ULONG res, tmp;

    asm volatile(
        "ldrb   %0, [%2, #0]\n\t"
        "ldrb   %1, [%2, #1]\n\t"
        "orr    %0, %0, %1, lsl #8\n\t"
        "ldrb   %1, [%2, #2]\n\t"
        "orr    %0, %0, %1, lsl #16\n\t"
        "ldrb   %1, [%2, #3]\n\t"
        "orr    %0, %0, %1, lsl #24"
        :"=&r"(res), "=&r"(tmp) : "r"(src)
    );

    return res;
}

#define READLONG_UNALIGNED(src) readlong_unaligned(&src);
#endif

/*
 * This code parses special .ARM.Attributes section and
 * extracts system requirements from it. Things like float ABI,
 * minimum CPU and FPU version are described there.
 */

static UBYTE arm_cpus[] =
{
    CPUFAMILY_ARM_3,
    CPUFAMILY_ARM_4,
    CPUFAMILY_ARM_4T,
    CPUFAMILY_ARM_5T,
    CPUFAMILY_ARM_5TE,
    CPUFAMILY_ARM_5TEJ,
    CPUFAMILY_ARM_6,
    CPUFAMILY_ARM_6,
    CPUFAMILY_ARM_6, /* 6KZ  */
    CPUFAMILY_ARM_6, /* 6T2  */
    CPUFAMILY_ARM_6, /* 6K   */
    CPUFAMILY_ARM_7,
    CPUFAMILY_ARM_6, /* 6-M  */
    CPUFAMILY_ARM_6, /* 6S-M */
    CPUFAMILY_ARM_7  /* 7E-M */
};

static BOOL ARM_ParseAttrs(UBYTE *data, ULONG len, struct DosLibrary *DOSBase)
{
    struct attrs_section *attrs;

    if (data[0] != ATTR_VERSION_CURRENT)
    {
        DATTR(bug("[ELF.ARM] Unknown attributes version: 0x%02\n", data[0]));
        return FALSE;
    }

    attrs = (void *)data + 1;
    while (len > 0)
    {
        ULONG attrs_size = READLONG_UNALIGNED(attrs->size);

        if (strcmp(attrs->vendor, "aeabi") == 0)
        {
            struct attrs_subsection *aeabi_attrs = (void *)attrs->vendor + 6;
            ULONG aeabi_len = attrs_size - 10;

            DATTR(bug("[ELF.ARM] Found aeabi attributes @ 0x%p (length %u)\n", aeabi_attrs, aeabi_len));

            while (aeabi_len > 0)
            {
                ULONG aeabi_attrs_size = READLONG_UNALIGNED(aeabi_attrs->size);

                if (aeabi_attrs->tag == Tag_File)
                {
                    UBYTE *file_subsection = (void *)aeabi_attrs + sizeof(struct attrs_subsection);
                    UBYTE file_len = aeabi_attrs_size - sizeof(struct attrs_subsection);

                    DATTR(bug("[ELF.ARM] Found file-wide attributes @ 0x%p (length %u)\n", file_subsection, file_len));

                    while (file_len > 0)
                    {
                        UBYTE tag, shift;
                        ULONG val = 0;

                        tag = *file_subsection++;
                        file_len--;

                        if (file_len == 0)
                        {
                            DATTR(bug("[ELF.ARM] Mailformed attribute tag %d (no data)\n", tag));
                            return FALSE;
                        }

                        switch (tag)
                        {
                        case Tag_CPU_raw_name:
                        case Tag_CPU_name:
                        case Tag_compatibility:
                        case Tag_also_compatible_with:
                        case Tag_conformance:
                            /* These two are NULL-terminated strings. Just skip. */
                            while (file_len)
                            {
                                file_len--;
                                if (*file_subsection++ == 0)
                                    break;
                            }
                            break;

                        default:
                            /* Read ULEB128 value */
                            shift = 0;
                            while (file_len)
                            {
                                UBYTE byte;

                                byte = *file_subsection++;
                                file_len--;

                                val |= (byte & 0x7F) << shift;
                                if (!(byte & 0x80))
                                    break;

                                shift += 7;
                            }
                        }

                        switch (tag)
                        {
                        case Tag_CPU_arch:
                            DATTR(bug("[ELF.ARM] ARM CPU architecture set to %d\n", val));

                            if (val > ELF_CPU_ARMv7EM)
                            {
                                DATTR(bug("[ELF.ARM] Unknown CPU tag value (%d)\n", val));
                                return FALSE;
                            }

                            if (arm_cpus[val] > IDosBase(DOSBase)->arm_Arch)
                            {
                                DATTR(bug("[ELF.ARM] CPU Requirements too high (system %d, file %d)\n", IDosBase(DOSBase)->arm_Arch, arm_cpus[val]));
                                return FALSE;
                            }
                            break;

                        case Tag_FP_arch:
                            DATTR(bug("[ELF.ARM] ARM FPU architecture set to %d\n", val));

                            switch (val)
                            {
                            case ELF_FP_None:
                                 break;

                            case ELF_FP_v1:
                            case ELF_FP_v2:
                                if (!IDosBase(DOSBase)->arm_VFP)
                                {
                                    DATTR(bug("[ELF.ARM] VFP required but missing\n"));
                                    return FALSE;
                                }
                                break;

                            case ELF_FP_v3:
                            case ELF_FP_v3_Short:
                                if (!IDosBase(DOSBase)->arm_VFP_v3)
                                {
                                    DATTR(bug("[ELF.ARM] VFPv3 required but missing\n"));
                                    return FALSE;
                                }
                                break;

                            case ELF_FP_v4:
                            case ELF_FP_v4_Short:
                                /* VFPv4 is a superset of VFPv3; accept on any VFPv3-capable CPU */
                                if (!IDosBase(DOSBase)->arm_VFP_v3)
                                {
                                    DATTR(bug("[ELF.ARM] VFPv4 required but missing\n"));
                                    return FALSE;
                                }
                                break;

                            default:
                                DATTR(bug("[ELF.ARM] VFP %d required -- unsupported\n", val));
                                return FALSE;
                            }

                            break;
                        }
                    }

                    /* We allow to execute only files which contain attributes section */
                    return TRUE;
                }
                aeabi_attrs = (void *)aeabi_attrs + aeabi_attrs_size;
                aeabi_len -= aeabi_attrs_size;
            }

            return FALSE;
        }
        attrs = (void *)attrs + attrs_size;
        len -= attrs_size;
    }
    return FALSE;
}

#endif

static BPTR load_seg_elf_int
(
    BPTR               file,
    SIPTR             *funcarray,
    BOOL               force31,
    BOOL              *reloc_out_of_range,
    struct DosLibrary *DOSBase
)
{
    struct elfheader  eh;
    struct sheader   *sh;
    struct sheader   *symtab_shndx = NULL;
    struct sheader   *strtab = NULL;
    BPTR   hunks         = 0;
#if defined(DOCACHECLEAR)
    BPTR curr;
#endif
    BPTR  *next_hunk_ptr = &hunks;
    ULONG  i;
    BOOL   exec_hunk_seen = FALSE;
    ULONG  int_shnum;
    struct SRBuffer srb = { 0 };
    UBYTE *arena = NULL;
    UBYTE *arena_cursor = NULL;
    IPTR   arena_size = 0;
    ULONG  arena_flags = 0;

    /* load and validate ELF header */
    if (!load_header(file, &eh, funcarray, &srb, DOSBase))
        return 0;

    int_shnum = read_shnum(file, &eh, funcarray, &srb, DOSBase);
    if (!int_shnum)
        return 0;

    /* load section headers */
    if (!(sh = load_block(file, eh.shoff, int_shnum * eh.shentsize, funcarray, &srb, DOSBase)))
        return 0;

#ifdef __arm__
    for (i = 0; i < int_shnum; i++)
    {
        if (sh[i].type == SHT_ARM_ATTRIBUTES)
        {
            ULONG len = sh[i].size;
            UBYTE *data = load_block(file, sh[i].offset, len, funcarray, &srb, DOSBase);

            if (data)
            {
                BOOL res = ARM_ParseAttrs(data, len, DOSBase);

                ilsFreeMem(data, len);

                if (!res)
                {
                    D(bug("[ELF Loader] Can't parse ARM attributes\n"));
                    SetIoErr(ERROR_NOT_EXECUTABLE);
                    goto error;
                }
            }
        }
    }
#endif

    /* Iterate over the section headers in order to do some stuff... */
    for (i = 0; i < int_shnum; i++)
    {
        /*
           Load the symbol and string table(s).

           NOTICE: the ELF standard, at the moment (Nov 2002) explicitely states
                   that only one symbol table per file is allowed. However, it
                   also states that this may change in future... we already handle it.
        */
        if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB || sh[i].type == SHT_SYMTAB_SHNDX)
        {
            sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, &srb, DOSBase);
            if (!sh[i].addr)
                goto error;

            if (sh[i].type == SHT_STRTAB && i == eh.shstrndx) {
                if (strtab == NULL) {
                    strtab = &sh[i];
                } else {
                    D(bug("[ELF Loader] file contains multiple strtab tables. only using the first one\n"));
                }
            }

            if (sh[i].type == SHT_SYMTAB_SHNDX) {
                if (symtab_shndx == NULL)
                    symtab_shndx = &sh[i];
                else
                    D(bug("[ELF Loader] file contains multiple symtab shndx tables. only using the first one\n"));
            }
        }
   }

    /* Now that we have the string and symbol tables loaded,
     * load the rest of the hunks.
     */
    BOOL do_align;

#if defined(__x86_64__)
    /*
     * Modules contain 32-bit PC-relative references between their hunks,
     * which cannot span more than +/-2GB. Rather than restricting every
     * hunk to 31-bit memory, allocate one arena for the whole module and
     * carve the hunks from it: proximity is then guaranteed by
     * construction, and the module can live anywhere in RAM. A container
     * hunk owning the arena is linked at the end of the seg list; the
     * carved hunks store size 0, which ilsFreeVec() skips on unload.
     *
     * If the module additionally contains R_X86_64_32/32S absolute
     * relocations (which can only encode addresses below 2GB) and the
     * arena was placed higher, the relocation pass fails with
     * reloc_out_of_range set and the caller retries with force31.
     */
    for (i = 0; i < int_shnum; i++)
    {
        if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB || sh[i].type == SHT_SYMTAB_SHNDX)
            continue;

        if ((sh[i].flags & SHF_ALLOC) && sh[i].size)
        {
            if (sh[i].flags & SHF_EXECINSTR)
            {
                exec_hunk_seen = TRUE;
                arena_flags |= MEMF_EXECUTABLE;
            }

            do_align = (exec_hunk_seen && sh[i].addralign >= 2) ? TRUE : FALSE;
            arena_size += AROS_ROUNDUP2(elf_hunk_size(&sh[i], do_align), AROS_WORSTALIGN) + AROS_WORSTALIGN;
        }
    }
    exec_hunk_seen = FALSE;

    if (arena_size)
    {
        arena_size += sizeof(struct hunk) + AROS_WORSTALIGN;

        arena = ilsAllocMem(arena_size,
            arena_flags | MEMF_PUBLIC | MEMF_CLEAR | (force31 ? MEMF_31BIT : 0));

        if (arena && TypeOfMem(arena) == 0)
        {
            /*
             * The allocation was served by KrnAllocPages() (W^X host):
             * pages are protection-flipped per hunk there, so mixing code
             * and data in one arena is not possible. Free it and fall back
             * to per-hunk allocations below.
             */
            ilsFreeMem(arena, arena_size);
            arena = NULL;
        }

        if (arena)
        {
            /* Leave room for the container hunk at the start */
            arena_cursor = arena + sizeof(struct hunk);
        }
    }
#endif

    for (i = 0; i < int_shnum; i++)
    {
        /* Skip the already loaded hunks */
        if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB || sh[i].type == SHT_SYMTAB_SHNDX)
            continue;

        /* Load the section in memory if needed, and make a hunk out of it */
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

                do_align = (exec_hunk_seen && sh[i].addralign >= 2) ? TRUE : FALSE;
                if (!load_hunk(file, &next_hunk_ptr, &sh[i], strtab ? strtab->addr : NULL, funcarray, do_align,
                               arena ? &arena_cursor : NULL,
                               force31 ? MEMF_31BIT : 0, &srb, DOSBase))
                    goto error;
            }
        }
    }

#if defined(__x86_64__)
    if (arena && hunks)
    {
        /* Link the arena's container hunk as the last segment; freeing it
           on unload releases the whole module. */
        struct hunk *cont = (struct hunk *)arena;

        cont->size = arena_size;
        cont->next = 0;
        /* next_hunk_ptr points at the last hunk's next field */
        *next_hunk_ptr = HUNK2BPTR(cont);
        next_hunk_ptr = (BPTR *)((IPTR)cont + offsetof(struct hunk, next));
        arena = NULL; /* now owned by the seg list */
    }
    else if (arena)
    {
        /* No hunks were made; release the arena directly */
        ilsFreeMem(arena, arena_size);
        arena = NULL;
    }
#endif

    /* Relocate the sections */
    for (i = 0; i < int_shnum; i++)
    {
        /* Does this relocation section refer to a hunk? If so, addr must be != 0.
         * Accept both SHT_REL and SHT_RELA: the actual section type produced by
         * a given toolchain is not fixed (e.g. clang/lld emits SHT_REL for ARM
         * 32-bit while some gcc builds emit SHT_RELA).
         */
        if ((sh[i].type == SHT_REL || sh[i].type == SHT_RELA) && sh[sh[i].info].addr)
        {
            sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, &srb, DOSBase);
            if (!sh[i].addr || !relocate(&eh, sh, i, symtab_shndx, reloc_out_of_range, funcarray, &next_hunk_ptr, DOSBase))
                goto error;

            ilsFreeMem(sh[i].addr, sh[i].size);
            sh[i].addr = NULL;
        }
    }

    register_elf(file, hunks, &eh, sh, DOSBase);
    goto end;

error:

    /* There were some errors, deallocate The hunks */

#if defined(__x86_64__)
    /* If the arena has not been handed over to the seg list yet, free it
       here; the carved hunks inside it are skipped by the unload below. */
    if (arena)
    {
        ilsFreeMem(arena, arena_size);
        arena = NULL;
    }
#endif

    InternalUnLoadSeg(hunks, (VOID_FUNC)funcarray[2]);
    hunks = 0;

end:

#if defined(DOCACHECLEAR)
    /* Clear the caches to let the CPU see the new data and instructions. */
    curr = hunks;

    while (curr)
    {
        struct hunk *hunk = BPTR2HUNK(BADDR(curr));

        ils_ClearCache(hunk->data, hunk->size, CACRF_ClearD | CACRF_ClearI);
        curr = hunk->next;
    }
#endif

    /* deallocate the symbol tables */
    for (i = 0; i < int_shnum; i++)
    {
        if (((sh[i].type == SHT_SYMTAB) || (sh[i].type == SHT_STRTAB)) && (sh[i].addr != NULL))
            ilsFreeMem(sh[i].addr, sh[i].size);
    }

    /* Free the section headers */
    ilsFreeMem(sh, int_shnum * eh.shentsize);

    if (srb.srb_Buffer) FreeMem(srb.srb_Buffer, LOADSEG_SMALL_READ);

    return hunks;
}

BPTR InternalLoadSeg_ELF
(
    BPTR               file,
    BPTR               table __unused,
    SIPTR             *funcarray,
    LONG              *stack __unused,
    struct DosLibrary *DOSBase
)
{
    BOOL reloc_out_of_range = FALSE;
    BPTR seg;

    seg = load_seg_elf_int(file, funcarray, FALSE, &reloc_out_of_range, DOSBase);

#if defined(__x86_64__)
    if (!seg && reloc_out_of_range)
    {
        /*
         * The module contains absolute 32-bit relocations and its arena was
         * placed too high in memory: retry with the arena in 31-bit RAM.
         */
        D(bug("[ELF Loader] retrying load in 31-bit memory\n"));
        seg = load_seg_elf_int(file, funcarray, TRUE, &reloc_out_of_range, DOSBase);
    }
#endif

    return seg;
}
