/*
    Copyright © 2013-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: elf.c
    Lang: english
 */

#include "elf.h"
#include "boot.h"

#include <dos/elf.h>
#include <stdlib.h>
#include <string.h>

#define DELF(x) /* x */

uint32_t        int_shnum;
uint32_t        int_shstrndx;

int checkHeader(struct elfheader *eh)
{
	if (eh->ident[0] != 0x7f || eh->ident[1] != 'E'  ||
			eh->ident[2] != 'L'  || eh->ident[3] != 'F')
	{
		return 0;
	}

	int_shnum = eh->shnum;
	int_shstrndx = eh->shstrndx;

	/* the ELF header only uses 16 bits to store the count of section headers,
	 * so it can't handle more than 65535 headers. if the count is 0, and an
	 * offset is defined, then the real count can be found in the first
	 * section header (which always exists).
	 *
	 * similarly, if the string table index is SHN_XINDEX, then the actual
	 * index is found in the first section header also.
	 *
	 * see the System V ABI 2001-04-24 draft for more details.
	 */
	if (int_shnum == 0 || int_shstrndx == SHN_XINDEX)
	{
		if (eh->shoff == 0)
		{
			return 0;
		}

		/* Get section header. I hope it's there, in memory already. */
		struct sheader *sh = (struct sheader *)((intptr_t)eh + eh->shoff);

		/* wider section header count is in the size field */
		if (int_shnum == 0)
			int_shnum = sh->size;

		/* wider string table index is in the link field */
		if (int_shstrndx == SHN_XINDEX)
			int_shstrndx = sh->link;

		/* sanity, if they're still invalid then this isn't elf */
		if (int_shnum == 0 || int_shstrndx == SHN_XINDEX)
		{
			return 0;
		}
	}

	if
	(
			eh->ident[EI_CLASS]   != ELFCLASS32  ||
			eh->ident[EI_VERSION] != EV_CURRENT  ||
			!(eh->type == ET_REL || eh->type == ET_EXEC) ||
#if AROS_BIG_ENDIAN
			eh->ident[EI_DATA]        != ELFDATA2MSB ||
#else
			eh->ident[EI_DATA]        != ELFDATA2LSB ||
#endif
			eh->machine               != EM_ARM
	)
	{
		return 0;
	}

	return 1;
}

int getElfSize(void *elf_file, uint32_t *size_rw, uint32_t *size_ro)
{
	struct elfheader *eh = (struct elfheader *)elf_file;
	uint32_t s_ro = 0;
	uint32_t s_rw = 0;

	DELF(kprintf("[BOOT:ELF] getElfSize(%p)", eh));

	if (checkHeader(eh))
	{
		struct sheader *sh = (struct sheader *)((intptr_t)elf_file + eh->shoff);
		int i;

		for (i = 0; i < int_shnum; i++)
		{
			/* Does the section require memoy allcation? */
			if (sh[i].flags & SHF_ALLOC)
			{
				uint32_t size = (sh[i].size + sh[i].addralign - 1) & ~(sh[i].addralign - 1);

				/*
				 * I extend the section size according to the alignment requirement. However, also the already
				 * measured size has to be aligned poperly. It is so, because the loader has to align the load address later on.
				 */
				if (sh[i].flags & SHF_WRITE)
				{
					s_rw = (s_rw + sh[i].addralign - 1) & ~(sh[i].addralign - 1);
					s_rw += size;
				}
				else
				{
					s_ro = (s_ro + sh[i].addralign - 1) & ~(sh[i].addralign - 1);
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

struct bss_tracker tracker[MAX_BSS_SECTIONS];
static struct bss_tracker *bss_tracker = &tracker[0];

/*
 * read_block function copies the range of memory within ELF file to any specified location.
 */
static inline void read_block(void *file, long offset, void *dest, long length)
{
	memcpy(dest, (void *)((unsigned long)file + offset), length);
}

/*
 * Get the memory for chunk and load it
 */
static int load_hunk(void *file, struct sheader *sh)
{
	void *ptr = (void *)0;

	/* empty chunk? Who cares :) */
	if (!sh->size)
		return 1;

	/* Allocate a chunk with write access */
	if (sh->flags & SHF_WRITE)
	{
		ptr_rw = (((unsigned long)ptr_rw
				+ (unsigned long)sh->addralign - 1)
				& ~((unsigned long)sh->addralign - 1));
		ptr = (APTR)ptr_rw;
		ptr_rw = ptr_rw + sh->size;
	}
	else
	{
		/* Read-Only mode? Get the memory from the kernel space, align it accorting to the demand */
		ptr_ro = (((unsigned long)ptr_ro
				+ (unsigned long)sh->addralign - 1)
				& ~((unsigned long)sh->addralign - 1));
		ptr = (APTR)ptr_ro;
		ptr_ro = ptr_ro + sh->size;
	}

	sh->addr = ptr;

	/* copy block of memory from ELF file if it exists */
	if (sh->type != SHT_NOBITS)
	{
		read_block(file, sh->offset, (void *)((unsigned long)sh->addr),
				sh->size);
	}
	else
	{
		bzero(ptr, sh->size);
		bss_tracker->addr =
				(void *)((unsigned long)ptr + virtoffset);
		bss_tracker->length = sh->size;
		bss_tracker++;
		/*
		 * empty the subsequent tracker in case it's the last one. We did that in case a buggy firmare
		 * forgot to clear our .bss section
		 */
		bss_tracker->addr = (void*)0;
		bss_tracker->length = 0;
	}

	return 1;
}

/* Perform relocations of given section */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx,
		uint32_t virt, uintptr_t *deltas)
{
	struct sheader *shrel = &sh[shrel_idx];
	struct sheader *shsymtab = &sh[shrel->link];
	struct sheader *toreloc = &sh[shrel->info];
	uintptr_t orig_addr = deltas[shrel->info];

	struct symbol *symtab =
			(struct symbol *)((unsigned long)shsymtab->addr);
	struct relo *rel = (struct relo *)((unsigned long)shrel->addr);
	char *section = (char *)((unsigned long)toreloc->addr);

	unsigned int numrel = (unsigned long)shrel->size
			/ (unsigned long)shrel->entsize;
	unsigned int i;

	uint32_t virtoffset;

	struct symbol *SysBase_sym = (void *)0;

	for (i = 0; i < numrel; i++, rel++)
	{
		struct symbol *sym = &symtab[ELF32_R_SYM(rel->info)];
		uint32_t *p = (uint32_t *) & section[rel->offset - orig_addr];
		uint32_t s;
		virtoffset = virt;

		/*
		 * R_ARM_V4BX are actually special marks for the linker.
		 * They even never have a target (shindex == SHN_UNDEF),
		 * so we simply ignore them before doing any checks.
		 */
		if (ELF_R_TYPE(rel->info) == R_ARM_V4BX)
			continue;

		switch (sym->shindex)
		{
		case SHN_UNDEF:
			kprintf
			("[BOOT:ELF] Undefined symbol '%s' in section '%s'\n",
					(char *)((uint32_t) sh[shsymtab->link].addr) +
					sym->name,
					(char *)((uint32_t) sh[eh->shstrndx].addr) +
					toreloc->name);
			return 0;

		case SHN_COMMON:
			kprintf
			("[BOOT:ELF] COMMON symbol '%s' in section '%s'\n",
					(char *)((uint32_t) sh[shsymtab->link].addr) +
					sym->name,
					(char *)((uint32_t) sh[eh->shstrndx].addr) +
					toreloc->name);

			return 0;

		case SHN_ABS:
			if (SysBase_sym == (void *)0) {
				if (strncmp
						((char *)((uint32_t) sh[shsymtab->link].
								addr) + sym->name, "SysBase",
								8) == 0) {
					SysBase_sym = sym;
					goto SysBase_yes;
				} else
					goto SysBase_no;
			} else if (SysBase_sym == sym) {
				SysBase_yes:                    s = (uint32_t) 4UL;
				virtoffset = 0;
			} else
				SysBase_no:                     s = sym->value;
			break;
		default:
			s = (uint32_t) sh[sym->shindex].addr + sym->value;
		}
		switch (ELF32_R_TYPE(rel->info)) {

		//		case R_386_32: /* 32bit direct/absolute */
		//			*p += s + virtoffset;
		//			break;

		case R_ARM_CALL:
		case R_ARM_JUMP24:
		case R_ARM_PC24:
		{
			/* On ARM the 24 bit offset is shifted by 2 to the right */
			signed long offset = (AROS_LE2LONG(*p) & 0x00ffffff) << 2;
			/* If highest bit set, make offset negative */
			if (offset & 0x02000000)
				offset -= 0x04000000;

			offset += s - (ULONG)p;

			offset >>= 2;
			*p &= AROS_LONG2LE(0xff000000);
			*p |= AROS_LONG2LE(offset & 0x00ffffff);
		}
		break;


		case R_ARM_MOVW_ABS_NC:
		case R_ARM_MOVT_ABS:
		{
			signed long offset = AROS_LE2LONG(*p);
			offset = ((offset & 0xf0000) >> 4) | (offset & 0xfff);
			offset = (offset ^ 0x8000) - 0x8000;

			offset += s + virtoffset;

			if (ELF_R_TYPE(rel->info) == R_ARM_MOVT_ABS)
				offset >>= 16;

			*p &= AROS_LONG2LE(0xfff0f000);
			*p |= AROS_LONG2LE(((offset & 0xf000) << 4) | (offset & 0x0fff));
		}
		break;

		case R_ARM_ABS32: /* PC relative 32 bit signed */
		*p += s + virtoffset;
		break;

		case R_ARM_NONE: /* No reloc */
			break;

		default:
			kprintf("[BOOT:ELF] Unknown relocation %d in ELF file\n",
					ELF32_R_TYPE(rel->info));
			return 0;
		}
	}
	return 1;

}

int loadElf(void *elf_file)
{
	struct elfheader *eh = (struct elfheader *)elf_file;
	uintptr_t deltas[int_shnum];
	//uint32_t s_ro = 0;
	//uint32_t s_rw = 0;

	DELF(kprintf("[BOOT] loadElf(%p)\n", eh));

	if (checkHeader(eh))
	{
		struct sheader *sh = (struct sheader *)((intptr_t)elf_file + eh->shoff);
		int i;

		for (i = 0; i < int_shnum; i++)
		{
			/* Load the symbol and string tables */
			if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB)
			{
				sh[i].addr = (APTR)((unsigned long)elf_file + sh[i].offset);
			}
			/* Does the section require memoy allcation? */
			else if (sh[i].flags & SHF_ALLOC)
			{
				deltas[i] = (uintptr_t)sh[i].addr;
				/* Yup, it does. Load the hunk */
				if (!load_hunk(elf_file, &sh[i]))
				{
					return 0;
				}
				else
				{
					if (sh[i].size)
					{
						DELF(kprintf("[BOOT:ELF] %s section loaded at %p (Virtual addr: %p, requestet addr: %p)\n",
								sh[i].flags & SHF_WRITE ? "RW":"RO",
										sh[i].addr,
										sh[i].addr + virtoffset,
										deltas[i]));
					}
				}
			}
		}

		/* For every loaded section perform the relocations */
		for (i = 0; i < int_shnum; i++)
		{
			if (sh[i].type == SHT_REL && sh[sh[i].info].addr)
			{
				sh[i].addr = (APTR)((uint32_t) elf_file + sh[i].offset);
				if (!sh[i].addr || !relocate(eh, sh, i, virtoffset, deltas))
				{
					return 0;
				}
			}
		}
	}
    return 1;
}
