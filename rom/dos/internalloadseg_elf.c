/*
    Copyright (C) 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code to dynamically load ELF executables
    Lang: english
*/

#define DATTR(x)

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <dos/elf.h>
#include <dos/dosasl.h>
#include <libraries/debug.h>
#include <resources/processor.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <proto/debug.h>
#include <proto/exec.h>

#include <string.h>
#include <stddef.h>

#include "internalloadseg.h"
#include "dos_intern.h"
#include "include/loadseg.h"

struct hunk
{
    ULONG size;
    BPTR  next;
    char  data[0];
} __attribute__((packed));

#define BPTR2HUNK(bptr) ((struct hunk *)((void *)bptr - offsetof(struct hunk, next)))
#define HUNK2BPTR(hunk) MKBADDR(&hunk->next)

#define LOADSEG_SMALL_READ  2048 /* Size adjusted by profiling in Callgrind */

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

static int __attribute__ ((noinline)) load_hunk
(
    BPTR                 file,
    BPTR               **next_hunk_ptr,
    struct sheader      *sh,
    CONST_STRPTR         strtab,
    SIPTR               *funcarray,
    BOOL                 do_align,
    struct SRBuffer     *srb,
    struct DosLibrary   *DOSBase
)
{
    struct hunk *hunk;
    ULONG  hunk_size;
    ULONG  memflags = 0;

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

    hunk = ilsAllocMem(hunk_size, memflags | MEMF_PUBLIC | (sh->type == SHT_NOBITS ? MEMF_CLEAR : 0));
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
        *next_hunk_ptr = &hunk->next;

        if (sh->type != SHT_NOBITS)
            return !elf_read_block(file, sh->offset, sh->addr, sh->size, funcarray, srb, DOSBase);

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
    struct sheader *shsymtab = &sh[shrel->link];
    struct sheader *toreloc  = &sh[shrel->info];

    struct symbol *symtab   = (struct symbol *)shsymtab->addr;
    struct relo   *rel      = (struct relo *)shrel->addr;

    /*
     * Ignore relocs if the target section has no allocation. that can happen
     * eg. with a .debug PROGBITS and a .rel.debug section
     */
    if (!(toreloc->flags & SHF_ALLOC))
        return 1;

    ULONG numrel = shrel->size / shrel->entsize;
    ULONG i;

    for (i=0; i<numrel; i++, rel++)
    {
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

        DB2(bug("[ELF Loader] Processing symbol %s\n", sh[shsymtab->link].addr + sym->name));

        switch (shindex)
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

            case R_386_NONE:
                break;

            #elif defined(__x86_64__)
            case R_X86_64_64: /* 64bit direct/absolute */
                *(UQUAD *)p = s + rel->addend;
                break;

            case R_X86_64_PLT32:
            case R_X86_64_PC32: /* PC relative 32 bit signed */
                *(ULONG *)p = s + rel->addend - (IPTR) p;
                break;

            case R_X86_64_32:
                *(ULONG *)p = (UQUAD)s + (UQUAD)rel->addend;
                break;

            case R_X86_64_32S:
                *(LONG *)p = (QUAD)s + (QUAD)rel->addend;
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
            case R_ARM_PREL31:
            {
                /* On ARM the 24 bit offset is shifted by 2 to the right */
                signed long offset = (AROS_LE2LONG(*p) & 0x00ffffff) << 2;
                /* If highest bit set, make offset negative */
                if (offset & 0x02000000)
                    offset -= 0x04000000;

                if (offset >= 0x02000000 ||
                        offset <= -0x02000000)
                {
                        bug("[ELF Loader] Relocation type %d %d out of range!\n", i, ELF_R_TYPE(rel->info));
                        SetIoErr(ERROR_BAD_HUNK);
                        return 0;
                }
                offset += s - (ULONG)p;

                offset >>= 2;
                *p &= AROS_LONG2LE(0xff000000);
                *p |= AROS_LONG2LE(offset & 0x00ffffff);
            }
            break;

            case R_ARM_THM_CALL:
            case R_ARM_THM_JUMP24:
            {
                ULONG upper,lower,sign,j1,j2;
                LONG offset;

                upper = AROS_WORD2LE(*((UWORD *)p));
                lower = AROS_WORD2LE(*((UWORD *)p+1));

                sign = (upper >> 10) & 1;
                j1 = (lower >> 13) & 1;
                j2 = (lower >> 11) & 1;

                offset = (sign << 24) | ((~(j1 ^ sign) & 1) << 23) |
                                ((~(j2 ^ sign) & 1) << 22) |
                                ((upper & 0x03ff) << 12) |
                                ((lower & 0x07ff) << 1);

                if (offset & 0x01000000)
                        offset -= 0x02000000;

                if (offset >= 0x01000000 ||
                        offset <= -0x01000000)
                {
                        bug("[ELF Loader] Relocation type %d %d out of range!\n", i, ELF_R_TYPE(rel->info));
                        SetIoErr(ERROR_BAD_HUNK);
                        return 0;
                }
                offset += s - (ULONG)p;

                sign = (offset >> 24) & 1;
                j1 = sign ^ (~(offset >> 23) & 1);
                j2 = sign ^ (~(offset >> 22) & 1);

                *(UWORD *)p = AROS_WORD2LE((UWORD)((upper & 0xf800) | (sign << 10) |
                                ((offset >> 12) & 0x03ff)));
                *((UWORD *)p + 1) = AROS_WORD2LE((UWORD)((lower & 0xd000) |
                                (j1 << 13) | (j2 << 11) | ((offset >> 1) & 0x07ff)));

            }
            break;

            case R_ARM_THM_MOVW_ABS_NC:
            case R_ARM_THM_MOVT_ABS:
            {
                ULONG upper,lower;
                LONG offset;

                upper = AROS_LE2WORD(*((UWORD *)p));
                lower = AROS_LE2WORD(*((UWORD *)p+1));

                offset = ((upper & 0x000f) << 12) |
                                ((upper & 0x0400) << 1) |
                                ((lower & 0x7000) >> 4) |
                                (lower & 0x00ff);

                offset = (offset ^ 0x8000) - 0x8000;

                offset += s;

                if (ELF_R_TYPE(rel->info) == R_ARM_THM_MOVT_ABS)
                        offset >>= 16;

                *(UWORD *)p = AROS_WORD2LE((UWORD)((upper & 0xfbf0) |
                                ((offset & 0xf000) >> 12) |
                                ((offset & 0x0800) >> 1)));
                *((UWORD *)p + 1) = AROS_WORD2LE((UWORD)((lower & 0x8f00) |
                                ((offset & 0x0700)<< 4) |
                                (offset & 0x00ff)));
            }
            break;

            case R_ARM_MOVW_ABS_NC:
            case R_ARM_MOVT_ABS:
            {
                signed long offset = AROS_LE2LONG(*p);
                offset = ((offset & 0xf0000) >> 4) | (offset & 0xfff);
                offset = (offset ^ 0x8000) - 0x8000;

                offset += s;

                if (ELF_R_TYPE(rel->info) == R_ARM_MOVT_ABS)
                    offset >>= 16;

                *p &= AROS_LONG2LE(0xfff0f000);
                *p |= AROS_LONG2LE(((offset & 0xf000) << 4) | (offset & 0x0fff));
            }
            break;

            case R_ARM_TARGET2: /* maps to R_ARM_ABS32 under EABI for AROS*/
            case R_ARM_TARGET1: /* use for constructors/destructors; maps to
                                   R_ARM_ABS32 */
            case R_ARM_ABS32:
                *p += s;
                break;

            case R_ARM_NONE:
                break;

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

        if (!strcmp(attrs->vendor, "aeabi"))
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

                            default:
                                /* This includes VFPv4 for now */
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

BPTR InternalLoadSeg_ELF
(
    BPTR               file,
    BPTR               table __unused,
    SIPTR             *funcarray,
    LONG              *stack __unused,
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

                if (!load_hunk(file, &next_hunk_ptr, &sh[i], strtab ? strtab->addr : NULL, funcarray, exec_hunk_seen, &srb, DOSBase))
                    goto error;
            }
        }
    }

    /* Relocate the sections */
    for (i = 0; i < int_shnum; i++)
    {
        /* Does this relocation section refer to a hunk? If so, addr must be != 0 */
        if ((sh[i].type == AROS_ELF_REL) && sh[sh[i].info].addr)
        {
            sh[i].addr = load_block(file, sh[i].offset, sh[i].size, funcarray, &srb, DOSBase);
            if (!sh[i].addr || !relocate(&eh, sh, i, symtab_shndx, DOSBase))
                goto error;

            ilsFreeMem(sh[i].addr, sh[i].size);
            sh[i].addr = NULL;
        }
    }

    /* Everything is loaded now. Register the module at kernel.resource */
    register_elf(file, hunks, &eh, sh, DOSBase);
    goto end;

error:

    /* There were some errors, deallocate The hunks */

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
