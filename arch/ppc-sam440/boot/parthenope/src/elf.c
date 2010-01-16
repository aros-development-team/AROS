/*
 * $Id: elf.c 49 2008-03-19 23:27:28Z michalsc $
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

#define DEBUG 1
#include "elf.h"
#include "support.h"

static char *ptr_ro = (char *)KERNEL_PHYS_BASE;
static char *ptr_rw = (char *)KERNEL_PHYS_BASE;

char *get_ptr_ro()
{
	return ptr_ro;
}

char *get_ptr_rw()
{
	return ptr_rw;
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
 * Test for correct ELF header here
 */
static int check_header(struct elfheader *eh)
{
	if (eh->ident[0] != 0x7f ||
	    eh->ident[1] != 'E' || eh->ident[2] != 'L' || eh->ident[3] != 'F') {
		return 0;
	}

	if (eh->type != ET_REL || eh->machine != EM_PPC) {
		return 0;
	}
	return 1;
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

	/* Allocate a chunk with write access - take aligned memory beneath the RO kernel */
	if (sh->flags & SHF_WRITE) {
		ptr = (void *)(((unsigned long)ptr_rw - (unsigned long)sh->size
				- (unsigned long)sh->addralign + 1)
			       & ~((unsigned long)sh->addralign - 1));
		ptr_rw = ptr;
	} else {
		/* Read-Only mode? Get the memory from the kernel space, align it accorting to the demand */
		ptr_ro = (char *)(((unsigned long)ptr_ro
				   + (unsigned long)sh->addralign - 1)
				  & ~((unsigned long)sh->addralign - 1));
		ptr = ptr_ro;
		ptr_ro = ptr_ro + sh->size;
	}

	sh->addr = (long)ptr;

	/* copy block of memory from ELF file if it exists */
	if (sh->type != SHT_NOBITS) {
		read_block(file, sh->offset, (void *)((unsigned long)sh->addr),
			   sh->size);
	} else {
		bzero(ptr, sh->size);
		bss_tracker->addr =
		    (void *)((unsigned long)ptr + KERNEL_VIRT_BASE -
			     KERNEL_PHYS_BASE);
		bss_tracker->length = sh->size;
		bss_tracker++;
	}

	return 1;
}

/* Perform relocations of given section */
static int relocate(struct elfheader *eh, struct sheader *sh, long shrel_idx,
		    uint32_t virt)
{
	struct sheader *shrel = &sh[shrel_idx];
	struct sheader *shsymtab = &sh[shrel->link];
	struct sheader *toreloc = &sh[shrel->info];

	struct symbol *symtab =
	    (struct symbol *)((unsigned long)shsymtab->addr);
	struct relo *rel = (struct relo *)((unsigned long)shrel->addr);
	char *section = (char *)((unsigned long)toreloc->addr);

	unsigned int numrel = (unsigned long)shrel->size
	    / (unsigned long)shrel->entsize;
	unsigned int i;

	uint32_t virtoffset;

	struct symbol *SysBase_sym = (void *)0;

	for (i = 0; i < numrel; i++, rel++) {
		struct symbol *sym = &symtab[ELF32_R_SYM(rel->info)];
		uint32_t *p = (uint32_t *) & section[rel->offset];
		uint32_t s;
		virtoffset = virt;

		switch (sym->shindex) {
		case SHN_UNDEF:
			printf
			    ("[ELF Loader] Undefined symbol '%s' in section '%s'\n",
			     (char *)((uint32_t) sh[shsymtab->link].addr) +
			     sym->name,
			     (char *)((uint32_t) sh[eh->shstrndx].addr) +
			     toreloc->name);
			return 0;

		case SHN_COMMON:
			printf
			    ("[ELF Loader] COMMON symbol '%s' in section '%s'\n",
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
SysBase_yes:			s = (uint32_t) 4UL;
				virtoffset = 0;
			} else
SysBase_no:			s = sym->value;
			break;
		default:
			s = (uint32_t) sh[sym->shindex].addr + sym->value;
		}

		switch (ELF32_R_TYPE(rel->info)) {
		case R_PPC_ADDR32:
			*p = s + rel->addend + virtoffset;
			break;

		case R_PPC_ADDR16_LO:
			{
				unsigned short *c = (unsigned short *)p;
				*c = (s + rel->addend + virtoffset) & 0xffff;
			}
			break;

		case R_PPC_ADDR16_HA:
			{
				unsigned short *c = (unsigned short *)p;
				uint32_t temp = s + rel->addend + virtoffset;
				*c = temp >> 16;
				if ((temp & 0x8000) != 0)
					(*c)++;
			}
			break;

		case R_PPC_REL24:
			*p &= ~0x3fffffc;
			*p |= (s + rel->addend - (uint32_t) p) & 0x3fffffc;
			break;

		case R_PPC_REL32:
			*p = s + rel->addend - (uint32_t) p;
			break;

		case R_PPC_NONE:
			break;

		default:
			printf("[BOOT] Unknown relocation %d in ELF file\n",
			       ELF32_R_TYPE(rel->info));
			return 0;
		}
	}
	return 1;
}

int load_elf_file(void *file)
{
	struct elfheader *eh;
	struct sheader *sh;
	long i;
	int shown = 0;

	eh = file;

	/* Check the header of ELF file */
	if (!check_header(eh)) {
		printf("[BOOT] ELF header invalid\n");
		return 0;
	}

	sh = (struct sheader *)((unsigned long)file + eh->shoff);

	/* Iterate over the section header in order to prepare memory and eventually load some hunks */
	for (i = 0; i < eh->shnum; i++) {
		/* Load the symbol and string tables */
		if (sh[i].type == SHT_SYMTAB || sh[i].type == SHT_STRTAB) {
			sh[i].addr = (unsigned long)file + sh[i].offset;
		}
		/* Does the section require memoy allcation? */
		else if (sh[i].flags & SHF_ALLOC) {
			/* Yup, it does. Load the hunk */
			if (!load_hunk(file, &sh[i])) {
				return 0;
			} else {
				if (sh[i].size && !shown) {
					printf
					    ("[BOOT] ELF: section loaded at %p (Virtual addr: %p)\n",
					     sh[i].addr,
					     sh[i].addr + KERNEL_VIRT_BASE -
					     KERNEL_PHYS_BASE);
					shown = 1;
				}
			}
		}
	}

	/* For every loaded section perform the relocations */
	for (i = 0; i < eh->shnum; i++) {
		if (sh[i].type == SHT_RELA && sh[sh[i].info].addr) {
			sh[i].addr = (uint32_t) file + sh[i].offset;
			if (!sh[i].addr
			    || !relocate(eh, sh, i,
					 KERNEL_VIRT_BASE - KERNEL_PHYS_BASE)) {
				return 0;
			}
		}
	}
	return 1;
}
