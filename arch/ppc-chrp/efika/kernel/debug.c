/*
 * debug.c
 *
 *  Created on: Aug 25, 2008
 *      Author: misc
 */

#define PCIC0_IO 0

#include <asm/io.h>
#include <aros/libcall.h>
#include <stdarg.h>
#include <string.h>
#include <proto/exec.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include "kernel_intern.h"

void __putc(uint8_t chr)
{
	if (chr == '\n')
		__putc('\r');

	while (!(inw(0xf0002004) & 0x400));
	outb(chr, 0xf0002080);
	while (!(inw(0xf0002004) & 0x400));
}


AROS_LH2(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 12, Kernel)
{
    AROS_LIBFUNC_INIT

#if 1
    int result;

    result = __vcformat(NULL, __putc, format, args);

    return result;
#endif

    AROS_LIBFUNC_EXIT
}







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

AROS_LH3(void, KrnRegisterModule,
		AROS_LHA(const char *, 		name, A0),
		AROS_LHA(struct sheader *,	sections, A1),
		AROS_LHA(struct elfheader *,eh, A2),
		struct KernelBase *, KernelBase, 22, Kernel)
{
	AROS_LIBFUNC_INIT

	struct ExecBase *SysBase = getSysBase();

	if (name && sections && eh)
	{
		module_t *mod = AllocVec(sizeof(module_t), MEMF_PUBLIC | MEMF_CLEAR);

		if (mod)
		{
			int i;

//			D(bug("[KRN] KrnRegisterModule('%s', %08x, %d)\n", name, sections, eh->int_shnum));

			mod->m_name = AllocVec(strlen(name)+1, MEMF_PUBLIC | MEMF_CLEAR);
			strcpy(mod->m_name, name);

			NEWLIST(&mod->m_symbols);

			mod->m_lowest = 0xffffffff;
			mod->m_highest = 0;

			for (i=0; i < eh->int_shnum; i++)
			{
				/* If we have string table, copy it */
				if (sections[i].type == SHT_STRTAB && i != SHINDEX(eh->int_shstrndx))
				{
//					D(bug("[KRN]  symbol table of length %d in section %d\n", sections[i].size, i));

					if (!mod->m_str)
					{
						mod->m_str = AllocVec(sections[i].size, MEMF_PUBLIC | MEMF_CLEAR);
						CopyMem(sections[i].addr, mod->m_str, sections[i].size);
					}
				}

				if ((sections[i].flags & (SHF_ALLOC | SHF_EXECINSTR)) == (SHF_ALLOC | SHF_EXECINSTR))
				{
					if (sections[i].addr)
					{
						if (sections[i].addr < mod->m_lowest)
							mod->m_lowest = sections[i].addr;
						if (sections[i].addr + sections[i].size > mod->m_highest)
							mod->m_highest = sections[i].addr + sections[i].size;
					}
				}
			}

			for (i=0; i < eh->int_shnum; i++)
			{
				if (sections[i].addr && sections[i].type == SHT_SYMTAB)
				{
					int j;
					struct symbol *st = (struct symbol *)sections[i].addr;

					for (j=0; j < (sections[i].size / sizeof(struct symbol)); j++)
					{
						if (st[j].shindex != SHN_XINDEX)
						{
							if (sections[st[j].shindex].addr && (sections[st[j].shindex].flags & (SHF_ALLOC | SHF_EXECINSTR)) == (SHF_ALLOC | SHF_EXECINSTR))
							{
								symbol_t *sym = AllocVec(sizeof(symbol_t), MEMF_PUBLIC | MEMF_CLEAR);
								sym->s_name = &mod->m_str[st[j].name];
								sym->s_lowest = sections[st[j].shindex].addr + st[j].value;
								sym->s_highest = sym->s_lowest + st[j].size;

					//			D(bug("[KRN]  Adding symbol '%s' %08x-%08x\n", sym->s_name, sym->s_lowest, sym->s_highest-1));

								AddHead(&mod->m_symbols, sym);
							}
						}
					}

					break;
				}
			}

//			D(bug("[KRN]  address range %08x - %08x\n", mod->m_lowest, mod->m_highest-1));



			AddHead(&KernelBase->kb_Modules, mod);
		}
	}

	AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnUnregisterModule,
		AROS_LHA(void *,		address, A0),
		struct KernelBase *, KernelBase, 23, Kernel)
{
	AROS_LIBFUNC_INIT

	struct ExecBase *SysBase = getSysBase();
	module_t *mod;
	intptr_t addr = (intptr_t)address;

	ForeachNode(&KernelBase->kb_Modules, mod)
	{
		if (mod->m_lowest <= addr && mod->m_highest > addr)
		{
			symbol_t *sym;

			Remove(mod);

			FreeVec(mod->m_str);
			FreeVec(mod->m_name);

			while(sym = RemHead(&mod->m_symbols))
			{
				FreeVec(sym);
			}

			FreeVec(mod);

			break;
		}
	}

	AROS_LIBFUNC_EXIT
}

extern module_t *modlist;
extern uint32_t modlength;

uint32_t findNames(intptr_t addr, char **module, char **function)
{
	struct KernelBase *KernelBase = getKernelBase();
	module_t *mod;
	symbol_t *sym;
	uint32_t offset = 0;

	*module = NULL;
	*function = NULL;

	ForeachNode(&KernelBase->kb_Modules, mod)
	{
		/* if address suits the module bounds, you got it */
		if (mod->m_lowest <= addr && mod->m_highest > addr)
		{
			*module = mod->m_name;
			offset = addr - mod->m_lowest;

			ForeachNode(&mod->m_symbols, sym)
			{
				if (sym->s_lowest <= addr && sym->s_highest > addr)
				{
					offset = addr - sym->s_lowest;
					*function = sym->s_name;
					break;
				}
			}

			break;
		}
	}

	/* module unset? then look through the kernel list */
	if (*module == NULL)
	{
		int i;
		for (i=0; i < modlength; i++)
		{
			if (modlist[i].m_lowest <= addr && modlist[i].m_highest > addr)
			{
				*module = modlist[i].m_name;
				offset = addr - modlist[i].m_lowest;

				ForeachNode(&modlist[i].m_symbols, sym)
				{
					if (sym->s_lowest <= addr && sym->s_highest > addr)
					{
						offset = addr - sym->s_lowest;
						*function = sym->s_name;
						break;
					}
				}

				break;
			}
		}
	}

	return offset;
}
