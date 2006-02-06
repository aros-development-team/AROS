#ifndef ELFLOADER_H_
#define ELFLOADER_H_

/*
    Copyright (C) 2006 The AROS Development Team. All rights reserved.
    $Id:$
*/

#define SHT_PROGBITS    1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3
#define SHT_RELA    4
#define SHT_NOBITS  8
#define SHT_REL     9

#define ET_REL      1
#define EM_X86_64   62

#define STT_OBJECT  1
#define STT_FUNC    2

#define SHN_ABS     0xfff1
#define SHN_COMMON  0xfff2
#define SHN_UNDEF   0

#define SHF_WRITE   (1 << 0)
#define SHF_ALLOC   (1 << 1)
#define SHF_EXECINSTR   (1 << 2)

#define R_X86_64_NONE   0
#define R_X86_64_64 1
#define R_X86_64_PC32   2
#define R_X86_64_32 10
#define R_X86_64_32S    11

#define ELF64_ST_TYPE(i)    ((i) & 0x0F)

#define ELF64_R_SYM(i)      (unsigned long)((i) >> 32)
#define ELF64_R_TYPE(i)     (unsigned long)((i) & 0xffffffffULL)
#define ELF64_R_INFO(sym, type) (((unsigned long long)(sym) << 32) + (type))

struct elfheader {
    unsigned char   ident[16];
    unsigned short  type;
    unsigned short  machine;
    unsigned int    version;
    unsigned long long  entry;
    unsigned long long  phoff;
    unsigned long long  shoff;
    unsigned int    flags;
    unsigned short  ehsize;
    unsigned short  phentsize;
    unsigned short  phnum;
    unsigned short  shentsize;
    unsigned short  shnum;
    unsigned short  shstrndx;
};

struct sheader {
    unsigned int    name;
    unsigned int    type;
    unsigned long long  flags;
    unsigned long long  addr;
    unsigned long long  offset;
    unsigned long long  size;
    unsigned int    link;
    unsigned int    info;
    unsigned long long  addralign;
    unsigned long long  entsize;
};

struct symbol {
    unsigned int    name;
    unsigned char   info;
    unsigned char   other;
    unsigned short  shindex;
    unsigned long long  value;
    unsigned long long  size;
};

struct relo {
    unsigned long long  offset;
    unsigned long long  info;
    signed long long    addend;
};

void load_elf_file(void *);

#endif /*ELFLOADER_H_*/
