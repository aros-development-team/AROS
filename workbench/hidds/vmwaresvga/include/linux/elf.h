/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_ELF_H_
#define _LINUX_ELF_H_

#include <linux/types.h>
typedef u32 Elf32_Addr; typedef u16 Elf32_Half; typedef u32 Elf32_Off; typedef s32 Elf32_Sword; typedef u32 Elf32_Word;
typedef u64 Elf64_Addr; typedef u16 Elf64_Half; typedef s16 Elf64_SHalf; typedef u64 Elf64_Off; typedef s32 Elf64_Sword; typedef u32 Elf64_Word; typedef u64 Elf64_Xword; typedef s64 Elf64_Sxword;
#define EI_NIDENT 16
typedef struct elf32_hdr { unsigned char e_ident[EI_NIDENT]; Elf32_Half e_type; Elf32_Half e_machine; Elf32_Word e_version; Elf32_Addr e_entry; Elf32_Off e_phoff; Elf32_Off e_shoff; Elf32_Word e_flags; Elf32_Half e_ehsize; Elf32_Half e_phentsize; Elf32_Half e_phnum; Elf32_Half e_shentsize; Elf32_Half e_shnum; Elf32_Half e_shstrndx; } Elf32_Ehdr;
typedef struct elf64_hdr { unsigned char e_ident[EI_NIDENT]; Elf64_Half e_type; Elf64_Half e_machine; Elf64_Word e_version; Elf64_Addr e_entry; Elf64_Off e_phoff; Elf64_Off e_shoff; Elf64_Word e_flags; Elf64_Half e_ehsize; Elf64_Half e_phentsize; Elf64_Half e_phnum; Elf64_Half e_shentsize; Elf64_Half e_shnum; Elf64_Half e_shstrndx; } Elf64_Ehdr;
typedef struct elf32_shdr { Elf32_Word sh_name; Elf32_Word sh_type; Elf32_Word sh_flags; Elf32_Addr sh_addr; Elf32_Off sh_offset; Elf32_Word sh_size; Elf32_Word sh_link; Elf32_Word sh_info; Elf32_Word sh_addralign; Elf32_Word sh_entsize; } Elf32_Shdr;
typedef struct elf64_shdr { Elf64_Word sh_name; Elf64_Word sh_type; Elf64_Xword sh_flags; Elf64_Addr sh_addr; Elf64_Off sh_offset; Elf64_Xword sh_size; Elf64_Word sh_link; Elf64_Word sh_info; Elf64_Xword sh_addralign; Elf64_Xword sh_entsize; } Elf64_Shdr;
typedef struct elf32_phdr { Elf32_Word p_type; Elf32_Off p_offset; Elf32_Addr p_vaddr; Elf32_Addr p_paddr; Elf32_Word p_filesz; Elf32_Word p_memsz; Elf32_Word p_flags; Elf32_Word p_align; } Elf32_Phdr;
typedef struct elf64_phdr { Elf64_Word p_type; Elf64_Word p_flags; Elf64_Off p_offset; Elf64_Addr p_vaddr; Elf64_Addr p_paddr; Elf64_Xword p_filesz; Elf64_Xword p_memsz; Elf64_Xword p_align; } Elf64_Phdr;
#define ELFMAG "\177ELF"
#define SELFMAG 4
#define EI_CLASS 4
#define ELFCLASS32 1
#define ELFCLASS64 2
#define PT_LOAD 1
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_NOBITS 8
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_STRINGS 0x20
#define SHF_OS_NONCONFORMING 0x100
#define SHF_MASKOS 0x0ff00000
#define SHF_MASKPROC 0xf0000000
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define EI_DATA 5
#define ELFDATA2LSB 1
#define EM_RISCV 243
#define ET_EXEC 2
#define PT_NULL 0
#define PF_R 0x4
#define PF_W 0x2
#define PF_X 0x1

#endif /* _LINUX_ELF_H_ */
