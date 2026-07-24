/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: elf.c - AArch64 ELF64 loader for boot
*/

#define ELF_64BIT
#include "elf.h"
#include "boot.h"

#include <dos/elf.h>
#include <stdlib.h>
#include <string.h>

/* Set to 'x' to enable verbose boot ELF-loader tracing */
#define DELF(x)

/*
 * Byte-safe memory copy for potentially unaligned ELF data.
 * The standard memcpy (compiler-rt) may use 8/16-byte loads that fault
 * on QEMU AArch64 when the source is not naturally aligned.
 * Using volatile prevents the compiler from widening to multi-byte loads.
 */
static void elf_memcpy(void *dst, const void *src, unsigned long len)
{
        volatile uint8_t *d = (volatile uint8_t *)dst;
        const volatile uint8_t *s = (const volatile uint8_t *)src;
        while (len--)
                *d++ = *s++;
}

/*
 * Byte-safe bzero for the same reason as elf_memcpy above.
 * The compiler-provided bzero/memset may use NEON STP Q stores
 * requiring 16-byte alignment.
 */
static void elf_bzero(void *dst, unsigned long len)
{
        volatile uint8_t *d = (volatile uint8_t *)dst;
        while (len--)
                *d++ = 0;
}

/*
 * Safe unaligned read for 64-bit ELF header fields.
 * Takes const void * to avoid -Waddress-of-packed-member warnings.
 */
static inline elf_uintptr_t elf_read_uintptr(const void *p)
{
        elf_uintptr_t v;
        elf_memcpy(&v, p, sizeof(v));
        return v;
}

uint32_t        int_shnum;
uint32_t        int_shstrndx;

int checkHeader(struct elfheader *eh)
{
        if (eh->ident[0] != 0x7f || eh->ident[1] != 'E'  ||
                        eh->ident[2] != 'L'  || eh->ident[3] != 'F')
        {
                kprintf("[BOOT:ELF] not ELF\n");
                return 0;
        }

        DELF(kprintf("[BOOT:ELF] class=%d data=%d type=%d machine=%d\n",
                eh->ident[EI_CLASS], eh->ident[EI_DATA], eh->type, eh->machine));
        DELF(kprintf("[BOOT:ELF] shnum=%d\n", eh->shnum));
        DELF(kprintf("[BOOT:ELF] shstrndx=%d\n", eh->shstrndx));
        {
                volatile uint32_t *raw = (volatile uint32_t *)((uint8_t *)eh + 40);
                DELF(kprintf("[BOOT:ELF] shoff raw: %08x %08x\n", raw[0], raw[1]));
        }

        int_shnum = eh->shnum;
        int_shstrndx = eh->shstrndx;

        if (int_shnum == 0 || int_shstrndx == SHN_XINDEX)
        {
                if (elf_read_uintptr(&eh->shoff) == 0)
                {
                        return 0;
                }

                struct sheader *sh = (struct sheader *)((intptr_t)eh + elf_read_uintptr(&eh->shoff));
                DELF(kprintf("[BOOT:ELF] extended: sh=%p\n", sh));

                if (int_shnum == 0)
                        int_shnum = sh->size;

                if (int_shstrndx == SHN_XINDEX)
                        int_shstrndx = sh->link;

                if (int_shnum == 0 || int_shstrndx == SHN_XINDEX)
                {
                        return 0;
                }
        }

        if
        (
                        eh->ident[EI_CLASS]   != ELFCLASS64  ||
                        eh->ident[EI_VERSION] != EV_CURRENT  ||
                        !(eh->type == ET_REL || eh->type == ET_EXEC) ||
                        eh->ident[EI_DATA]    != ELFDATA2LSB ||
                        eh->machine           != EM_AARCH64
        )
        {
                kprintf("[BOOT:ELF] header mismatch\n");
                return 0;
        }

        /*
         * The section-header table (and, in loadElf, the deltas[] array)
         * are VLAs on the small boot stack. A corrupt or hostile shnum
         * (up to 65535) would silently overflow it. BOOT_MAX_SHNUM is
         * derived from BOOT_STACK_SIZE and sits far above any real ROM
         * module (~11 sections); reject anything larger cleanly.
         */
        if (int_shnum > BOOT_MAX_SHNUM)
        {
                kprintf("[BOOT:ELF] too many sections (%d > %d)\n",
                        int_shnum, BOOT_MAX_SHNUM);
                return 0;
        }

        DELF(kprintf("[BOOT:ELF] checkHeader OK shnum=%d\n", int_shnum));
        return 1;
}

int getElfSize(void *elf_file, uint32_t *size_rw, uint32_t *size_ro)
{
        struct elfheader *eh = (struct elfheader *)elf_file;
        uint32_t s_ro = 0;
        uint32_t s_rw = 0;

        DELF(kprintf("[BOOT:ELF] getElfSize(%p)\n", eh));

        if (checkHeader(eh))
        {
                elf_uintptr_t shoff = elf_read_uintptr(&eh->shoff);

                DELF(kprintf("[BOOT:ELF] shoff=%lx shnum=%d\n", (unsigned long)shoff, int_shnum));

                /* Copy section headers to aligned local storage */
                void *sh_src = (void *)((intptr_t)elf_file + shoff);
                DELF(kprintf("[BOOT:ELF] copying %d sheaders from %p (%d bytes)\n",
                        int_shnum, sh_src, (int)(int_shnum * sizeof(struct sheader))));
                struct sheader sh[int_shnum];
                elf_memcpy(sh, sh_src, int_shnum * sizeof(struct sheader));
                int i;

                DELF(kprintf("[BOOT:ELF] memcpy done, scanning sections\n"));

                for (i = 0; i < int_shnum; i++)
                {
                        if (sh[i].flags & SHF_ALLOC)
                        {
                                /* ELF permits addralign 0 (= no constraint);
                                 * (x + 0 - 1) & ~(0 - 1) would collapse to 0 */
                                uint32_t align = sh[i].addralign ? sh[i].addralign : 1;
                                uint32_t size = (sh[i].size + align - 1) & ~(align - 1);

                                if (sh[i].flags & SHF_WRITE)
                                {
                                        s_rw = (s_rw + align - 1) & ~(align - 1);
                                        s_rw += size;
                                }
                                else
                                {
                                        s_ro = (s_ro + align - 1) & ~(align - 1);
                                        s_ro += size;
                                }
                        }
                }
        }
        DELF(kprintf(": ro=%p, rw=%p\n", s_ro, s_rw));

        if (size_ro)
                *size_ro = s_ro;
        if (size_rw)
                *size_rw = s_rw;

        return 1;
}

static uintptr_t ptr_ro;
static uintptr_t ptr_rw;
static uintptr_t virtoffset;

void initAllocator(uintptr_t addr_ro, uintptr_t addr_rw, uintptr_t virtoff)
{
        ptr_ro = addr_ro;
        ptr_rw = addr_rw;
        virtoffset = virtoff;
}

/* Bring-up diagnostic: current RO load position (virtual) */
uintptr_t elf_get_ro_virt(void)
{
        return ptr_ro + virtoffset;
}

struct bss_tracker tracker[MAX_BSS_SECTIONS];
static struct bss_tracker *bss_tracker = &tracker[0];

static inline void read_block(void *file, long offset, void *dest, long length)
{
        elf_memcpy(dest, (void *)((unsigned long)file + offset), length);
}

static int load_hunk(void *file, struct sheader *sh)
{
        void *ptr = (void *)0;

        DELF(kprintf("[BOOT:ELF] load_hunk: type=%d flags=%lx size=%lx align=%lx offset=%lx\n",
                sh->type, (unsigned long)sh->flags, (unsigned long)sh->size,
                (unsigned long)sh->addralign, (unsigned long)sh->offset));

        if (!sh->size)
                return 1;

        /* ELF permits addralign 0 (= no constraint); the round-up below
         * would collapse the pointer to 0 with it */
        unsigned long align = sh->addralign ? (unsigned long)sh->addralign : 1;

        if (sh->flags & SHF_WRITE)
        {
                ptr_rw = (((unsigned long)ptr_rw + align - 1) & ~(align - 1));
                ptr = (APTR)ptr_rw;
                ptr_rw = ptr_rw + sh->size;
                DELF(kprintf("[BOOT:ELF] load_hunk: RW ptr=%p end=%p\n", ptr, (void *)ptr_rw));
        }
        else
        {
                ptr_ro = (((unsigned long)ptr_ro + align - 1) & ~(align - 1));
                ptr = (APTR)ptr_ro;
                ptr_ro = ptr_ro + sh->size;
                DELF(kprintf("[BOOT:ELF] load_hunk: RO ptr=%p end=%p\n", ptr, (void *)ptr_ro));
        }

        sh->addr = (elf_ptr_t)(uintptr_t)ptr;

        if (sh->type != SHT_NOBITS)
        {
                DELF(kprintf("[BOOT:ELF] load_hunk: copying %lx bytes from file+%lx to %p\n",
                        (unsigned long)sh->size, (unsigned long)sh->offset, ptr));
                read_block(file, sh->offset, (void *)((uintptr_t)sh->addr),
                                sh->size);
        }
        else
        {
                DELF(kprintf("[BOOT:ELF] load_hunk: BSS bzero %p len=%lx\n", ptr, (unsigned long)sh->size));
                elf_bzero(ptr, sh->size);
                /* Keep room for the terminator entry written below */
                if (bss_tracker - &tracker[0] >= MAX_BSS_SECTIONS - 1)
                {
                        kprintf("[BOOT:ELF] too many BSS sections (max %d)\n", MAX_BSS_SECTIONS);
                        return 0;
                }
                bss_tracker->addr =
                                (void *)((unsigned long)ptr + virtoffset);
                bss_tracker->length = sh->size;
                bss_tracker++;
                bss_tracker->addr = (void*)0;
                bss_tracker->length = 0;
        }

        return 1;
}

/* Perform relocations of given section - AArch64 RELA relocations */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx,
                uintptr_t virt, uintptr_t *deltas)
{
        struct sheader *shrel = &sh[shrel_idx];
        struct sheader *shsymtab = &sh[shrel->link];
        struct sheader *toreloc = &sh[shrel->info];
        uintptr_t orig_addr = deltas[shrel->info];
        int is_exec = (eh->type == ET_EXEC);

        struct symbol *symtab =
                        (struct symbol *)((uintptr_t)shsymtab->addr);
        struct rela *rel = (struct rela *)((uintptr_t)shrel->addr);
        char *section = (char *)((uintptr_t)toreloc->addr);

        unsigned int numrel = (unsigned long)shrel->size
                        / (unsigned long)shrel->entsize;
        unsigned int i;

        uintptr_t virtoff;

        uint32_t SysBase_sym_idx = (uint32_t)-1;

        for (i = 0; i < numrel; i++, rel++)
        {
                /*
                 * Copy rela entry to aligned local storage.
                 * The RELA section data lives in the raw PKG archive which
                 * may not be 8-byte aligned.  The compiler can emit LDP
                 * instructions for packed-struct field access, and LDP
                 * requires natural alignment even with SCTLR_EL1.A=0.
                 */
                struct rela rel_local;
                elf_memcpy(&rel_local, rel, sizeof(rel_local));

                uint32_t sym_idx = ELF64_R_SYM(rel_local.info);
                uint32_t rtype = ELF64_R_TYPE(rel_local.info);

                /*
                 * Copy symbol entry to aligned local storage (same reason).
                 */
                struct symbol sym_local;
                elf_memcpy(&sym_local, &symtab[sym_idx], sizeof(sym_local));
                struct symbol *sym = &sym_local;

                uintptr_t p = (uintptr_t)&section[rel_local.offset - orig_addr];
                uintptr_t s;
                int64_t addend = rel_local.addend;
                virtoff = virt;

                /*
                 * Section address/link-time delta of the symbol's section,
                 * used by the ET_EXEC arms below. Special section indices
                 * (SHN_ABS & co, >= SHN_LORESERVE) have no section entry --
                 * indexing sh[]/deltas[] with them reads far out of bounds.
                 * An absolute symbol never moves, so 0/0 gives the correct
                 * zero movement (and the right adjustment in the
                 * PC-relative arms: minus the section's own movement).
                 */
                uintptr_t sym_secaddr, sym_secdelta;
                if (sym->shindex < SHN_LORESERVE)
                {
                        sym_secaddr  = (uintptr_t)sh[sym->shindex].addr;
                        sym_secdelta = deltas[sym->shindex];
                }
                else
                {
                        sym_secaddr  = 0;
                        sym_secdelta = 0;
                }

                if (i < 3 || (i % 500 == 0))
                        DELF(kprintf("[BOOT:ELF] rel[%d/%d] type=%d sym=%d p=%p\n", i, numrel, rtype, sym_idx, p));

                if (rtype == R_AARCH64_NONE)
                        continue;

                switch (sym->shindex)
                {
                case SHN_UNDEF:
                        kprintf
                        ("[BOOT:ELF] Undefined symbol '%s' in section '%s'\n",
                                        (char *)((uintptr_t) sh[shsymtab->link].addr) +
                                        sym->name,
                                        (char *)((uintptr_t) sh[eh->shstrndx].addr) +
                                        toreloc->name);
                        return 0;

                case SHN_COMMON:
                        kprintf
                        ("[BOOT:ELF] COMMON symbol '%s' in section '%s'\n",
                                        (char *)((uintptr_t) sh[shsymtab->link].addr) +
                                        sym->name,
                                        (char *)((uintptr_t) sh[eh->shstrndx].addr) +
                                        toreloc->name);
                        return 0;

                case SHN_ABS:
                        if (SysBase_sym_idx == (uint32_t)-1) {
                                if (strncmp
                                                ((char *)((uintptr_t) sh[shsymtab->link].
                                                                addr) + sym->name, "SysBase",
                                                                8) == 0) {
                                        SysBase_sym_idx = sym_idx;
                                        goto SysBase_yes;
                                } else
                                        goto SysBase_no;
                        } else if (SysBase_sym_idx == sym_idx) {
                                SysBase_yes:                    s = (uintptr_t) 4UL;
                                virtoff = 0;
                        } else
                                SysBase_no:                     s = sym->value;
                        break;
                default:
                        s = sym_secaddr + sym->value - sym_secdelta;
                }

                /*
                 * Data relocations (ABS/PREL) can target ANY alignment --
                 * a .quad in packed rodata may sit on a 4-byte boundary.
                 * The bootstrap runs with the MMU off, so every access is
                 * Device memory and an unaligned str/ldr faults; go through
                 * elf_memcpy for the value. Instruction relocations are
                 * safe: instructions are always 4-byte aligned.
                 */
                switch (rtype) {
                case R_AARCH64_ABS64:
                {
                        uint64_t val;
                        elf_memcpy(&val, (void *)p, 8);
                        if (is_exec)
                                val += sym_secaddr - (uintptr_t)sym_secdelta + virtoff;
                        else
                                val = s + addend + virtoff;
                        elf_memcpy((void *)p, &val, 8);
                }
                break;

                case R_AARCH64_ABS32:
                {
                        uint32_t val;
                        elf_memcpy(&val, (void *)p, 4);
                        if (is_exec)
                                val += (uint32_t)(sym_secaddr - (uintptr_t)sym_secdelta + virtoff);
                        else
                                val = (uint32_t)(s + addend + virtoff);
                        elf_memcpy((void *)p, &val, 4);
                }
                break;

                case R_AARCH64_PREL32:
                {
                        uint32_t val;
                        elf_memcpy(&val, (void *)p, 4);
                        if (is_exec)
                        {
                                if (shrel->info != sym->shindex)
                                {
                                        intptr_t expected_delta = sym_secdelta - deltas[shrel->info];
                                        intptr_t actual_delta = sym_secaddr - (uintptr_t)sh[shrel->info].addr;
                                        val += (uint32_t)(actual_delta - expected_delta);
                                }
                        }
                        else
                        {
                                val = (uint32_t)(s + addend - p);
                        }
                        elf_memcpy((void *)p, &val, 4);
                }
                break;

                case R_AARCH64_CALL26:
                case R_AARCH64_JUMP26:
                {
                        uint32_t *loc = (uint32_t *)p;
                        int64_t offset;

                        if (is_exec)
                        {
                                if (shrel->info != sym->shindex)
                                {
                                        intptr_t expected_delta = sym_secdelta - deltas[shrel->info];
                                        intptr_t actual_delta = sym_secaddr - (uintptr_t)sh[shrel->info].addr;

                                        offset = (int64_t)(int32_t)((*loc & 0x03ffffff) << 6) >> 4;
                                        offset += actual_delta - expected_delta;
                                        *loc = (*loc & 0xfc000000) | ((uint32_t)(offset >> 2) & 0x03ffffff);
                                }
                        }
                        else
                        {
                                offset = (int64_t)(s + addend) - (int64_t)p;
                                *loc = (*loc & 0xfc000000) | ((uint32_t)(offset >> 2) & 0x03ffffff);
                        }
                }
                break;

                case R_AARCH64_ADR_PREL_PG_HI21:
                case R_AARCH64_ADR_PREL_PG_HI21_NC:
                {
                        uint32_t *loc = (uint32_t *)p;
                        int64_t offset;

                        if (is_exec)
                        {
                                if (shrel->info != sym->shindex)
                                {
                                        /* ADRP: page-relative, need to recompute */
                                        uintptr_t target = s + addend + virtoff;
                                        uintptr_t pc_virt = p + virtoff;
                                        offset = ((target & ~0xFFFUL) - (pc_virt & ~0xFFFUL)) >> 12;
                                        /* Re-encode immhi:immlo in ADRP instruction */
                                        *loc = (*loc & 0x9F00001F) |
                                               ((uint32_t)(offset & 0x3) << 29) |
                                               ((uint32_t)((offset >> 2) & 0x7FFFF) << 5);
                                }
                        }
                        else
                        {
                                uintptr_t target = s + addend + virtoff;
                                uintptr_t pc_virt = p + virtoff;
                                offset = ((int64_t)((target & ~0xFFFUL) - (pc_virt & ~0xFFFUL))) >> 12;
                                *loc = (*loc & 0x9F00001F) |
                                       ((uint32_t)(offset & 0x3) << 29) |
                                       ((uint32_t)((offset >> 2) & 0x7FFFF) << 5);
                        }
                }
                break;

                case R_AARCH64_ADD_ABS_LO12_NC:
                case R_AARCH64_LDST8_ABS_LO12_NC:
                {
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFF << 10)) | (((uint32_t)(target & 0xFFF)) << 10);
                }
                break;

                case R_AARCH64_LDST16_ABS_LO12_NC:
                {
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFF << 10)) | (((uint32_t)((target & 0xFFF) >> 1)) << 10);
                }
                break;

                case R_AARCH64_LDST32_ABS_LO12_NC:
                {
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFF << 10)) | (((uint32_t)((target & 0xFFF) >> 2)) << 10);
                }
                break;

                case R_AARCH64_LDST64_ABS_LO12_NC:
                {
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFF << 10)) | (((uint32_t)((target & 0xFFF) >> 3)) << 10);
                }
                break;

                case R_AARCH64_LDST128_ABS_LO12_NC:
                {
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFF << 10)) | (((uint32_t)((target & 0xFFF) >> 4)) << 10);
                }
                break;

                case R_AARCH64_MOVW_UABS_G0:
                case R_AARCH64_MOVW_UABS_G0_NC:
                {
                        /* MOVZ/MOVK: bits [15:0] of S + A */
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFFF << 5)) | (((uint32_t)(target & 0xFFFF)) << 5);
                }
                break;

                case R_AARCH64_MOVW_UABS_G1:
                case R_AARCH64_MOVW_UABS_G1_NC:
                {
                        /* MOVZ/MOVK: bits [31:16] of S + A */
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFFF << 5)) | (((uint32_t)((target >> 16) & 0xFFFF)) << 5);
                }
                break;

                case R_AARCH64_MOVW_UABS_G2:
                case R_AARCH64_MOVW_UABS_G2_NC:
                {
                        /* MOVZ/MOVK: bits [47:32] of S + A */
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFFF << 5)) | (((uint32_t)((target >> 32) & 0xFFFF)) << 5);
                }
                break;

                case R_AARCH64_MOVW_UABS_G3:
                {
                        /* MOVZ/MOVK: bits [63:48] of S + A */
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        *loc = (*loc & ~(0xFFFF << 5)) | (((uint32_t)((target >> 48) & 0xFFFF)) << 5);
                }
                break;

                case R_AARCH64_LD_PREL_LO19:
                {
                        /* LDR (literal): S + A - P, imm19 in bits [23:5], scaled by 4 */
                        uint32_t *loc = (uint32_t *)p;
                        int64_t offset;
                        if (is_exec)
                        {
                                if (shrel->info != sym->shindex)
                                {
                                        intptr_t expected_delta = sym_secdelta - deltas[shrel->info];
                                        intptr_t actual_delta = sym_secaddr - (uintptr_t)sh[shrel->info].addr;
                                        offset = (int64_t)(int32_t)((*loc >> 5) & 0x7FFFF) << 2;
                                        offset += actual_delta - expected_delta;
                                        *loc = (*loc & ~(0x7FFFF << 5)) | (((uint32_t)((offset >> 2) & 0x7FFFF)) << 5);
                                }
                        }
                        else
                        {
                                offset = (int64_t)(s + addend) - (int64_t)p;
                                *loc = (*loc & ~(0x7FFFF << 5)) | (((uint32_t)((offset >> 2) & 0x7FFFF)) << 5);
                        }
                }
                break;

                case R_AARCH64_ADR_PREL_LO21:
                {
                        /*
                         * ADR: S + A - P, immhi in [23:5], immlo in [30:29]
                         * ADR is PC-relative: at runtime PC is virtual,
                         * so use virtual addresses for both target and PC.
                         */
                        uint32_t *loc = (uint32_t *)p;
                        int64_t offset;
                        uintptr_t pc_virt = p + virtoff;
                        if (is_exec)
                        {
                                if (shrel->info != sym->shindex)
                                {
                                        uintptr_t target = s + addend + virtoff;
                                        offset = (int64_t)target - (int64_t)pc_virt;
                                        *loc = (*loc & 0x9F00001F) |
                                               ((uint32_t)(offset & 0x3) << 29) |
                                               ((uint32_t)((offset >> 2) & 0x7FFFF) << 5);
                                }
                        }
                        else
                        {
                                offset = (int64_t)(s + addend + virtoff) - (int64_t)pc_virt;
                                *loc = (*loc & 0x9F00001F) |
                                       ((uint32_t)(offset & 0x3) << 29) |
                                       ((uint32_t)((offset >> 2) & 0x7FFFF) << 5);
                        }
                }
                break;

                case R_AARCH64_CONDBR19:
                {
                        /* B.cond/CBZ/CBNZ: S + A - P, imm19 in bits [23:5], scaled by 4 */
                        uint32_t *loc = (uint32_t *)p;
                        int64_t offset;
                        if (is_exec)
                        {
                                if (shrel->info != sym->shindex)
                                {
                                        intptr_t expected_delta = sym_secdelta - deltas[shrel->info];
                                        intptr_t actual_delta = sym_secaddr - (uintptr_t)sh[shrel->info].addr;
                                        offset = (int64_t)(int32_t)((*loc >> 5) & 0x7FFFF) << 2;
                                        offset += actual_delta - expected_delta;
                                        *loc = (*loc & ~(0x7FFFF << 5)) | (((uint32_t)((offset >> 2) & 0x7FFFF)) << 5);
                                }
                        }
                        else
                        {
                                offset = (int64_t)(s + addend) - (int64_t)p;
                                *loc = (*loc & ~(0x7FFFF << 5)) | (((uint32_t)((offset >> 2) & 0x7FFFF)) << 5);
                        }
                }
                break;

                case R_AARCH64_TSTBR14:
                {
                        /* TBZ/TBNZ: S + A - P, imm14 in bits [18:5], scaled by 4 */
                        uint32_t *loc = (uint32_t *)p;
                        int64_t offset;
                        if (is_exec)
                        {
                                if (shrel->info != sym->shindex)
                                {
                                        intptr_t expected_delta = sym_secdelta - deltas[shrel->info];
                                        intptr_t actual_delta = sym_secaddr - (uintptr_t)sh[shrel->info].addr;
                                        offset = (int64_t)(int32_t)((*loc >> 5) & 0x3FFF) << 2;
                                        offset += actual_delta - expected_delta;
                                        *loc = (*loc & ~(0x3FFF << 5)) | (((uint32_t)((offset >> 2) & 0x3FFF)) << 5);
                                }
                        }
                        else
                        {
                                offset = (int64_t)(s + addend) - (int64_t)p;
                                *loc = (*loc & ~(0x3FFF << 5)) | (((uint32_t)((offset >> 2) & 0x3FFF)) << 5);
                        }
                }
                break;

                case R_AARCH64_PREL64:
                {
                        /* Data relocation: alignment-safe store, see ABS64 */
                        uint64_t val = s + addend - p;
                        elf_memcpy((void *)p, &val, 8);
                }
                break;

                case R_AARCH64_ADR_GOT_PAGE:
                {
                        /*
                         * GOT relaxation: rewrite ADRP to point at the symbol
                         * page directly instead of through a GOT entry.
                         * The boot loader resolves all addresses statically,
                         * so no GOT indirection is needed.
                         *
                         * ADRP is PC-relative: at runtime PC is virtual,
                         * so we must use virtual address for both target
                         * and instruction location.
                         */
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value
                                         - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        uintptr_t pc_virt = p + virtoff;
                        int64_t offset = ((int64_t)((target & ~0xFFFUL) - (pc_virt & ~0xFFFUL))) >> 12;
                        *loc = (*loc & 0x9F00001F) |
                               ((uint32_t)(offset & 0x3) << 29) |
                               ((uint32_t)((offset >> 2) & 0x7FFFF) << 5);
                }
                break;

                case R_AARCH64_LD64_GOT_LO12_NC:
                {
                        /*
                         * GOT relaxation: convert LDR (load from GOT) to ADD
                         * (compute address directly). The ADRP from the paired
                         * ADR_GOT_PAGE already points at the symbol page, so
                         * we just need the low 12 bits as an immediate.
                         */
                        uint32_t *loc = (uint32_t *)p;
                        uintptr_t target;
                        if (is_exec)
                                target = sym_secaddr + sym->value
                                         - sym_secdelta + addend + virtoff;
                        else
                                target = s + addend + virtoff;
                        /* Extract Rd and Rn from original LDR instruction */
                        uint32_t insn = *loc;
                        uint32_t Rd = insn & 0x1F;
                        uint32_t Rn = (insn >> 5) & 0x1F;
                        /* Build ADD Xd, Xn, #imm12 */
                        uint32_t imm12 = (uint32_t)(target & 0xFFF);
                        *loc = 0x91000000 | (imm12 << 10) | (Rn << 5) | Rd;
                }
                break;

                default:
                        kprintf("[BOOT:ELF] Unknown relocation %d in ELF file\n",
                                        rtype);
                        return 0;
                }
        }
        return 1;
}

int loadElf(void *elf_file)
{
        struct elfheader *eh = (struct elfheader *)elf_file;

        DELF(kprintf("[BOOT] loadElf(%p)\n", eh));

        if (checkHeader(eh))
        {
                /*
                 * Declared here, after checkHeader() has set (and bounds-
                 * checked) int_shnum for THIS file. int_shnum is a global;
                 * declaring the VLA before the call would size it from the
                 * previously loaded module's section count.
                 */
                uintptr_t deltas[int_shnum];
                elf_uintptr_t shoff = elf_read_uintptr(&eh->shoff);

                /*
                 * Copy section headers to aligned local storage.
                 * BSP modules in PKG archives may sit at non-8-byte-aligned addresses,
                 * making direct 64-bit field access from the original location unsafe.
                 */
                struct sheader sh[int_shnum];
                elf_memcpy(sh, (void *)((intptr_t)elf_file + shoff),
                       int_shnum * sizeof(struct sheader));
                int i;

                for (i = 0; i < int_shnum; i++)
                {
                        DELF(kprintf("[BOOT:ELF] section[%d] type=%d flags=%lx size=%lx\n",
                                i, sh[i].type, (unsigned long)sh[i].flags, (unsigned long)sh[i].size));
                        if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB)
                        {
                                sh[i].addr = (elf_ptr_t)((uintptr_t)elf_file + sh[i].offset);
                        }
                        else if (sh[i].flags & SHF_ALLOC)
                        {
                                deltas[i] = (uintptr_t)sh[i].addr;
                                if (!load_hunk(elf_file, &sh[i]))
                                {
                                        return 0;
                                }
                                else
                                {
                                        if (sh[i].size)
                                        {
                                                DELF(kprintf("[BOOT:ELF] %s section loaded at %p (Virtual addr: %p, requested addr: %p)\n",
                                                                sh[i].flags & SHF_WRITE ? "RW":"RO",
                                                                                sh[i].addr,
                                                                                (uintptr_t)sh[i].addr + virtoffset,
                                                                                deltas[i]));
                                        }
                                }
                        }
                }

                /* For every loaded section perform the RELA relocations */
                for (i = 0; i < int_shnum; i++)
                {
                        if (sh[i].type == SHT_RELA && sh[sh[i].info].addr)
                        {
                                struct sheader *shrel = &sh[i];
                                unsigned int numrel = (unsigned long)shrel->size / (unsigned long)shrel->entsize;
                                DELF(kprintf("[BOOT:ELF] Relocating section %d -> %d (%d entries)\n", i, shrel->info, numrel));
                                sh[i].addr = (elf_ptr_t)((uintptr_t)elf_file + sh[i].offset);
                                if (!sh[i].addr || !relocate(eh, sh, i, virtoffset, deltas))
                                {
                                        kprintf("[BOOT:ELF] Relocation of section %d failed\n", i);
                                        return 0;
                                }
                                DELF(kprintf("[BOOT:ELF] Section %d relocated OK\n", i));
                        }
                }
        }
    return 1;
}
