/* dl-386.c - arch-dependent part of loadable module support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/dl.h>
#include <grub/elf.h>
#include <grub/misc.h>
#include <grub/err.h>

/* Check if EHDR is a valid ELF header.  */
grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  Elf32_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != EM_386)
    return grub_error (GRUB_ERR_BAD_OS, "invalid arch specific ELF magic");

  return GRUB_ERR_NONE;
}

/* Relocate symbols.  */
grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  Elf32_Ehdr *e = ehdr;
  Elf32_Shdr *s;
  Elf32_Sym *symtab;
  Elf32_Word entsize;
  unsigned i;

  /* Find a symbol table.  */
  for (i = 0, s = (Elf32_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf32_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return grub_error (GRUB_ERR_BAD_MODULE, "no symtab found");
  
  symtab = (Elf32_Sym *) ((char *) e + s->sh_offset);
  entsize = s->sh_entsize;

  for (i = 0, s = (Elf32_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf32_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_REL)
      {
	grub_dl_segment_t seg;

	/* Find the target segment.  */
	for (seg = mod->segment; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    Elf32_Rel *rel, *max;
	    
	    for (rel = (Elf32_Rel *) ((char *) e + s->sh_offset),
		   max = rel + s->sh_size / s->sh_entsize;
		 rel < max;
		 rel++)
	      {
		Elf32_Word *addr;
		Elf32_Sym *sym;
		
		if (seg->size < rel->r_offset)
		  return grub_error (GRUB_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");
		
		addr = (Elf32_Word *) ((char *) seg->addr + rel->r_offset);
		sym = (Elf32_Sym *) ((char *) symtab
				     + entsize * ELF32_R_SYM (rel->r_info));
		
		switch (ELF32_R_TYPE (rel->r_info))
		  {
		  case R_386_32:
		    *addr += sym->st_value;
		    break;

		  case R_386_PC32:
		    *addr += (sym->st_value - (Elf32_Word) seg->addr
			      - rel->r_offset);
		    break;
		  }
	      }
	  }
      }

  return GRUB_ERR_NONE;
}
