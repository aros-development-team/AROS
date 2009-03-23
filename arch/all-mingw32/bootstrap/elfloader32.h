#ifndef ELFLOADER_H_
#define ELFLOADER_H_

/*
 Copyright (C) 2006 The AROS Development Team. All rights reserved.
 $Id$
 */

#define SHT_PROGBITS    1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3
#define SHT_RELA    4
#define SHT_NOBITS  8
#define SHT_REL     9

#define ET_REL      1
#define EM_386      3

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

#define ELF64_ST_TYPE(i)    ((i) & 0x0F)

#define ELF32_R_SYM(val)        ((val) >> 8)
#define ELF32_R_TYPE(val)       ((val) & 0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))

struct elfheader
{
  unsigned char ident[16];
  unsigned short type;
  unsigned short machine;
  unsigned int version;
  void*  entry;
  unsigned int phoff;
  unsigned int shoff;
  unsigned int flags;
  unsigned short ehsize;
  unsigned short phentsize;
  unsigned short phnum;
  unsigned short shentsize;
  unsigned short shnum;
  unsigned short shstrndx;
  
  /* these are internal, and not part of the header proper. they are wider
   * versions of shnum and shstrndx for when they don't fit in the header
   * and we need to get them from the first section header. see
   * load_header() for details
   */
  unsigned int int_shnum;
  unsigned int int_shstrndx;
};

struct sheader
{
  unsigned int name;
  unsigned int type;
  unsigned int flags;
  void*  addr;
  unsigned int offset;
  unsigned int size;
  unsigned int link;
  unsigned int info;
  unsigned int addralign;
  unsigned int entsize;
};

struct symbol
{
  unsigned int name;     /* Offset of the name string in the string table */
  unsigned int value;    /* Varies; eg. the offset of the symbol in its hunk */
  unsigned int size;     /* How much memory does the symbol occupy */
  unsigned char info;     /* What kind of symbol is this ? (global, variable, etc) */
  unsigned char other;    /* undefined */
  unsigned short shindex;  /* In which section is the symbol defined ? */
};


struct relo
{
  unsigned int offset;   /* Address of the relocation relative to the section it refers to */
  unsigned int info;     /* Type of the relocation */
};

int load_elf_file(void *, ULONG_PTR);
void *kernel_lowest();
void *kernel_highest();
void set_base_address(void *tracker, void ** sysbaseaddr);

#endif /*ELFLOADER_H_*/
