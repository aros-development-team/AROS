/*
 * $Id$
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef ELF_H_
#define ELF_H_

#include "context.h"

struct bss_tracker {
	void *addr;
	unsigned int length;
};

extern struct bss_tracker tracker[];

int load_elf_file(const char *name, void *file);
char *get_ptr_rw();
char *get_ptr_ro();

#define SHT_PROGBITS    1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3
#define SHT_RELA    4
#define SHT_NOBITS  8
#define SHT_REL     9

#define ET_REL      1
#define EM_386      3
#define EM_PPC      20
#define EM_X86_64   62

#define STT_OBJECT  1
#define STT_FUNC    2

#define SHN_ABS     0xfff1
#define SHN_COMMON  0xfff2
#define SHN_UNDEF   0

#define SHF_WRITE   (1 << 0)
#define SHF_ALLOC   (1 << 1)
#define SHF_EXECINSTR   (1 << 2)

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2

#define R_X86_64_NONE   0
#define R_X86_64_64 1
#define R_X86_64_PC32   2
#define R_X86_64_32 10
#define R_X86_64_32S    11

#define R_PPC_NONE      0
#define R_PPC_ADDR32    1
#define R_PPC_ADDR16_LO 4
#define R_PPC_ADDR16_HA 6
#define R_PPC_REL24     10
#define R_PPC_REL32     26

#define ELF64_ST_TYPE(i)    ((i) & 0x0F)

#define ELF64_R_SYM(i)      (unsigned long)((i) >> 32)
#define ELF64_R_TYPE(i)     (unsigned long)((i) & 0xffffffffULL)
#define ELF64_R_INFO(sym, type) (((unsigned long long)(sym) << 32) + (type))

#define ELF32_R_SYM(i)          ((i) >> 8)
#define ELF32_R_TYPE(i)         ((i) & 0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))

struct elfheader {
	uint8_t ident[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
};

struct sheader {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t addralign;
	uint32_t entsize;
};

struct pheader {
	uint32_t p_type;	/* Segment type */
	uint32_t p_offset;	/* Segment file offset */
	uint32_t p_vaddr;	/* Segment virtual address */
	uint32_t p_paddr;	/* Segment physical address */
	uint32_t p_filesz;	/* Segment size in file */
	uint32_t p_memsz;	/* Segment size in memory */
	uint32_t p_flags;	/* Segment flags */
	uint32_t p_align;	/* Segment alignment */
};

struct symbol {
	uint32_t name;
	uint32_t value;
	uint32_t size;
	uint8_t info;
	uint8_t other;
	uint16_t shindex;
};

struct relo {
	uint32_t offset;
	uint32_t info;
	int32_t addend;
};

#endif /*ELF_H_ */
