/* dl-386.c - arch-dependent part of loadable module support */
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
#include <grub/cpu/types.h>
#include <grub/mm.h>
#include <grub/i18n.h>

/* Dummy __gnu_local_gp. Resolved by linker.  */
static char __gnu_local_gp_dummy;

/* Check if EHDR is a valid ELF header.  */
grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
#ifdef GRUB_CPU_WORDS_BIGENDIAN
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2MSB
      || e->e_machine != EM_MIPS)
#else
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != EM_MIPS)
#endif
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
  grub_size_t gp_size = 0;
  /* FIXME: suboptimal.  */
  grub_uint32_t *gp, *gpptr;
  grub_uint32_t gp0;

  /* Find a symbol table.  */
  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return grub_error (GRUB_ERR_BAD_MODULE, N_("no symbol table"));

  entsize = s->sh_entsize;

  /* Find reginfo. */
  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_MIPS_REGINFO)
      break;

  if (i == e->e_shnum)
    return grub_error (GRUB_ERR_BAD_MODULE, "no reginfo found");

  gp0 = ((grub_uint32_t *)((char *) e + s->sh_offset))[5];

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_REL)
      {
	grub_dl_segment_t seg;

	/* Find the target segment.  */
	for (seg = mod->segment; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    Elf_Rel *rel, *max;

	    for (rel = (Elf_Rel *) ((char *) e + s->sh_offset),
		   max = rel + s->sh_size / s->sh_entsize;
		 rel < max;
		 rel++)
		switch (ELF_R_TYPE (rel->r_info))
		  {
		  case R_MIPS_GOT16:
		  case R_MIPS_CALL16:
		  case R_MIPS_GPREL32:
		    gp_size += 4;
		    break;
		  }
	  }
      }

  if (gp_size > 0x08000)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "__gnu_local_gp is too big\n");

  gpptr = gp = grub_malloc (gp_size);
  if (!gp)
    return grub_errno;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_REL)
      {
	grub_dl_segment_t seg;

	/* Find the target segment.  */
	for (seg = mod->segment; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    Elf_Rel *rel, *max;

	    for (rel = (Elf_Rel *) ((char *) e + s->sh_offset),
		   max = rel + s->sh_size / s->sh_entsize;
		 rel < max;
		 rel++)
	      {
		grub_uint8_t *addr;
		Elf_Sym *sym;

		if (seg->size < rel->r_offset)
		  return grub_error (GRUB_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");

		addr = (grub_uint8_t *) ((char *) seg->addr + rel->r_offset);
		sym = (Elf_Sym *) ((char *) mod->symtab
				     + entsize * ELF_R_SYM (rel->r_info));
		if (sym->st_value == (grub_addr_t) &__gnu_local_gp_dummy)
		  sym->st_value = (grub_addr_t) gp;

		switch (ELF_R_TYPE (rel->r_info))
		  {
		  case R_MIPS_HI16:
		    {
		      grub_uint32_t value;
		      Elf_Rel *rel2;

#ifdef GRUB_CPU_WORDS_BIGENDIAN
		      addr += 2;
#endif

		      /* Handle partner lo16 relocation. Lower part is
			 treated as signed. Hence add 0x8000 to compensate. 
		       */
		      value = (*(grub_uint16_t *) addr << 16)
			+ sym->st_value + 0x8000;
		      for (rel2 = rel + 1; rel2 < max; rel2++)
			if (ELF_R_SYM (rel2->r_info)
			    == ELF_R_SYM (rel->r_info)
			    && ELF_R_TYPE (rel2->r_info) == R_MIPS_LO16)
			  {
			    value += *(grub_int16_t *)
			      ((char *) seg->addr + rel2->r_offset
#ifdef GRUB_CPU_WORDS_BIGENDIAN
			       + 2
#endif
			       );
			    break;
			  }
		      *(grub_uint16_t *) addr = (value >> 16) & 0xffff;
		    }
		    break;
		  case R_MIPS_LO16:
#ifdef GRUB_CPU_WORDS_BIGENDIAN
		    addr += 2;
#endif
		    *(grub_uint16_t *) addr += (sym->st_value) & 0xffff;
		    break;
		  case R_MIPS_32:
		    *(grub_uint32_t *) addr += sym->st_value;
		    break;
		  case R_MIPS_GPREL32:
		    *(grub_uint32_t *) addr = sym->st_value
		      + *(grub_uint32_t *) addr + gp0 - (grub_uint32_t)gp;
		    break;

		  case R_MIPS_26:
		    {
		      grub_uint32_t value;
		      grub_uint32_t raw;
		      raw = (*(grub_uint32_t *) addr) & 0x3ffffff;
		      value = raw << 2;
		      value += sym->st_value;
		      raw = (value >> 2) & 0x3ffffff;
			
		      *(grub_uint32_t *) addr = 
			raw | ((*(grub_uint32_t *) addr) & 0xfc000000);
		    }
		    break;
		  case R_MIPS_GOT16:
		  case R_MIPS_CALL16:
		    /* FIXME: reuse*/
#ifdef GRUB_CPU_WORDS_BIGENDIAN
		    addr += 2;
#endif
		    *gpptr = sym->st_value + *(grub_uint16_t *) addr;
		    *(grub_uint16_t *) addr
		      = sizeof (grub_uint32_t) * (gpptr - gp);
		    gpptr++;
		    break;
		  case R_MIPS_JALR:
		    break;
		  default:
		    {
		      grub_free (gp);
		      return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
					 N_("relocation 0x%x is not implemented yet"),
					 ELF_R_TYPE (rel->r_info));
		    }
		    break;
		  }
	      }
	  }
      }

  return GRUB_ERR_NONE;
}

void 
grub_arch_dl_init_linker (void)
{
  grub_dl_register_symbol ("__gnu_local_gp", &__gnu_local_gp_dummy, 0, 0);
}

