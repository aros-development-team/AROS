/* dl.c - arch-dependent part of loadable module support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2004,2005,2007,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/dl.h>
#include <grub/elf.h>
#include <grub/misc.h>
#include <grub/err.h>

/* Check if EHDR is a valid ELF header.  */
grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2MSB
      || e->e_machine != EM_PPC)
    return grub_error (GRUB_ERR_BAD_OS, "invalid arch specific ELF magic");

  return GRUB_ERR_NONE;
}


/* Relocate symbols.  */
grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  Elf_Ehdr *e = ehdr;
  Elf_Shdr *s;
  Elf_Word entsize;
  unsigned i;

  /* Find a symbol table.  */
  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return grub_error (GRUB_ERR_BAD_MODULE, "no symtab found");

  entsize = s->sh_entsize;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_RELA)
      {
	grub_dl_segment_t seg;

	/* Find the target segment.  */
	for (seg = mod->segment; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    Elf_Rela *rel, *max;

	    for (rel = (Elf_Rela *) ((char *) e + s->sh_offset),
		   max = rel + s->sh_size / s->sh_entsize;
		 rel < max;
		 rel++)
	      {
		Elf_Word *addr;
		Elf_Sym *sym;
		grub_uint32_t value;

		if (seg->size < rel->r_offset)
		  return grub_error (GRUB_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");

		addr = (Elf_Word *) ((char *) seg->addr + rel->r_offset);
		sym = (Elf_Sym *) ((char *) mod->symtab
				     + entsize * ELF_R_SYM (rel->r_info));

		/* On the PPC the value does not have an explicit
		   addend, add it.  */
		value = sym->st_value + rel->r_addend;
		switch (ELF_R_TYPE (rel->r_info))
		  {
		  case R_PPC_ADDR16_LO:
		    *(Elf_Half *) addr = value;
		    break;

		  case R_PPC_REL24:
		    {
		      Elf_Sword delta = value - (Elf_Word) addr;

		      if (delta << 6 >> 6 != delta)
			return grub_error (GRUB_ERR_BAD_MODULE, "relocation overflow");
		      *addr = (*addr & 0xfc000003) | (delta & 0x3fffffc);
		      break;
		    }

		  case R_PPC_ADDR16_HA:
		    *(Elf_Half *) addr = (value + 0x8000) >> 16;
		    break;

		  case R_PPC_ADDR32:
		    *addr = value;
		    break;

		  case R_PPC_REL32:
		    *addr = value - (Elf_Word) addr;
		    break;

		  default:
		    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
				       "this relocation (%d) is not implemented yet",
				       ELF_R_TYPE (rel->r_info));
		  }
	      }
	  }
      }

  return GRUB_ERR_NONE;
}
