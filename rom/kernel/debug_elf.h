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
#define R_PPC_REL32     26
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

#define ELF32_R_SYM(val)        ((val) >> 8)
#define ELF32_R_TYPE(val)       ((val) & 0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))


struct elfheader
{
	UBYTE ident[16];
	UWORD type;
	UWORD machine;
	ULONG version;
	APTR  entry;
	ULONG phoff;
	ULONG shoff;
	ULONG flags;
	UWORD ehsize;
	UWORD phentsize;
	UWORD phnum;
	UWORD shentsize;
	UWORD shnum;
	UWORD shstrndx;

	/* these are internal, and not part of the header proper. they are wider
	 * versions of shnum and shstrndx for when they don't fit in the header
	 * and we need to get them from the first section header. see
	 * load_header() for details
	 */
	ULONG int_shnum;
	ULONG int_shstrndx;
};

struct sheader
{
	ULONG name;
	ULONG type;
	ULONG flags;
	APTR  addr;
	ULONG offset;
	ULONG size;
	ULONG link;
	ULONG info;
	ULONG addralign;
	ULONG entsize;
};

struct symbol
{
	ULONG name;     /* Offset of the name string in the string table */
	ULONG value;    /* Varies; eg. the offset of the symbol in its hunk */
	ULONG size;     /* How much memory does the symbol occupy */
	UBYTE info;     /* What kind of symbol is this ? (global, variable, etc) */
	UBYTE other;    /* undefined */
	UWORD shindex;  /* In which section is the symbol defined ? */
};

struct relo
{
	ULONG offset;   /* Address of the relocation relative to the section it refers to */
	ULONG info;     /* Type of the relocation */
#if defined(__mc68000__) || defined (__x86_64__) || defined (__ppc__) || defined (__powerpc__) || defined(__arm__)
	LONG  addend;   /* Constant addend used to compute value */
#endif
};

struct hunk
{
	ULONG size;
	BPTR  next;
	char  data[0];
} __attribute__((packed));

#define BPTR2HUNK(bptr) ((struct hunk *)((char *)BADDR(bptr) - offsetof(struct hunk, next)))
#define HUNK2BPTR(hunk) MKBADDR(&hunk->next)

/* convert section header number to array index */
#define SHINDEX(n) \
		((n) < SHN_LORESERVE ? (n) : ((n) <= SHN_HIRESERVE ? 0 : (n) - (SHN_HIRESERVE + 1 - SHN_LORESERVE)))

/* convert section header array index to section number */
#define SHNUM(i) \
		((i) < SHN_LORESERVE ? (i) : (i) + (SHN_HIRESERVE + 1 - SHN_LORESERVE))
