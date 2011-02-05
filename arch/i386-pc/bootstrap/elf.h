/*
 * elf.h
 *
 *  Created on: Jun 27, 2010
 *      Author: misc
 */

#ifndef ELF_H_
#define ELF_H_

#include <inttypes.h>

#if 0

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
#define R_PPC_REL16_LO  250
#define R_PPC_REL16_HA  252

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

#define SHF_WRITE   		  (1 << 0)
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


struct elfheader
{
    uint8_t		ident[16];
    uint16_t	type;
    uint16_t	machine;
    uint32_t	version;
    void *		entry;
    uint32_t	phoff;
    uint32_t	shoff;
    uint32_t	flags;
    uint16_t	ehsize;
    uint16_t	phentsize;
    uint16_t	phnum;
    uint16_t	shentsize;
    uint16_t	shnum;
    uint16_t	shstrndx;
};

struct sheader
{
    uint32_t	name;
    uint32_t	type;
    uint32_t	flags;
    void *		addr;
    uint32_t	offset;
    uint32_t	size;
    uint32_t	link;
    uint32_t	info;
    uint32_t	addralign;
    uint32_t	entsize;
};

struct symbol
{
    uint32_t	name;     /* Offset of the name string in the string table */
    uint32_t	value;    /* Varies; eg. the offset of the symbol in its hunk */
    uint32_t	size;     /* How much memory does the symbol occupy */
    uint8_t		info;     /* What kind of symbol is this ? (global, variable, etc) */
    uint8_t		other;    /* undefined */
    uint16_t	shindex;  /* In which section is the symbol defined ? */
};

struct relo
{
    uint32_t	offset;   /* Address of the relocation relative to the section it refers to */
    uint32_t	info;     /* Type of the relocation */
};

/* convert section header number to array index */
#define SHINDEX(n) \
    ((n) < SHN_LORESERVE ? (n) : ((n) <= SHN_HIRESERVE ? 0 : (n) - (SHN_HIRESERVE + 1 - SHN_LORESERVE)))

/* convert section header array index to section number */
#define SHNUM(i) \
    ((i) < SHN_LORESERVE ? (i) : (i) + (SHN_HIRESERVE + 1 - SHN_LORESERVE))

#endif


#define ELF32_R_SYM(val)        ((val) >> 8)
#define ELF32_R_TYPE(val)       ((val) & 0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))

struct bss_tracker {
	void *addr;
	unsigned int length;
};

extern struct bss_tracker tracker[];

int getElfSize(void *elf_file, uint32_t *size_rw, uint32_t *size_ro);
void initAllocator(uintptr_t addr_ro, uintptr_t addr_rw, uintptr_t virtoffset);
int loadElf(void *elf_file);

#endif /* ELF_H_ */
