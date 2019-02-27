#ifndef DOS_ELF_H
#define DOS_ELF_H

/*
    Copyright (C) 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definition of ELF file structures.
    Lang: english
*/

#include <exec/types.h>

/*
 * Define one of ELF_64BIT or ELF_32BIT in your code if you want to enforce specific
 * version of ELF structures. Otherwize it fails back to your native machine's size.
 */
#ifdef ELF_64BIT
#define elf_ptr_t       UQUAD
#define elf_uintptr_t   UQUAD
#define elf_intptr_t    QUAD
#endif

#ifdef ELF_32BIT
#define elf_ptr_t       ULONG
#define elf_uintptr_t   ULONG
#define elf_intptr_t    LONG
#endif

#ifndef elf_ptr_t
#define elf_ptr_t       APTR
#define elf_uintptr_t   IPTR
#define elf_intptr_t    SIPTR

#if (__WORDSIZE == 64)
#define ELF_64BIT
#endif

#endif

#define SHT_PROGBITS       1
#define SHT_SYMTAB         2
#define SHT_STRTAB         3
#define SHT_RELA           4
#define SHT_NOBITS         8
#define SHT_REL            9
#define SHT_SYMTAB_SHNDX   18
#define SHT_ARM_ATTRIBUTES 0x70000003

#define ET_REL          1
#define ET_EXEC         2

#define EM_386          3
#define EM_68K          4
#define EM_PPC          20
#define EM_ARM          40
#define EM_X86_64       62      /* AMD x86-64 */

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2

/* AMD x86-64 relocations.  */
#define R_X86_64_NONE   0       /* No reloc */
#define R_X86_64_64     1       /* Direct 64 bit  */
#define R_X86_64_PC32   2       /* PC relative 32 bit signed */
#define R_X86_64_PLT32  4
#define R_X86_64_32     10
#define R_X86_64_32S    11

#define R_68K_NONE      0
#define R_68K_32        1
#define R_68K_16        2
#define R_68K_8         3
#define R_68K_PC32      4
#define R_68K_PC16      5
#define R_68K_PC8       6

#define R_PPC_NONE      0
#define R_PPC_ADDR32    1
#define R_PPC_ADDR16_LO 4
#define R_PPC_ADDR16_HA 6
#define R_PPC_REL24     10
#define R_PPC_REL32     26
#define R_PPC_REL16_LO  250
#define R_PPC_REL16_HA  252

#define R_ARM_NONE            0
#define R_ARM_PC24            1
#define R_ARM_ABS32           2
#define R_ARM_CALL            28
#define R_ARM_JUMP24          29
#define R_ARM_TARGET1         38
#define R_ARM_V4BX            40
#define R_ARM_TARGET2         41
#define R_ARM_PREL31          42
#define R_ARM_MOVW_ABS_NC     43
#define R_ARM_MOVT_ABS        44
#define R_ARM_THM_CALL        10
#define R_ARM_THM_JUMP24      30
#define R_ARM_THM_MOVW_ABS_NC 47
#define R_ARM_THM_MOVT_ABS    48

#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4
#define STT_LOPROC      13
#define STT_HIPROC      15

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2
#define STB_LOOS        10
#define STB_GNU_UNIQUE  10
#define STB_HIOS        12
#define STB_LOPROC      13
#define STB_HIPROC      15

#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_XINDEX      0xffff
#define SHN_HIRESERVE   0xffff

#define SHF_WRITE       (1 << 0)
#define SHF_ALLOC       (1 << 1)
#define SHF_EXECINSTR   (1 << 2)

#define ELF_ST_TYPE(i)    ((i) & 0x0F)

#define EI_VERSION      6
#define EV_CURRENT      1

#define EI_DATA         5
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

#define EI_CLASS        4
#define ELFCLASS32      1
#define ELFCLASS64      2               /* 64-bit objects */

#define EI_OSABI        7
#define EI_ABIVERSION   8

#define ELFOSABI_AROS   15

#define PF_X            (1 << 0)

struct elfheader
{
    UBYTE         ident[16];
    UWORD         type;
    UWORD         machine;
    ULONG         version;
    elf_ptr_t     entry;
    elf_uintptr_t phoff;
    elf_uintptr_t shoff;
    ULONG         flags;
    UWORD         ehsize;
    UWORD         phentsize;
    UWORD         phnum;
    UWORD         shentsize;
    UWORD         shnum;
    UWORD         shstrndx;
};

struct sheader
{
    ULONG         name;
    ULONG         type;
    elf_uintptr_t flags;
    elf_ptr_t     addr;
    elf_uintptr_t offset;
    elf_uintptr_t size;
    ULONG         link;
    ULONG         info;
    elf_uintptr_t addralign;
    elf_uintptr_t entsize;
};

#define PT_LOAD 1

#ifdef ELF_64BIT

struct pheader
{
    ULONG         type;
    ULONG         flags;
    elf_uintptr_t offset;
    elf_ptr_t     vaddr;
    elf_ptr_t     paddr;                
    elf_uintptr_t filesz;
    elf_uintptr_t memsz;
    elf_uintptr_t align;
};

struct symbol
{
    ULONG         name;     /* Offset of the name string in the string table            */
    UBYTE         info;     /* What kind of symbol is this ? (global, variable, etc)    */
    UBYTE         other;    /* undefined                                                */
    UWORD         shindex;  /* In which section is the symbol defined ?                 */
    elf_uintptr_t value;    /* Varies; eg. the offset of the symbol in its hunk         */
    elf_uintptr_t size;     /* How much memory does the symbol occupy                   */
};

#define ELF_R_SYM(i)          (ULONG)((i) >> 32)
#define ELF_R_TYPE(i)         (ULONG)((i) & 0xffffffffULL)
#define ELF_R_INFO(sym, type) (((UQUAD)(sym) << 32) + (type))

#else

struct pheader
{
    ULONG     type;
    ULONG     offset;
    elf_ptr_t vaddr;
    elf_ptr_t paddr;                
    ULONG     filesz;
    ULONG     memsz;
    ULONG     flags;
    ULONG     align;
};

struct symbol
{
    ULONG         name;     /* Offset of the name string in the string table            */
    elf_uintptr_t value;    /* Varies; eg. the offset of the symbol in its hunk         */
    elf_uintptr_t size;     /* How much memory does the symbol occupy                   */
    UBYTE         info;     /* What kind of symbol is this ? (global, variable, etc)    */
    UBYTE         other;    /* undefined                                                */
    UWORD         shindex;  /* In which section is the symbol defined ?                 */
};

#define ELF_R_SYM(val)        ((val) >> 8)
#define ELF_R_TYPE(val)       ((val) & 0xff)
#define ELF_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))

#endif

#define ELF_S_BIND(val)         ((val) >> 4)
#define ELF_S_TYPE(val)         ((val) & 0xF)
#define ELF_S_INFO(bind, type)  (((bind) << 4) + ((type) & 0xF))

struct rel
{
    elf_uintptr_t offset;   /* Address of the relocation relative to the section it refers to */
    elf_uintptr_t info;     /* Type of the relocation */
};

struct rela
{
    elf_uintptr_t offset;   /* Address of the relocation relative to the section it refers to */
    elf_uintptr_t info;     /* Type of the relocation */
    elf_intptr_t  addend;   /* Constant addend used to compute value */
};

/* Note: the conversion below is not in line with ELF specification and is fixed in GNU binutils since 2008
 * See: https://sourceware.org/bugzilla/show_bug.cgi?id=5900
 */
/* convert section header number to array index */
/*#define SHINDEX(n) \
    ((n) < SHN_LORESERVE ? (n) : ((n) <= SHN_HIRESERVE ? 0 : (n) - (SHN_HIRESERVE + 1 - SHN_LORESERVE)))*/

/* convert section header array index to section number */
/*#define SHNUM(i) \
    ((i) < SHN_LORESERVE ? (i) : (i) + (SHN_HIRESERVE + 1 - SHN_LORESERVE))*/

/* ARM-specific attributes section definitions follow */

#define ATTR_VERSION_CURRENT 0x41

struct attrs_section
{
    ULONG size;
    char  vendor[1];        /* NULL-terminated name */
                            /* Vendor-specific subsections follow */
};

struct attrs_subsection
{
    UBYTE tag;
    ULONG size;
} __attribute__((packed));

#define Tag_File                 1
#define Tag_Section              2
#define Tag_Symbol               3
#define Tag_CPU_raw_name         4
#define Tag_CPU_name             5
#define Tag_CPU_arch             6
#define Tag_FP_arch              10
#define Tag_compatibility        32
#define Tag_also_compatible_with 65
#define Tag_conformance          67

/* Tag_CPU_arch values */
#define ELF_CPU_PREv4    0
#define ELF_CPU_ARMv4    1 
#define ELF_CPU_ARMv4T   2
#define ELF_CPU_ARMv5T   3
#define ELF_CPU_ARMv5TE  4
#define ELF_CPU_ARMv5TEJ 5
#define ELF_CPU_ARMv6    6
#define ELF_CPU_ARMv6KZ  7
#define ELF_CPU_ARMv6T2  8
#define ELF_CPU_ARMv6K   9
#define ELF_CPU_ARMv7    10
#define ELF_CPU_ARM_v6M  11
#define ELF_CPU_ARMv6SM  12
#define ELF_CPU_ARMv7EM  13

/* Tag_FP_arch values */
#define ELF_FP_None     0
#define ELF_FP_v1       1
#define ELF_FP_v2       2
#define ELF_FP_v3       3
#define ELF_FP_v3_Short 4
#define ELF_FP_v4       5
#define ELF_FP_v4_Short 6

/* Machine's native values */
#ifdef ELF_64BIT
#define AROS_ELF_CLASS ELFCLASS64
#else
#define AROS_ELF_CLASS ELFCLASS32
#endif

#if AROS_BIG_ENDIAN
#define AROS_ELF_DATA ELFDATA2MSB
#else
#define AROS_ELF_DATA ELFDATA2LSB
#endif

#if defined(__i386__) || defined(__x86_64__)
#ifdef ELF_64BIT
#define AROS_ELF_MACHINE EM_X86_64
#define AROS_ELF_REL     SHT_RELA
#define relo             rela
#else
#define AROS_ELF_MACHINE EM_386
#define AROS_ELF_REL     SHT_REL
#define relo             rel
#endif
#endif
#ifdef __mc68000__
#define AROS_ELF_MACHINE EM_68K
#define AROS_ELF_REL     SHT_RELA
#define relo             rela
#endif
#ifdef __powerpc__
#define AROS_ELF_MACHINE EM_PPC
#define AROS_ELF_REL     SHT_RELA
#define relo             rela
#endif
#ifdef __arm__
#define AROS_ELF_MACHINE EM_ARM
#define AROS_ELF_REL     SHT_REL
#define relo             rel
#endif

#endif
