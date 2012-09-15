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
#include <grub/mm.h>
#include <grub/i18n.h>

/* Check if EHDR is a valid ELF header.  */
grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != EM_IA_64)
    return grub_error (GRUB_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  return GRUB_ERR_NONE;
}

#pragma GCC diagnostic ignored "-Wcast-align"

#define MASK20 ((1 << 20) - 1)
#define MASK19 ((1 << 19) - 1)

struct unaligned_uint32
{
  grub_uint32_t val;
}  __attribute__ ((packed));

static void
add_value_to_slot_20b (grub_addr_t addr, grub_uint32_t value)
{
  struct unaligned_uint32 *p;
  switch (addr & 3)
    {
    case 0:
      p = (struct unaligned_uint32 *) ((addr & ~3ULL) + 2);
      p->val = ((((((p->val >> 2) & MASK20) + value) & MASK20) << 2) 
		| (p->val & ~(MASK20 << 2)));
      break;
    case 1:
      p = (struct unaligned_uint32 *) ((grub_uint8_t *) (addr & ~3ULL) + 7);
      p->val = ((((((p->val >> 3) & MASK20) + value) & MASK20) << 3)
		| (p->val & ~(MASK20 << 3)));
      break;
    case 2:
      p = (struct unaligned_uint32 *) ((grub_uint8_t *) (addr & ~3ULL) + 12);
      p->val = ((((((p->val >> 4) & MASK20) + value) & MASK20) << 4)
		| (p->val & ~(MASK20 << 4)));
      break;
    }
}

#define MASKF21 ( ((1 << 23) - 1) & ~((1 << 7) | (1 << 8)) )

static grub_uint32_t
add_value_to_slot_21_real (grub_uint32_t a, grub_uint32_t value)
{
  grub_uint32_t high, mid, low, c;
  low  = (a & 0x00007f);
  mid  = (a & 0x7fc000) >> 7;
  high = (a & 0x003e00) << 7;
  c = (low | mid | high) + value;
  return (c & 0x7f) | ((c << 7) & 0x7fc000) | ((c >> 7) & 0x0003e00); //0x003e00
}

static void
add_value_to_slot_21 (grub_addr_t addr, grub_uint32_t value)
{
  struct unaligned_uint32 *p;
  switch (addr & 3)
    {
    case 0:
      p = (struct unaligned_uint32 *) ((addr & ~3ULL) + 2);
      p->val = ((add_value_to_slot_21_real (((p->val >> 2) & MASKF21), value) & MASKF21) << 2) | (p->val & ~(MASKF21 << 2));
      break;
    case 1:
      p = (struct unaligned_uint32 *) ((grub_uint8_t *) (addr & ~3ULL) + 7);
      p->val = ((add_value_to_slot_21_real (((p->val >> 3) & MASKF21), value) & MASKF21) << 3) | (p->val & ~(MASKF21 << 3));
      break;
    case 2:
      p = (struct unaligned_uint32 *) ((grub_uint8_t *) (addr & ~3ULL) + 12);
      p->val = ((add_value_to_slot_21_real (((p->val >> 4) & MASKF21), value) & MASKF21) << 4) | (p->val & ~(MASKF21 << 4));
      break;
    }
}

static const grub_uint8_t nopm[5] =
  {
    /* [MLX]       nop.m 0x0 */
    0x05, 0x00, 0x00, 0x00, 0x01
  };

static const grub_uint8_t jump[0x20] =
  {
    /* ld8 r16=[r15],8 */
    0x02, 0x80, 0x20, 0x1e, 0x18, 0x14,
    /* mov r14=r1;; */
    0xe0, 0x00, 0x04, 0x00, 0x42, 0x00,
    /* nop.i 0x0 */
    0x00, 0x00, 0x04, 0x00,
    /* ld8 r1=[r15] */
    0x11, 0x08, 0x00, 0x1e, 0x18, 0x10,
    /* mov b6=r16 */
    0x60, 0x80, 0x04, 0x80, 0x03, 0x00,
    /* br.few b6;; */
    0x60, 0x00, 0x80, 0x00
  };

struct ia64_trampoline
{
  /* nop.m */
  grub_uint8_t nop[5];
  /* movl r15 = addr*/
  grub_uint8_t addr_hi[6];
  grub_uint8_t e0;
  grub_uint8_t addr_lo[4];
  grub_uint8_t jump[0x20];
};

static void
make_trampoline (struct ia64_trampoline *tr, grub_uint64_t addr)
{
  COMPILE_TIME_ASSERT (sizeof (struct ia64_trampoline)
		       == GRUB_IA64_DL_TRAMP_SIZE);
  grub_memcpy (tr->nop, nopm, sizeof (tr->nop));
  tr->addr_hi[0] = ((addr & 0xc00000) >> 16);
  tr->addr_hi[1] = (addr >> 24) & 0xff;
  tr->addr_hi[2] = (addr >> 32) & 0xff;
  tr->addr_hi[3] = (addr >> 40) & 0xff;
  tr->addr_hi[4] = (addr >> 48) & 0xff;
  tr->addr_hi[5] = (addr >> 56) & 0xff;
  tr->e0 = 0xe0;
  tr->addr_lo[0] = ((addr & 0x000f) << 4) | 0x01;
  tr->addr_lo[1] = (((addr & 0x0070) >> 4) | ((addr & 0x070000) >> 11)
		    | ((addr & 0x200000) >> 17));
  tr->addr_lo[2] = ((addr & 0x1f80) >> 5) | ((addr & 0x180000) >> 19);
  tr->addr_lo[3] = ((addr & 0xe000) >> 13) | 0x60;
  grub_memcpy (tr->jump, jump, sizeof (tr->jump));
}

/* Relocate symbols.  */
grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  Elf_Ehdr *e = ehdr;
  Elf_Shdr *s;
  Elf_Word entsize;
  unsigned i;
  grub_uint64_t *gp, *gpptr;
  struct ia64_trampoline *tr;

  gp = (grub_uint64_t *) mod->base;
  gpptr = (grub_uint64_t *) mod->got;
  tr = mod->tramp;

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
		grub_addr_t addr;
		Elf_Sym *sym;
		grub_uint64_t value;

		if (seg->size < (rel->r_offset & ~3))
		  return grub_error (GRUB_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");

		addr = (grub_addr_t) seg->addr + rel->r_offset;
		sym = (Elf_Sym *) ((char *) mod->symtab
				     + entsize * ELF_R_SYM (rel->r_info));

		/* On the PPC the value does not have an explicit
		   addend, add it.  */
		value = sym->st_value + rel->r_addend;

		switch (ELF_R_TYPE (rel->r_info))
		  {
		  case R_IA64_PCREL21B:
		    {
		      grub_uint64_t noff;
		      make_trampoline (tr, value);
		      noff = ((char *) tr - (char *) (addr & ~3)) >> 4;
		      tr++;
		      if (noff & ~MASK19)
			return grub_error (GRUB_ERR_BAD_OS,
					   "trampoline offset too big (%lx)", noff);
		      add_value_to_slot_20b (addr, noff);
		    }
		    break;
		  case R_IA64_SEGREL64LSB:
		    *(grub_uint64_t *) addr += value - (grub_addr_t) seg->addr;
		    break;
		  case R_IA64_FPTR64LSB:
		  case R_IA64_DIR64LSB:
		    *(grub_uint64_t *) addr += value;
		    break;
		  case R_IA64_PCREL64LSB:
		    *(grub_uint64_t *) addr += value - addr;
		    break;
		  case R_IA64_GPREL22:
		    add_value_to_slot_21 (addr, value - (grub_addr_t) gp);
		    break;

		  case R_IA64_LTOFF22X:
		  case R_IA64_LTOFF22:
		    if (ELF_ST_TYPE (sym->st_info) == STT_FUNC)
		      value = *(grub_uint64_t *) sym->st_value + rel->r_addend;
		  case R_IA64_LTOFF_FPTR22:
		    *gpptr = value;
		    add_value_to_slot_21 (addr, (grub_addr_t) gpptr - (grub_addr_t) gp);
		    gpptr++;
		    break;

		    /* We treat LTOFF22X as LTOFF22, so we can ignore LDXMOV.  */
		  case R_IA64_LDXMOV:
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
