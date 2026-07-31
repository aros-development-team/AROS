/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Relocating ELF64 loader for boot-time modules.

    AROS modules are relocatable ELF objects (collect-aros output), so
    the boot loader has to place their sections and apply relocations
    before the resident scan can find their romtags. This is the
    riscv64 counterpart of the bootstrap ELF loaders used by the
    hosted/PC ports.
*/

#include <inttypes.h>

#include <exec/types.h>
#include <dos/elf.h>

#include "kernel_intern.h"

#define D(x)

/* Bump allocator over the free RAM the loader was given */
static IPTR alloc_ptr, alloc_end;
static IPTR loaded_lo, loaded_hi;

static void *krnBumpAlloc(IPTR size, IPTR align)
{
    IPTR addr = (alloc_ptr + align - 1) & ~(align - 1);

    if (align < 8)
        align = 8;
    if (addr + size > alloc_end)
        return NULL;

    alloc_ptr = addr + size;
    if (addr < loaded_lo)
        loaded_lo = addr;
    if (alloc_ptr > loaded_hi)
        loaded_hi = alloc_ptr;

    return (void *)addr;
}

static inline struct sheader *shdr(struct elfheader *eh, unsigned int n)
{
    return (struct sheader *)((UBYTE *)eh + eh->shoff + n * eh->shentsize);
}

/*
 * Apply one RELA relocation. 'loc' is the address being patched, 'val'
 * the resolved symbol value + addend. RISC-V splits wide references
 * across instruction pairs, so several types only patch a field.
 */
static int krnRelocOne(ULONG type, UBYTE *loc, IPTR val, IPTR place)
{
    switch (type)
    {
    case R_RISCV_NONE:
    case R_RISCV_RELAX:     /* relaxation hint - we do not relax */
    case R_RISCV_ALIGN:     /* alignment NOPs are already in place */
        break;

    case R_RISCV_64:
        *(UQUAD *)loc = val;
        break;
    case R_RISCV_32:
        *(ULONG *)loc = (ULONG)val;
        break;
    case R_RISCV_32_PCREL:
        *(ULONG *)loc = (ULONG)(val - place);
        break;

    /* Label arithmetic (used by debug/exception tables) */
    case R_RISCV_ADD8:  *(UBYTE *)loc += (UBYTE)val;  break;
    case R_RISCV_ADD16: *(UWORD *)loc += (UWORD)val;  break;
    case R_RISCV_ADD32: *(ULONG *)loc += (ULONG)val;  break;
    case R_RISCV_ADD64: *(UQUAD *)loc += (UQUAD)val;  break;
    case R_RISCV_SUB8:  *(UBYTE *)loc -= (UBYTE)val;  break;
    case R_RISCV_SUB16: *(UWORD *)loc -= (UWORD)val;  break;
    case R_RISCV_SUB32: *(ULONG *)loc -= (ULONG)val;  break;
    case R_RISCV_SUB64: *(UQUAD *)loc -= (UQUAD)val;  break;
    case R_RISCV_SET6:
        *(UBYTE *)loc = (*(UBYTE *)loc & 0xC0) | (val & 0x3F);
        break;
    case R_RISCV_SUB6:
        *(UBYTE *)loc = (*(UBYTE *)loc & 0xC0) |
                        (((*(UBYTE *)loc & 0x3F) - val) & 0x3F);
        break;
    case R_RISCV_SET8:  *(UBYTE *)loc = (UBYTE)val;   break;
    case R_RISCV_SET16: *(UWORD *)loc = (UWORD)val;   break;
    case R_RISCV_SET32: *(ULONG *)loc = (ULONG)val;   break;

    case R_RISCV_BRANCH:
    {
        SIPTR off = val - place;
        ULONG insn = *(ULONG *)loc & 0x01FFF07F;
        insn |= ((off & 0x1000) << 19) | ((off & 0x07E0) << 20) |
                ((off & 0x001E) << 7)  | ((off & 0x0800) >> 4);
        *(ULONG *)loc = insn;
        break;
    }

    case R_RISCV_JAL:
    {
        SIPTR off = val - place;
        ULONG insn = *(ULONG *)loc & 0x00000FFF;
        insn |= ((off & 0x100000) << 11) | ((off & 0x0007FE) << 20) |
                ((off & 0x000800) << 9)  | (off & 0x0FF000);
        *(ULONG *)loc = insn;
        break;
    }

    case R_RISCV_CALL:
    case R_RISCV_CALL_PLT:
    {
        SIPTR off = val - place;
        ULONG hi = (off + 0x800) & 0xFFFFF000;
        ULONG lo = (off - hi) & 0xFFF;

        /* auipc + jalr pair */
        *(ULONG *)loc = (*(ULONG *)loc & 0x00000FFF) | hi;
        *(ULONG *)(loc + 4) = (*(ULONG *)(loc + 4) & 0x000FFFFF) | (lo << 20);
        break;
    }

    case R_RISCV_PCREL_HI20:
    {
        SIPTR off = val - place;
        ULONG hi = (off + 0x800) & 0xFFFFF000;

        *(ULONG *)loc = (*(ULONG *)loc & 0x00000FFF) | hi;
        break;
    }

    case R_RISCV_HI20:
        *(ULONG *)loc = (*(ULONG *)loc & 0x00000FFF) |
                        ((val + 0x800) & 0xFFFFF000);
        break;
    case R_RISCV_LO12_I:
        *(ULONG *)loc = (*(ULONG *)loc & 0x000FFFFF) | ((val & 0xFFF) << 20);
        break;
    case R_RISCV_LO12_S:
    {
        ULONG v = val & 0xFFF;
        *(ULONG *)loc = (*(ULONG *)loc & 0x01FFF07F) |
                        ((v & 0xFE0) << 20) | ((v & 0x1F) << 7);
        break;
    }

    case R_RISCV_RVC_BRANCH:
    {
        SIPTR off = val - place;
        UWORD insn = *(UWORD *)loc & 0xE383;
        insn |= ((off & 0x100) << 4) | ((off & 0x018) << 7) |
                ((off & 0x0C0) >> 1) | ((off & 0x006) << 2) |
                ((off & 0x020) >> 3);
        *(UWORD *)loc = insn;
        break;
    }

    case R_RISCV_RVC_JUMP:
    {
        SIPTR off = val - place;
        UWORD insn = *(UWORD *)loc & 0xE003;
        insn |= ((off & 0x800) << 1) | ((off & 0x010) << 7) |
                ((off & 0x300) << 1) | ((off & 0x400) >> 2) |
                ((off & 0x040) << 1) | ((off & 0x080) >> 1) |
                ((off & 0x00E) << 2) | ((off & 0x020) >> 3);
        *(UWORD *)loc = insn;
        break;
    }

    default:
        return 0;
    }
    return 1;
}

/*
 * Load one relocatable ELF module. Returns TRUE on success; the loaded
 * range is accumulated into loaded_lo/loaded_hi for the romtag scan.
 */
static int krnLoadModule(void *addr, const char *name)
{
    struct elfheader *eh = addr;
    struct sheader *sh;
    unsigned int i, j;

    if (eh->ident[0] != 0x7F || eh->ident[1] != 'E' ||
        eh->ident[2] != 'L'  || eh->ident[3] != 'F')
    {
        krnSBIPutStr("[elf] not an ELF file: ");
        krnSBIPutStr(name);
        krnSBIPutStr("\n");
        return 0;
    }
    if (eh->machine != EM_RISCV)
    {
        krnSBIPutStr("[elf] wrong architecture: ");
        krnSBIPutStr(name);
        krnSBIPutStr("\n");
        return 0;
    }

    /* Place the allocatable sections */
    for (i = 0; i < eh->shnum; i++)
    {
        sh = shdr(eh, i);

        if (!(sh->flags & SHF_ALLOC) || !sh->size)
            continue;

        if (sh->type == SHT_NOBITS)
        {
            UBYTE *p = krnBumpAlloc(sh->size, sh->addralign);
            IPTR n;

            if (!p)
                return 0;
            for (n = 0; n < sh->size; n++)
                p[n] = 0;
            sh->addr = (elf_ptr_t)(IPTR)p;
        }
        else
        {
            UBYTE *p = krnBumpAlloc(sh->size, sh->addralign);
            UBYTE *src = (UBYTE *)eh + sh->offset;
            IPTR n;

            if (!p)
                return 0;
            for (n = 0; n < sh->size; n++)
                p[n] = src[n];
            sh->addr = (elf_ptr_t)(IPTR)p;
        }
    }

    /* Apply the relocations */
    for (i = 0; i < eh->shnum; i++)
    {
        struct sheader *shrel = shdr(eh, i);
        struct sheader *shtarget, *shsym;
        struct symbol *symtab;
        const char *strtab;
        struct rela *rel;
        IPTR count;

        if (shrel->type != SHT_RELA)
            continue;

        shtarget = shdr(eh, shrel->info);
        if (!(shtarget->flags & SHF_ALLOC) || !shtarget->addr)
            continue;

        shsym  = shdr(eh, shrel->link);
        symtab = (struct symbol *)((UBYTE *)eh + shsym->offset);
        strtab = (const char *)eh + shdr(eh, shsym->link)->offset;

        rel   = (struct rela *)((UBYTE *)eh + shrel->offset);
        count = shrel->size / sizeof(struct rela);

        for (j = 0; j < count; j++, rel++)
        {
            ULONG type = ELF_R_TYPE(rel->info);
            ULONG symi = ELF_R_SYM(rel->info);
            struct symbol *sym = &symtab[symi];
            UBYTE *loc = (UBYTE *)(IPTR)shtarget->addr + rel->offset;
            IPTR place = (IPTR)loc;
            IPTR val;

            /*
             * PCREL_LO12 references the *address of its HI20
             * instruction*, not the target; resolve through it.
             */
            if (type == R_RISCV_PCREL_LO12_I || type == R_RISCV_PCREL_LO12_S)
            {
                IPTR hiaddr = (IPTR)(IPTR)shtarget->addr + sym->value;
                struct rela *r2 = (struct rela *)((UBYTE *)eh + shrel->offset);
                IPTR k, hival = 0;
                int found = 0;

                for (k = 0; k < count; k++, r2++)
                {
                    ULONG t2 = ELF_R_TYPE(r2->info);

                    if ((t2 != R_RISCV_PCREL_HI20 && t2 != R_RISCV_GOT_HI20) ||
                        ((IPTR)shtarget->addr + r2->offset) != hiaddr)
                        continue;
                    {
                        struct symbol *s2 = &symtab[ELF_R_SYM(r2->info)];
                        IPTR base = (s2->shindex == SHN_ABS) ? 0 :
                                    (IPTR)shdr(eh, s2->shindex)->addr;
                        hival = base + s2->value + r2->addend;
                        found = 1;
                    }
                    break;
                }
                if (!found)
                {
                    krnSBIPutStr("[elf] orphan PCREL_LO12 in ");
                    krnSBIPutStr(name);
                    krnSBIPutStr("\n");
                    return 0;
                }
                /* value is relative to the HI20's own place */
                val = hival - hiaddr;
                {
                    SIPTR hi = (val + 0x800) & ~0xFFFL;
                    val = val - hi;     /* the low 12 bits the pair needs */
                }
                if (type == R_RISCV_PCREL_LO12_I)
                    *(ULONG *)loc = (*(ULONG *)loc & 0x000FFFFF) |
                                    ((val & 0xFFF) << 20);
                else
                {
                    ULONG v = val & 0xFFF;
                    *(ULONG *)loc = (*(ULONG *)loc & 0x01FFF07F) |
                                    ((v & 0xFE0) << 20) | ((v & 0x1F) << 7);
                }
                continue;
            }

            /*
             * Symbol index 0 means "no symbol" - R_RISCV_ALIGN and
             * R_RISCV_RELAX are emitted that way and carry only an
             * addend.
             */
            if (symi == 0)
                val = rel->addend;
            else if (sym->shindex == SHN_UNDEF)
            {
                krnSBIPutStr("[elf] undefined symbol '");
                krnSBIPutStr(strtab + sym->name);
                krnSBIPutStr("' in ");
                krnSBIPutStr(name);
                krnSBIPutStr("\n");
                return 0;
            }
            else if (sym->shindex == SHN_ABS)
                val = sym->value + rel->addend;
            else
                val = (IPTR)shdr(eh, sym->shindex)->addr + sym->value +
                      rel->addend;

            if (!krnRelocOne(type, loc, val, place))
            {
                krnSBIPutStr("[elf] unsupported relocation ");
                krnSBIPutDec(type);
                krnSBIPutStr(" in ");
                krnSBIPutStr(name);
                krnSBIPutStr("\n");
                return 0;
            }
        }
    }

    return 1;
}

/*
 * Walk an AROS package (see tools/package/FORMAT) and load every module
 * it contains. 'lo'/'hi' receive the range covering the loaded modules,
 * which the caller adds to the romtag scan.
 */
int krnLoadPackage(void *pkg, IPTR pkgsize, IPTR memlow, IPTR memhigh,
                   IPTR *lo, IPTR *hi, IPTR *memused)
{
    UBYTE *p = pkg;
    UBYTE *end = p + pkgsize;
    int count = 0;

    if (p[0] != 'P' || p[1] != 'K' || p[2] != 'G' || p[3] != 0x01)
    {
        krnSBIPutStr("[elf] not an AROS package\n");
        return 0;
    }

    alloc_ptr = (memlow + 15) & ~15UL;
    alloc_end = memhigh;
    loaded_lo = ~(IPTR)0;
    loaded_hi = 0;

    p += 8;         /* 'PKG', version, packageSize */

    while (p < end)
    {
        ULONG namelen = __builtin_bswap32(*(ULONG *)p);
        char *name = (char *)p + 4;
        ULONG datalen;
        UBYTE *data;

        p += 4 + namelen + 1;
        datalen = __builtin_bswap32(*(ULONG *)p);
        data = p + 4;
        p = data + datalen;

        if (!datalen)
            continue;

        krnSBIPutStr("[elf] loading ");
        krnSBIPutStr(name);
        krnSBIPutStr("\n");

        if (!krnLoadModule(data, name))
        {
            krnSBIPutStr("[elf] FAILED to load ");
            krnSBIPutStr(name);
            krnSBIPutStr("\n");
            return 0;
        }
        count++;
    }

    *lo = loaded_lo;
    *hi = loaded_hi;
    *memused = alloc_ptr;
    return count;
}
