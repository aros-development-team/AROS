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
#include <grub/i18n.h>

/* Check if EHDR is a valid ELF header.  */
grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2MSB
      || e->e_machine != EM_SPARCV9)
    return grub_error (GRUB_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  return GRUB_ERR_NONE;
}

#pragma GCC diagnostic ignored "-Wcast-align"

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
    return grub_error (GRUB_ERR_BAD_MODULE, N_("no symbol table"));

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
		Elf_Addr value;

		if (seg->size < rel->r_offset)
		  return grub_error (GRUB_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");

		addr = (Elf_Word *) ((char *) seg->addr + rel->r_offset);
		sym = (Elf_Sym *) ((char *) mod->symtab
				     + entsize * ELF_R_SYM (rel->r_info));

		value = sym->st_value + rel->r_addend;
		switch (ELF_R_TYPE (rel->r_info) & 0xff)
		  {
                  case R_SPARC_32: /* 3 V-word32 */
                    if (value & 0xFFFFFFFF00000000)
                      return grub_error (GRUB_ERR_BAD_MODULE,
                                         "address out of 32 bits range");
                    *addr = value;
                    break;
                  case R_SPARC_WDISP30: /* 7 V-disp30 */
                    if (((value - (Elf_Addr) addr) & 0xFFFFFFFF00000000) &&
                        (((value - (Elf_Addr) addr) & 0xFFFFFFFF00000000)
			 != 0xFFFFFFFF00000000))
                      return grub_error (GRUB_ERR_BAD_MODULE,
                                         "displacement out of 30 bits range");
                    *addr = (*addr & 0xC0000000) |
                      (((grub_int32_t) ((value - (Elf_Addr) addr) >> 2)) &
                       0x3FFFFFFF);
                    break;
                  case R_SPARC_HH22: /* 9 V-imm22 */
                    *addr = (*addr & 0xFFC00000) | ((value >> 42) & 0x3FFFFF);
                    break;
                  case R_SPARC_HM10: /* 12 T-simm13 */
                    *addr = (*addr & 0xFFFFFC00) | ((value >> 32) & 0x3FF);
                    break;
                  case R_SPARC_HI22: /* 9 V-imm22 */
                    *addr = (*addr & 0xFFC00000) | ((value >> 10) & 0x3FFFFF);
                    break;
                  case R_SPARC_LO10: /* 12 T-simm13 */
                    *addr = (*addr & 0xFFFFFC00) | (value & 0x3FF);
                    break;
                  case R_SPARC_64: /* 32 V-xwords64 */
                    *(Elf_Xword *) addr = value;
                    break;
		  case R_SPARC_OLO10:
		    *addr = (*addr & ~0x1fff)
		      | (((value & 0x3ff) +
			  (ELF_R_TYPE (rel->r_info) >> 8))
			 & 0x1fff);
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
