/* dl-x86_64.c - arch-dependent part of loadable module support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2007,2009  Free Software Foundation, Inc.
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
#include <grub/i18n.h>

/* Check if EHDR is a valid ELF header.  */
grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  Elf64_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != EM_X86_64)
    return grub_error (GRUB_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  return GRUB_ERR_NONE;
}

/* Relocate symbols.  */
grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  Elf64_Ehdr *e = ehdr;
  Elf64_Shdr *s;
  Elf64_Word entsize;
  unsigned i;

  /* Find a symbol table.  */
  for (i = 0, s = (Elf64_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf64_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return grub_error (GRUB_ERR_BAD_MODULE, N_("no symbol table"));

  entsize = s->sh_entsize;

  for (i = 0, s = (Elf64_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf64_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_RELA)
      {
	grub_dl_segment_t seg;

	/* Find the target segment.  */
	for (seg = mod->segment; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    Elf64_Rela *rel, *max;

	    for (rel = (Elf64_Rela *) ((char *) e + s->sh_offset),
		   max = rel + s->sh_size / s->sh_entsize;
		 rel < max;
		 rel++)
	      {
		Elf64_Word *addr32;
		Elf64_Xword *addr64;
		Elf64_Sym *sym;

		if (seg->size < rel->r_offset)
		  return grub_error (GRUB_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");

		addr32 = (Elf64_Word *) ((char *) seg->addr + rel->r_offset);
		addr64 = (Elf64_Xword *) addr32;
		sym = (Elf64_Sym *) ((char *) mod->symtab
				     + entsize * ELF_R_SYM (rel->r_info));

		switch (ELF_R_TYPE (rel->r_info))
		  {
		  case R_X86_64_64:
		    *addr64 += rel->r_addend + sym->st_value;
		    break;

		  case R_X86_64_PC32:
		    *addr32 += rel->r_addend + sym->st_value -
		              (Elf64_Xword) seg->addr - rel->r_offset;
		    break;

                  case R_X86_64_32:
                  case R_X86_64_32S:
                    *addr32 += rel->r_addend + sym->st_value;
                    break;

		  default:
		    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
				       N_("relocation 0x%x is not implemented yet"),
				       ELF_R_TYPE (rel->r_info));
		  }
	      }
	  }
      }

  return GRUB_ERR_NONE;
}
