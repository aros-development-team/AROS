/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

/*
** Based on code from the ppc.library emulator by Frank Wille:
** ppc.library emulation
** (c)1998-99 Frank Wille <frank@phoenix.owl.de>
*/


#include <exec/types.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

// Even though this is WarpUp only, we still need a few defines from PowerUp...
#include <powerup/ppclib/object.h>

#include <string.h>

#include "ahi_def.h"
#include "elfloader.h"
#include "misc.h"

/*** elfcommon.h *************************************************************/

/* e_indent indexes */
#define EI_NIDENT  16
#define EI_MAG0    0
#define EI_MAG1    1
#define EI_MAG2    2
#define EI_MAG3    3
#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6
#define EI_PAD     7

/* EI_CLASS */
#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

/* EI_DATA */
#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

/* e_type */
#define ET_NONE   0                 /* No file type */
#define ET_REL    1                 /* Relocatable file */
#define ET_EXEC   2                 /* Executable file */
#define ET_DYN    3                 /* Shared object file */
#define ET_CORE   4                 /* Core file */
#define ET_LOPROC 0xFF00            /* Processor-specific */
#define ET_HIPROC 0xFFFF            /* Processor-specific */

/* e_version */
#define EV_NONE    0
#define EV_CURRENT 1

/* e_machine */
#define EM_NONE           0
#define EM_M32            1
#define EM_SPARC          2
#define EM_386            3
#define EM_68K            4
#define EM_88K            5
#define EM_860            7
#define EM_MIPS           8
#define EM_MIPS_RS4_BE    10
#define EM_SPARC64        11
#define EM_PARISC         15
#define EM_PPC_OLD        17
#define EM_SPARC32PLUS    18
#define EM_PPC            20
#define EM_CYGNUS_POWERPC 0x9025
#define EM_ALPHA          0x9026

/* values for program header, p_type field */
#define PT_NULL    0                /* Program header table entry unused */
#define PT_LOAD    1                /* Loadable program segment */
#define PT_DYNAMIC 2                /* Dynamic linking information */
#define PT_INTERP  3                /* Program interpreter */
#define PT_NOTE    4                /* Auxiliary information */
#define PT_SHLIB   5                /* Reserved, unspecified semantics */
#define PT_PHDR    6                /* Entry for header table itself */
#define PT_LOPROC  0x70000000       /* Processor-specific */
#define PT_HIPROC  0x7FFFFFFF       /* Processor-specific */

/* Program segment permissions, in program header p_flags field */
#define PF_X        (1 << 0)        /* Segment is executable */
#define PF_W        (1 << 1)        /* Segment is writable */
#define PF_R        (1 << 2)        /* Segment is readable */
#define PF_MASKPROC 0xF0000000      /* Processor-specific reserved bits */

/* special sections indexes */
#define SHN_UNDEF 0
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2

/* sh_type */
#define SHT_NULL        0           /* Section header table entry unused */
#define SHT_PROGBITS    1           /* Program specific (private) data */
#define SHT_SYMTAB      2           /* Link editing symbol table */
#define SHT_STRTAB      3           /* A string table */
#define SHT_RELA        4           /* Relocation entries with addends */
#define SHT_HASH        5           /* A symbol hash table */
#define SHT_DYNAMIC     6           /* Information for dynamic linking */
#define SHT_NOTE        7           /* Information that marks file */
#define SHT_NOBITS      8           /* Section occupies no space in file */
#define SHT_REL         9           /* Relocation entries, no addends */
#define SHT_SHLIB       10          /* Reserved, unspecified semantics */
#define SHT_DYNSYM      11          /* Dynamic linking symbol table */
#define SHT_LOPROC      0x70000000  /* Processor-specific semantics, lo */
#define SHT_HIPROC      0x7FFFFFFF  /* Processor-specific semantics, hi */
#define SHT_LOUSER      0x80000000  /* Application-specific semantics */
#define SHT_HIUSER      0x8FFFFFFF  /* Application-specific semantics */

/* sh_flags */
#define SHF_WRITE     (1 << 0)      /* Writable data during execution */
#define SHF_ALLOC     (1 << 1)      /* Occupies memory during execution */
#define SHF_EXECINSTR (1 << 2)      /* Executable machine instructions */
#define SHF_MASKPROC  0xF0000000    /* Processor-specific semantics */

/* Values of note segment descriptor types for core files. */
#define NT_PRSTATUS 1               /* Contains copy of prstatus struct */
#define NT_FPREGSET 2               /* Contains copy of fpregset struct */
#define NT_PRPSINFO 3               /* Contains copy of prpsinfo struct */

#define STN_UNDEF 0                 /* undefined symbol index */

/* ST_BIND */
#define STB_LOCAL  0                /* Symbol not visible outside obj */
#define STB_GLOBAL 1                /* Symbol visible outside obj */
#define STB_WEAK   2                /* Like globals, lower precedence */
#define STB_LOPROC 13               /* Application-specific semantics */
#define STB_HIPROC 15               /* Application-specific semantics */

/* ST_TYPE */
#define STT_NOTYPE  0               /* Symbol type is unspecified */
#define STT_OBJECT  1               /* Symbol is a data object */
#define STT_FUNC    2               /* Symbol is a code object */
#define STT_SECTION 3               /* Symbol associated with a section */
#define STT_FILE    4               /* Symbol gives a file name */
#define STT_LOPROC  13              /* Application-specific semantics */
#define STT_HIPROC  15              /* Application-specific semantics */

/* Dynamic section tags */
#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_INIT     12
#define DT_FINI     13
#define DT_SONAME   14
#define DT_RPATH    15
#define DT_SYMBOLIC 16
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20
#define DT_DEBUG    21
#define DT_TEXTREL  22
#define DT_JMPREL   23
#define DT_LOPROC   0x70000000
#define DT_HIPROC   0x7fffffff

/*** elf32.h *****************************************************************/

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;


struct Elf32_Ehdr {
  unsigned char e_ident[EI_NIDENT];
  uint16 e_type;
  uint16 e_machine;
  uint32 e_version;
  uint32 e_entry;
  uint32 e_phoff;
  uint32 e_shoff;
  uint32 e_flags;
  uint16 e_ehsize;
  uint16 e_phentsize;
  uint16 e_phnum;
  uint16 e_shentsize;
  uint16 e_shnum;
  uint16 e_shstrndx;
};

struct Elf32_Shdr {
  uint32 sh_name;
  uint32 sh_type;
  uint32 sh_flags;
  uint32 sh_addr;
  uint32 sh_offset;
  uint32 sh_size;
  uint32 sh_link;
  uint32 sh_info;
  uint32 sh_addralign;
  uint32 sh_entsize;
};

struct Elf32_Sym {
  uint32 st_name;
  uint32 st_value;
  uint32 st_size;
  uint8 st_info;
  uint8 st_other;
  uint16 st_shndx;
};

/* st_info */
#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

struct Elf32_Rel {
  uint32 r_offset;
  uint32 r_info;
};

struct Elf32_Rela {
  uint32 r_offset;
  uint32 r_info;
  uint32 r_addend;
};

/* r_info */
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

/*** elfobject.h *************************************************************/

struct ELFSection {                     /* prog. sections for new tasks */
  char *name;
  APTR address;                         /* physical RAM address or NULL */
  UWORD flags;
  UWORD alignment;
  long offset;                          /* file offset and section size */
  ULONG size;
  int nrel;                             /* number of relocs */
  struct Elf32_Rela *relocs;            /* array of Elf32_Rela structs */
};

#define ElfSecB_NOBITS          0       /* bss section */
#define ElfSecF_NOBITS          1
#define ElfSecB_RELA            1       /* Relocs are of .rela type */
#define ElfSecF_RELA            2


struct ElfObject {
//  struct Node n;                        /* contains object's name ptr */
  UWORD flags;
//  struct PPCLibBase *ppcbase;
//  void *pool;                           /* pooled memory for object data */
  BPTR handle;                          /* ELF file handle */
//  APTR handle;                          /* ELF file handle */
//  struct Hook *hook;                    /* hook functions for read/seek */
//  struct Hook defaultHook;
  char *secnames;                       /* .shstrtab - section names */
  char *symnames;                       /* .strtab - symbol mames */
  struct Elf32_Ehdr *header;            /* the ELF file header */
  struct Elf32_Sym *symtab;             /* .symtab - symbol table */
  uint32 nsyms;                         /* number of symbols */
  uint32 gsyms;                         /* first global symbol */
  struct ELFSection **sections;         /* ELFSection pointers */
  uint32 nsects;                        /* number of sections */
//  struct SignalSemaphore ElfSS;         /* ElfObject access arbitration */
};

#define ElfObjB_Relocated       0       /* Object is relocated */
#define ElfObjF_Relocated       1

/*** relocnames.c ************************************************************/

const char *reloc_name[] = {
  "R_NONE",
  "R_PPC_ADDR32",
  "R_PPC_ADDR24",
  "R_PPC_ADDR16",
  "R_PPC_ADDR16_LO",
  "R_PPC_ADDR16_HI",
  "R_PPC_ADDR16_HA",
  "R_PPC_ADDR14",
  "R_PPC_ADDR14_BRTAKEN",
  "R_PPC_ADDR14_BRNTAKEN",
  "R_PPC_REL24",
  "R_PPC_REL14",
  "R_PPC_REL14_BRTAKEN",
  "R_PPC_REL14_BRNTAKEN",
  "R_PPC_GOT16",
  "R_PPC_GOT16_LO",
  "R_PPC_GOT16_HI",
  "R_PPC_GOT16_HA",
  "R_PPC_PLTREL24",
  "R_PPC_COPY",
  "R_PPC_GLOB_DAT",
  "R_PPC_JMP_SLOT",
  "R_PPC_RELATIVE",
  "R_PPC_LOCAL24PC",
  "R_PPC_UADDR32",
  "R_PPC_UADDR16",
  "R_PPC_REL32",
  "R_PPC_PLT32",
  "R_PPC_PLTREL32",
  "R_PPC_PLT16_LO",
  "R_PPC_PLT16_HI",
  "R_PPC_PLT16_HA",
  "R_PPC_SDAREL16",
  "R_PPC_SECTOFF",
  "R_PPC_SECTOFF_LO",
  "R_PPC_SECTOFF_HI",
  "R_PPC_SECTOFF_HA"
};

/*** ppcobject.c *************************************************************/


/* Bug fix for powerup/ppclib/object.h */
#ifdef R_PPC_ADDR16_L
#ifndef R_PPC_ADDR16_LO
#define R_PPC_ADDR16_LO R_PPC_ADDR16_L
#endif
#endif

static void
freeelfobj(struct ElfObject *elfobj);

static BOOL
loadelf32(struct ElfObject *eo,struct Elf32_Shdr *shdrs);

static BOOL
scanElfSymbols(struct ElfObject *eo,struct PPCObjectInfo *info,
                      BOOL relmode);

static BOOL
getsyminfo(struct ElfObject *eo,struct PPCObjectInfo *info,
           struct Elf32_Sym *stab);

static APTR
loadprogram(struct ElfObject *eo);

static void
freeprogram(struct ElfObject *eo);

static BOOL
relocate(struct ElfObject *eo);

static void*
allocstr( const char* string );

static void*
alloc32c( size_t size );

static void*
alloc32( size_t size );

void
free32( void* addr );

static BPTR
opstream(struct ElfObject *eo, const char* name );

static BOOL
clstream(struct ElfObject *eo);

static BOOL
rdstream(struct ElfObject *eo,void *buf,long len);

static long
skstream(struct ElfObject *eo,long offs,long mode);

static BOOL
prstream(struct ElfObject *eo,long offs,void *buf,long len);

static void*
readsection(struct ElfObject *eo,struct Elf32_Shdr *sh);

static struct ELFSection*
progsection(struct ElfObject *eo, struct Elf32_Shdr *sh);

static BOOL
common_symbols(struct ElfObject *eo);

static BOOL
getrelocs(struct ElfObject *eo,struct Elf32_Shdr *sh);

static const char ELFid[4] = {
  0x7f,'E','L','F'
};

void*
ELFLoadObject( const char* objname )
{
  struct ElfObject *elfobj = NULL;

  /* allocate ElfObject structure */

  elfobj = alloc32( sizeof( struct ElfObject ) );

  if( elfobj != NULL )
  {
    memset(elfobj,0,sizeof(struct ElfObject));

    elfobj->header = alloc32( sizeof( struct Elf32_Ehdr ) );

    if( elfobj->header != NULL )
    {
      /* open ELF stream for reading */

      elfobj->handle = opstream( elfobj, objname );

      if( elfobj->handle != NULL )
      {
        /* read and identify ELF 32bit PowerPC BigEndian header */

        if( rdstream( elfobj, elfobj->header, sizeof(struct Elf32_Ehdr) ) )
        {
          struct Elf32_Ehdr *hdr = elfobj->header;

          if (!strncmp(hdr->e_ident,ELFid,4) &&
              hdr->e_ident[EI_CLASS]==ELFCLASS32 &&
              hdr->e_ident[EI_DATA]==ELFDATA2MSB &&
              hdr->e_ident[EI_VERSION]==1 && hdr->e_version==1 &&
              (hdr->e_machine==EM_PPC || hdr->e_machine==EM_PPC_OLD ||
               hdr->e_machine==EM_CYGNUS_POWERPC) && hdr->e_type==ET_REL)
          {
            struct Elf32_Shdr *shdrs;
            ULONG shdrsize = (ULONG) hdr->e_shnum * (ULONG) hdr->e_shentsize;

//            KPrintF("elf32ppcbe format recognized\n");

            shdrs = alloc32( shdrsize );

            if( shdrs != NULL )
            {
              /* read section header table and parse rest of object */

              if( prstream( elfobj, hdr->e_shoff, shdrs, shdrsize ) )
              {
                if( loadelf32( elfobj, shdrs ) )
                {
                  void* start;
//                  KPrintF("ELF object loaded (0x%08lx)\n", elfobj );
                  start = loadprogram( elfobj );
//                  KPrintF("Start of PPC code: 0x%08lx\n", start );
                  free32( shdrs );
                  return (elfobj);
                }
              }
              free32( shdrs );
            }
          }
          else
            KPrintF( "Not an ELF32-PPC-BE\nrelocatable object.");
        }
      }
      freeelfobj(elfobj);
    }
  }

  return (NULL);
}


void
ELFUnLoadObject( void* obj )
{
  struct ElfObject* elfobj = (struct ElfObject*) obj;

  if(elfobj->sections != NULL && elfobj->nsects > 0 )
  {
    freeprogram(elfobj);
  }

  freeelfobj(elfobj);
}

BOOL
ELFGetSymbol( void* obj,
              const char* name, 
              void** ptr )
{
  struct PPCObjectInfo oi =
  {
    0,
    NULL,
    PPCELFINFOTYPE_SYMBOL,
    STT_SECTION,
    STB_GLOBAL,
    0
  };

  BOOL rc;

  oi.Name = (char*) name;
  rc = scanElfSymbols( (struct ElfObject*) obj, &oi, FALSE );

  if( rc )
  {
    *ptr = (void*) oi.Address;
  }

  return rc;
}


static void freeelfobj(struct ElfObject *elfobj)
/* free all memory connected to an ElfObject */
{
  if (elfobj) {
    if (elfobj->handle)
      clstream(elfobj);
    free32(elfobj);
  }
}


static BOOL loadelf32(struct ElfObject *eo,struct Elf32_Shdr *shdrs)
/* parse ELF object, initialize ElfObject structure */
{
  struct Elf32_Ehdr *hdr = eo->header;
  struct ELFSection *s;
  uint16 i;

  if ((eo->secnames = readsection(eo,&shdrs[hdr->e_shstrndx])) &&
      (eo->sections = alloc32((hdr->e_shnum+1)*sizeof(void *)))) {
    memset(eo->sections,0,(hdr->e_shnum+1)*sizeof(void *));
    eo->nsects = hdr->e_shnum + 1;  /* +1 section for COMMON symbols */
    for (i=1; i<hdr->e_shnum; i++) {
      switch (shdrs[i].sh_type) {
        case SHT_PROGBITS:
        case SHT_NOBITS:
          if (!(eo->sections[i] = progsection(eo,&shdrs[i])))
            return (FALSE);
          break;
        case SHT_SYMTAB:
          if (!(eo->symnames = readsection(eo,&shdrs[shdrs[i].sh_link]))
              || !(eo->symtab = readsection(eo,&shdrs[i])))
            return (FALSE);
          eo->nsyms = shdrs[i].sh_size / sizeof(struct Elf32_Sym);
          eo->gsyms = shdrs[i].sh_info;
          break;
        default:
          break;
      }
    }
    for (i=1; i<hdr->e_shnum; i++) {
      switch (shdrs[i].sh_type) {
        case SHT_REL:
        case SHT_RELA:
          if (!getrelocs(eo,&shdrs[i]))
            return (FALSE);
          break;
        default:
          break;
      }
    }
    /* allocate space for Common symbols */
    return (common_symbols(eo));
  }
  return (FALSE);
}


static BOOL scanElfSymbols(struct ElfObject *eo,struct PPCObjectInfo *info,
                           BOOL relmode)
/* Find an ELF symbol by its name or address and return all infos  */
/* in the supplied PPCObjectInfo structure. Return FALSE if symbol */
/* doesn't exist. */
/* ATTENTION: PPCLibBase may be locked at that stage! */
{
  ULONG addr = info->Address;
  char *name = info->Name;
//KPrintF( "scanElfSymbols( 0x%08lx, 0x%08lx, %ld\n", eo, info, relmode );
  if (relmode) {
    unsigned int i;
    int j;
    struct ELFSection *es;
    struct Elf32_Rela *r;

    for (i=1; i<(eo->nsects-1); i++) {
      if( (es = eo->sections[i]) != NULL ) {
        for (j=0,r=es->relocs; j<es->nrel; j++,r++) {
          if (getsyminfo(eo,info,&eo->symtab[ELF32_R_SYM(r->r_info)])) {
            info->Address = (ULONG)es->address + r->r_offset;
            info->Type = PPCELFINFOTYPE_RELOC;
            info->SubType = (ULONG)ELF32_R_TYPE(r->r_info);
            if (info->Address == addr) {
              if (name) {
                if (!strcmp(name,info->Name))
                  return (TRUE);
              }
              else
                return (TRUE);
            }
          }
        }
      }
    }
  }

  else {
    struct Elf32_Sym *stab = eo->symtab;
    int i = eo->nsyms;
    while (--i) {
//KPrintF( "i=%ld\n", i );
      if (getsyminfo(eo,info,++stab)) {
        if (!name) {
          if (info->Size) {
            if (addr>=info->Address && addr<(info->Address+info->Size))
              return (TRUE);
          }
          else {
            if (addr == info->Address)
              return (TRUE);
          }
        }
        else {
//KPrintF( "comparing %s and %s\n", name,info->Name );
          if (!strcmp(name,info->Name))
            return (TRUE);
        }
      }
    }
  }
  return (FALSE);
}


static BOOL getsyminfo(struct ElfObject *eo,struct PPCObjectInfo *info,
                       struct Elf32_Sym *stab)
{
  struct ELFSection *es;
  ULONG subtype;

  subtype = (ULONG)ELF32_ST_TYPE(stab->st_info);
  if (subtype < STT_FILE) {
    info->Type = subtype==STT_SECTION ? PPCELFINFOTYPE_SECTION
                                      : PPCELFINFOTYPE_SYMBOL;
    switch (stab->st_shndx) {
      case SHN_UNDEF:
        /* undefined symbols will disappear after relocation */
      case SHN_ABS:
        info->Address = stab->st_value;
        break;
      case SHN_COMMON:
        /* @@@ common symbols should have disappeared after common_symbols() */
        info->Type = PPCELFINFOTYPE_COMSYMBOL;
        info->Address = 0;
        break;
      default:
        if( (es = eo->sections[stab->st_shndx]) != NULL )
          info->Address = (ULONG)es->address + stab->st_value;
        else
          info->Address = stab->st_value;
        break;
    }
    info->Name = eo->symnames + stab->st_name;
    info->SubType = subtype;
    info->Binding = (ULONG)ELF32_ST_BIND(stab->st_info);
    info->Size = stab->st_size;
    return (TRUE);
  }
  return (FALSE);
}

static APTR loadprogram(struct ElfObject *eo)
/* load all sections into memory and relocate them */
{
  static const char *FN = "loadprogram(): ";
  struct ELFSection *s;
  uint8 *p,*entry=NULL;
  unsigned int i;

  for (i=0; i<(eo->nsects-1); i++) {
    if( (s = eo->sections[i]) != NULL ) {
      BOOL text = !strcmp(s->name,".text");   /* .text section flag */
      ULONG size = s->size;

      /* align to 32 bytes and allocate 32-byte aligned memory */
      size = (size+31)&~31;
      if( (p = alloc32c(size)) != NULL ) {
        s->address = (APTR)p;  /* store section's base address */
        if (!(s->flags & ElfSecF_NOBITS)) {
          /* a PROGBITS section - load it from file */

//          KPrintF("%sreading section %s\n",FN,s->name);
          if (prstream(eo,s->offset,p,s->size)) {
            if (text) {
              /* get start address of PPC program in .text */
              entry = p;
              if ((*entry & 0xfc) == 0) {
//                KPrintF("%sgcc traceback status word "
//                        "detected\n",FN);
                entry += 4;  /* 1st long reserved for gcc traceback word */
              }
              /* copy kernel stubs */
/*
               KPrintF("%sentry=0x%08lx, "
                       "invoking dynamic linker\n",FN,entry);
              if (!dynamic_linker(i,eo,p)) {
                entry = NULL;
                break;
              }
*/
            }
          }
          else {
            entry = NULL;
            break;
          }
        }
      }
      else {
        KPrintF("Failed to allocate %ld bytes\n"
               "for PPC %s section.",size,s->name);
        entry = NULL;
        break;
      }
    }
  }

  if (entry) {
    if (!relocate(eo)) {  /* relocate sections */
      entry = NULL;
      freeprogram(eo);
    }
  }
  else
    freeprogram(eo);

//  KPrintF("%sreturning with entry=0x%08lx\n",FN,entry);
  return (entry);
}


static void freeprogram(struct ElfObject *eo)
{
  struct ELFSection *s;
  unsigned int i;

  for (i=0; i<eo->nsects; i++) {
    if( (s = eo->sections[i]) != NULL ) {
      if (s->address) {
        free32(s->address);
        s->address = NULL;
      }
    }
  }
}


static BOOL relocate(struct ElfObject *eo)
{
  struct ELFSection *es;
  unsigned int shndx;

  for (shndx=0; shndx<(eo->nsects-1); shndx++) {
    if( (es = eo->sections[shndx]) != NULL ) {
      BOOL rela = (es->flags & ElfSecF_RELA) != 0;
      struct Elf32_Rela *r;
      int i;

//      KPrintF("relocate(): relocating section %s "
//              "at 0x%08lx\n",es->name,es->address);
      for (i=0,r=es->relocs; i<es->nrel; i++,r++) {
        struct Elf32_Sym *sym = &eo->symtab[ELF32_R_SYM(r->r_info)];
        long s = (long)eo->sections[sym->st_shndx]->address + sym->st_value;
        uint8 *p = (uint8 *)es->address + r->r_offset;

        switch (ELF32_R_TYPE(r->r_info)) {
          case R_PPC_NONE:
            break;

          case R_PPC_ADDR32:
            if (rela)
              *(long *)p = s + r->r_addend;
            else
              *(long *)p += s;
            break;

          case R_PPC_ADDR16:
            if (rela)
              *(short *)p = s + r->r_addend;
            else
              *(short *)p += s;
            break;

          case R_PPC_ADDR16_LO:
            if (rela)
              *(short *)p = (s + r->r_addend) & 0xffff;
            else
              *(short *)p = (s + *(short *)p) & 0xffff;
            break;

          case R_PPC_ADDR16_HI:
            if (rela)
              *(short *)p = (s + r->r_addend) >> 16;
            else
              *(short *)p = (s + *(short *)p) >> 16;
            break;

          case R_PPC_ADDR16_HA:
            if (rela)
              s += r->r_addend;
            else
              s += *(short *)p;
            *(short *)p = (s>>16) + ((s&0x8000) ? 1 : 0);
            break;

          case R_PPC_REL24:
            if (rela) {
              s = (s + r->r_addend) - (long)p;
            }
            else {
              if (*p & 0x02)
                s = (s + ((*(long *)p & 0x03fffffc) - 0x04000000)) - (long)p;
              else
                s = (s + (*(long *)p & 0x03fffffc)) - (long)p;
            }
            *(unsigned long *)p = (*(unsigned long *)p & 0xfc000003) |
                                  ((unsigned long)s & 0x03fffffc);
            break;

          case R_PPC_REL32:
            if (rela)
              *(long *)p = (s + r->r_addend) - (long)p;
            else
              *(long *)p = (s + *(long *)p) - (long)p;
            break;

          default:
            KPrintF("Relocation type %s\nat %s+%ld referencing\n"
                   "symbol %s+%ld\nis not supported.",
                   reloc_name[ELF32_R_TYPE(r->r_info)],es->name,r->r_offset,
                   eo->symnames+sym->st_name,sym->st_value);
            return (FALSE);
        }
      }
    }
  }
  return (TRUE);
}

static void*
alloc32( size_t size )
{
  return AHIAllocVec( size, MEMF_ANY );
}

static void*
alloc32c( size_t size )
{
  return AHIAllocVec( size, MEMF_ANY | MEMF_CLEAR );
}

void
free32( void* addr )
{
  AHIFreeVec( addr );
}


static void*
allocstr( const char* string )
{
  void* mem;

  mem = AllocVec( strlen( string ) + 1, MEMF_ANY );

  if( mem != NULL )
  {
    strcpy( mem, string );
  }

  return mem;
}


static BPTR
opstream(struct ElfObject *eo, const char* name )
{
  BPTR handle = NULL;

  if( name != NULL )
  {
    handle = Open( (char*) name, MODE_OLDFILE );
  }

  return (handle);
}


static BOOL
clstream(struct ElfObject *eo)
{
  Close( eo->handle );
  return TRUE;
}


static BOOL
rdstream(struct ElfObject *eo,void *buf,long len)
{
  long r;
  
  r = Read( eo->handle, buf, len );

  if( r != len )
  {
    return (FALSE);
  }
  return (TRUE);
}


static long
skstream(struct ElfObject *eo,long offs,long mode)
{
  long r;

  r = Seek( eo->handle, offs, mode );
  return (r);
}


static BOOL
prstream(struct ElfObject *eo,long offs,void *buf,long len)
/* position and read stream */
{
  if (skstream(eo,offs,OFFSET_BEGINNING) != -1)
    return (rdstream(eo,buf,len));
  return FALSE;
}


static void *readsection(struct ElfObject *eo,struct Elf32_Shdr *sh)
/* allocate memory and read section contents */
{
  void *p;

  if( (p = AllocVec(sh->sh_size,MEMF_ANY)) != NULL )
    if (prstream(eo,sh->sh_offset,p,sh->sh_size))
      return (p);
  return (NULL);
}


static struct ELFSection *progsection(struct ElfObject *eo,
                                      struct Elf32_Shdr *sh)
/* Create Section structure from Elf32_Shdr. The contents of these     */
/* sections will be loaded and relocated on demand, e.g. if a new task */
/* is created. */
{
  struct ELFSection *s;

  if( (s = AllocVec(sizeof(struct ELFSection),MEMF_ANY)) != NULL ) {
    memset(s,0,sizeof(struct ELFSection));
    s->name = eo->secnames + sh->sh_name;
    s->flags = sh->sh_type==SHT_NOBITS ? ElfSecF_NOBITS:0;
    s->alignment = (uint8)sh->sh_addralign;
    s->offset = sh->sh_offset;
    s->size = sh->sh_size;
    return (s);
  }
  return (NULL);
}


static BOOL common_symbols(struct ElfObject *eo)
/* Create a and initialize Section structure for Common symbols. */
{
  static const char *FN = "common_symbols(): ";
  static const char *bssname = ".bss";
  struct ELFSection *s = NULL;
  struct Elf32_Sym *sym;
  uint16 *relocp;
  uint32 offset = 0,idx = 0,cnt=0;
  unsigned int i;

  /* First try to find a .bss, where Common symbols could be appended */
  for (i=0; i<(eo->nsects-1); i++) {
    if( (s = eo->sections[i]) != NULL ) {
      if (!strcmp(s->name,bssname) && (s->flags & ElfSecF_NOBITS)) {
        idx = i;
        offset = s->size;
//        KPrintF("%sfound %s at index %ld with size=%ld\n",
//                FN,bssname,idx,offset);
        break;
      }
      else
        s = NULL;
    }
  }

  if (!s) {
    /* No .bss section present, allocate an own one */
    if( (s = AllocVec(sizeof(struct ELFSection),MEMF_ANY)) != NULL ) {
      memset(s,0,sizeof(struct ELFSection));
      s->name = allocstr((char *)bssname);
      s->flags = ElfSecF_NOBITS;
      s->alignment = 32;
      offset = 0;
      idx = eo->nsects-1;
      eo->sections[idx] = s;
//      KPrintF("%screated new %s at index %ld\n",FN,bssname,idx);
    }
    else
      return (FALSE);
  }

  /* Ok, search for COMMON symbols now */
  for (i=1,sym=&eo->symtab[1]; i<eo->nsyms; i++,sym++) {
    if (sym->st_shndx == SHN_COMMON) {
      offset = (offset + sym->st_value-1) & ~(sym->st_value-1);
      sym->st_value = offset;
      sym->st_shndx = idx;
      offset += sym->st_size;
      cnt++;
    }
  }
//  KPrintF("%sassigned %ld common symbols (%ld bytes)\n",
//          FN,cnt,offset-s->size);
  s->size = offset;  /* set new .bss section size */

  return (TRUE);
}


static BOOL getrelocs(struct ElfObject *eo,struct Elf32_Shdr *sh)
/* read relocation entries for a section */
{
  uint32 rsize = sh->sh_entsize;
  int nrelocs = (int)(sh->sh_size/rsize);
  struct ELFSection *s = eo->sections[sh->sh_info];


  s->nrel = nrelocs;
  if (sh->sh_type == SHT_RELA) {
    s->flags |= ElfSecF_RELA;
    if( (s->relocs = readsection(eo,sh)) != NULL )
      return (TRUE);
  }
  else {
    struct Elf32_Rela *r;

    if ((r = s->relocs = AllocVec(nrelocs*sizeof(struct Elf32_Rela),
                                  MEMF_ANY)) &&
        (skstream(eo,sh->sh_offset,OFFSET_BEGINNING) != -1)) {
      while (nrelocs--) {
        r->r_addend = 0;
        if (!rdstream(eo,r,sizeof(struct Elf32_Rel)))
          return (FALSE);
        r++;
      }
      return (TRUE);
    }        
  }
  return (FALSE);
}
