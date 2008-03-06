/* dl.c - arch-dependent part of loadable module support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2004,2005,2007  Free Software Foundation, Inc.
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
  Elf64_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2MSB
      || e->e_machine != EM_SPARCV9)
    return grub_error (GRUB_ERR_BAD_OS, "invalid arch specific ELF magic");

  return GRUB_ERR_NONE;
}


/* Relocate symbols.  */
grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  Elf64_Ehdr *e = ehdr;
  Elf64_Shdr *s;
  Elf64_Sym *symtab;
  Elf64_Word entsize;
  unsigned i;
  
  /* Find a symbol table.  */
  for (i = 0, s = (Elf64_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf64_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return grub_error (GRUB_ERR_BAD_MODULE, "no symtab found");
  
  symtab = (Elf64_Sym *) ((char *) e + s->sh_offset);
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
		Elf64_Word *addr;
		Elf64_Sym *sym;
		Elf64_Addr value;
		
		if (seg->size < rel->r_offset)
		  return grub_error (GRUB_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");
		
		addr = (Elf64_Word *) ((char *) seg->addr + rel->r_offset);
		sym = (Elf64_Sym *) ((char *) symtab
				     + entsize * ELF64_R_SYM (rel->r_info));

		value = sym->st_value + rel->r_addend;
		switch (ELF64_R_TYPE (rel->r_info))
		  {
                  case R_SPARC_32: /* 3 V-word32 */
                    if (value & 0xFFFFFFFF00000000)
                      return grub_error (GRUB_ERR_BAD_MODULE,
                                         "Address out of 32 bits range");
                    *addr = value;
                    break;
                  case R_SPARC_WDISP30: /* 7 V-disp30 */
                    if (((value - (Elf64_Addr) addr) & 0xFFFFFFFF00000000) &&
                        ((value - (Elf64_Addr) addr) & 0xFFFFFFFF00000000
                        != 0xFFFFFFFF00000000))
                      return grub_error (GRUB_ERR_BAD_MODULE,
                                         "Displacement out of 30 bits range");
                    *addr = (*addr & 0xC0000000) |
                      (((grub_int32_t) ((value - (Elf64_Addr) addr) >> 2)) &
                       0x3FFFFFFF);
                    break;
                  case R_SPARC_HI22: /* 9 V-imm22 */
                    if (((grub_int32_t) value) & 0xFF00000000)
                      return grub_error (GRUB_ERR_BAD_MODULE,
                                         "High address out of 22 bits range");
                    *addr = (*addr & 0xFFC00000) | ((value >> 10) & 0x3FFFFF);
                    break;
                  case R_SPARC_LO10: /* 12 T-simm13 */
                    *addr = (*addr & 0xFFFFFC00) | (value & 0x3FF);
                    break;
                  case R_SPARC_64: /* 32 V-xwords64 */
                    *(Elf64_Xword *) addr = value;
                    break;
		  default:
		    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
				       "This relocation (%d) is not implemented yet",
				       ELF64_R_TYPE (rel->r_info));
		  }
	      }
	  }
      }
  
  return GRUB_ERR_NONE;
}
