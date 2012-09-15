/* grub-mkimage.c - make a bootable image */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#undef ELF_R_SYM
#undef ELF_R_TYPE

#if defined(MKIMAGE_ELF32)
# define SUFFIX(x)	x ## 32
# define ELFCLASSXX	ELFCLASS32
# define Elf_Ehdr	Elf32_Ehdr
# define Elf_Phdr	Elf32_Phdr
# define Elf_Addr	Elf32_Addr
# define Elf_Sym	Elf32_Sym
# define Elf_Off	Elf32_Off
# define Elf_Shdr	Elf32_Shdr
# define Elf_Rela       Elf32_Rela
# define Elf_Rel        Elf32_Rel
# define Elf_Word       Elf32_Word
# define Elf_Half       Elf32_Half
# define Elf_Section    Elf32_Section
# define ELF_R_SYM(val)		ELF32_R_SYM(val)
# define ELF_R_TYPE(val)		ELF32_R_TYPE(val)
# define ELF_ST_TYPE(val)		ELF32_ST_TYPE(val)
#elif defined(MKIMAGE_ELF64)
# define SUFFIX(x)	x ## 64
# define ELFCLASSXX	ELFCLASS64
# define Elf_Ehdr	Elf64_Ehdr
# define Elf_Phdr	Elf64_Phdr
# define Elf_Addr	Elf64_Addr
# define Elf_Sym	Elf64_Sym
# define Elf_Off	Elf64_Off
# define Elf_Shdr	Elf64_Shdr
# define Elf_Rela       Elf64_Rela
# define Elf_Rel        Elf64_Rel
# define Elf_Word       Elf64_Word
# define Elf_Half       Elf64_Half
# define Elf_Section    Elf64_Section
# define ELF_R_SYM(val)		ELF64_R_SYM(val)
# define ELF_R_TYPE(val)		ELF64_R_TYPE(val)
# define ELF_ST_TYPE(val)		ELF64_ST_TYPE(val)
#else
#error "I'm confused"
#endif

/* Relocate symbols; note that this function overwrites the symbol table.
   Return the address of a start symbol.  */
static Elf_Addr
SUFFIX (relocate_symbols) (Elf_Ehdr *e, Elf_Shdr *sections,
			   Elf_Shdr *symtab_section, Elf_Addr *section_addresses,
			   Elf_Half section_entsize, Elf_Half num_sections,
			   void *jumpers, Elf_Addr jumpers_addr,
			   struct image_target_desc *image_target)
{
  Elf_Word symtab_size, sym_size, num_syms;
  Elf_Off symtab_offset;
  Elf_Addr start_address = 0;
  Elf_Sym *sym;
  Elf_Word i;
  Elf_Shdr *strtab_section;
  const char *strtab;
  grub_uint64_t *jptr = jumpers;

  strtab_section
    = (Elf_Shdr *) ((char *) sections
		      + (grub_target_to_host32 (symtab_section->sh_link)
			 * section_entsize));
  strtab = (char *) e + grub_target_to_host (strtab_section->sh_offset);

  symtab_size = grub_target_to_host (symtab_section->sh_size);
  sym_size = grub_target_to_host (symtab_section->sh_entsize);
  symtab_offset = grub_target_to_host (symtab_section->sh_offset);
  num_syms = symtab_size / sym_size;

  for (i = 0, sym = (Elf_Sym *) ((char *) e + symtab_offset);
       i < num_syms;
       i++, sym = (Elf_Sym *) ((char *) sym + sym_size))
    {
      Elf_Section cur_index;
      const char *name;

      name = strtab + grub_target_to_host32 (sym->st_name);

      cur_index = grub_target_to_host16 (sym->st_shndx);
      if (cur_index == STN_ABS)
        {
          continue;
        }
      else if ((cur_index == STN_UNDEF))
	{
	  if (sym->st_name)
	    grub_util_error ("undefined symbol %s", name);
	  else
	    continue;
	}
      else if (cur_index >= num_sections)
	grub_util_error ("section %d does not exist", cur_index);

      sym->st_value = (grub_target_to_host (sym->st_value)
		       + section_addresses[cur_index]);

      if (image_target->elf_target == EM_IA_64 && ELF_ST_TYPE (sym->st_info)
	  == STT_FUNC)
	{
	  *jptr = sym->st_value;
	  sym->st_value = (char *) jptr - (char *) jumpers + jumpers_addr;
	  jptr++;
	  *jptr = 0;
	  jptr++;
	}
      grub_util_info ("locating %s at 0x%llx (0x%llx)", name,
		      (unsigned long long) sym->st_value,
		      (unsigned long long) section_addresses[cur_index]);

      if (! start_address)
	if (strcmp (name, "_start") == 0 || strcmp (name, "start") == 0)
	  start_address = sym->st_value;
    }

  return start_address;
}

/* Return the address of a symbol at the index I in the section S.  */
static Elf_Addr
SUFFIX (get_symbol_address) (Elf_Ehdr *e, Elf_Shdr *s, Elf_Word i,
			     struct image_target_desc *image_target)
{
  Elf_Sym *sym;

  sym = (Elf_Sym *) ((char *) e
		       + grub_target_to_host32 (s->sh_offset)
		       + i * grub_target_to_host32 (s->sh_entsize));
  return sym->st_value;
}

/* Return the address of a modified value.  */
static Elf_Addr *
SUFFIX (get_target_address) (Elf_Ehdr *e, Elf_Shdr *s, Elf_Addr offset,
		    struct image_target_desc *image_target)
{
  return (Elf_Addr *) ((char *) e + grub_target_to_host32 (s->sh_offset) + offset);
}

#ifdef MKIMAGE_ELF64
static Elf_Addr
SUFFIX (count_funcs) (Elf_Ehdr *e, Elf_Shdr *symtab_section,
		      struct image_target_desc *image_target)
{
  Elf_Word symtab_size, sym_size, num_syms;
  Elf_Off symtab_offset;
  Elf_Sym *sym;
  Elf_Word i;
  int ret = 0;

  symtab_size = grub_target_to_host (symtab_section->sh_size);
  sym_size = grub_target_to_host (symtab_section->sh_entsize);
  symtab_offset = grub_target_to_host (symtab_section->sh_offset);
  num_syms = symtab_size / sym_size;

  for (i = 0, sym = (Elf_Sym *) ((char *) e + symtab_offset);
       i < num_syms;
       i++, sym = (Elf_Sym *) ((char *) sym + sym_size))
    if (ELF_ST_TYPE (sym->st_info) == STT_FUNC)
      ret++;

  return ret;
}
#endif

#ifdef MKIMAGE_ELF64
struct unaligned_uint32
{
  grub_uint32_t val;
}  __attribute__ ((packed));

#define MASK20 ((1 << 20) - 1)
#define MASK19 ((1 << 19) - 1)
#define MASK3 (~(grub_addr_t) 3)

static void
add_value_to_slot_20b (grub_addr_t addr, grub_uint32_t value)
{
  struct unaligned_uint32 *p;
  switch (addr & 3)
    {
    case 0:
      p = (struct unaligned_uint32 *) ((addr & MASK3) + 2);
      p->val = ((((((p->val >> 2) & MASK20) + value) & MASK20) << 2) 
		| (p->val & ~(MASK20 << 2)));
      break;
    case 1:
      p = (struct unaligned_uint32 *) ((grub_uint8_t *) (addr & MASK3) + 7);
      p->val = ((((((p->val >> 3) & MASK20) + value) & MASK20) << 3)
		| (p->val & ~(MASK20 << 3)));
      break;
    case 2:
      p = (struct unaligned_uint32 *) ((grub_uint8_t *) (addr & MASK3) + 12);
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
      p = (struct unaligned_uint32 *) ((addr & MASK3) + 2);
      p->val = ((add_value_to_slot_21_real (((p->val >> 2) & MASKF21), value) & MASKF21) << 2) | (p->val & ~(MASKF21 << 2));
      break;
    case 1:
      p = (struct unaligned_uint32 *) ((grub_uint8_t *) (addr & MASK3) + 7);
      p->val = ((add_value_to_slot_21_real (((p->val >> 3) & MASKF21), value) & MASKF21) << 3) | (p->val & ~(MASKF21 << 3));
      break;
    case 2:
      p = (struct unaligned_uint32 *) ((grub_uint8_t *) (addr & MASK3) + 12);
      p->val = ((add_value_to_slot_21_real (((p->val >> 4) & MASKF21), value) & MASKF21) << 4) | (p->val & ~(MASKF21 << 4));
      break;
    }
}


struct ia64_kernel_trampoline
{
  /* nop.m */
  grub_uint8_t nop[5];
  /* movl r15 = addr*/
  grub_uint8_t addr_hi[6];
  grub_uint8_t e0;
  grub_uint8_t addr_lo[4];
  grub_uint8_t jump[0x20];
};

static grub_uint8_t nopm[5] =
  {
    /* [MLX]       nop.m 0x0 */
    0x05, 0x00, 0x00, 0x00, 0x01
  };

static grub_uint8_t jump[0x20] =
  {
    /* [MMI]       add r15=r15,r1;; */
    0x0b, 0x78, 0x3c, 0x02, 0x00, 0x20,
    /* ld8 r16=[r15],8 */
    0x00, 0x41, 0x3c, 0x30, 0x28, 0xc0,
    /* mov r14=r1;; */
    0x01, 0x08, 0x00, 0x84,
    /* 	[MIB]       ld8 r1=[r15] */
    0x11, 0x08, 0x00, 0x1e, 0x18, 0x10,
    /* mov b6=r16 */
    0x60, 0x80, 0x04, 0x80, 0x03, 0x00, 
    /* br.few b6;; */
    0x60, 0x00, 0x80, 0x00       	            
  };

static void
make_trampoline (struct ia64_kernel_trampoline *tr, grub_uint64_t addr)
{
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
#endif

/* Deal with relocation information. This function relocates addresses
   within the virtual address space starting from 0. So only relative
   addresses can be fully resolved. Absolute addresses must be relocated
   again by a PE32 relocator when loaded.  */
static void
SUFFIX (relocate_addresses) (Elf_Ehdr *e, Elf_Shdr *sections,
			     Elf_Addr *section_addresses,
			     Elf_Half section_entsize, Elf_Half num_sections,
			     const char *strtab,
			     char *pe_target, Elf_Addr tramp_off,
			     Elf_Addr got_off,
			     struct image_target_desc *image_target)
{
  Elf_Half i;
  Elf_Shdr *s;
#ifdef MKIMAGE_ELF64
  struct ia64_kernel_trampoline *tr = (void *) (pe_target + tramp_off);
  grub_uint64_t *gpptr = (void *) (pe_target + got_off);
#endif

  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if ((s->sh_type == grub_host_to_target32 (SHT_REL)) ||
        (s->sh_type == grub_host_to_target32 (SHT_RELA)))
      {
	Elf_Rela *r;
	Elf_Word rtab_size, r_size, num_rs;
	Elf_Off rtab_offset;
	Elf_Shdr *symtab_section;
	Elf_Word target_section_index;
	Elf_Addr target_section_addr;
	Elf_Shdr *target_section;
	Elf_Word j;

	symtab_section = (Elf_Shdr *) ((char *) sections
					 + (grub_target_to_host32 (s->sh_link)
					    * section_entsize));
	target_section_index = grub_target_to_host32 (s->sh_info);
	target_section_addr = section_addresses[target_section_index];
	target_section = (Elf_Shdr *) ((char *) sections
					 + (target_section_index
					    * section_entsize));

	grub_util_info ("dealing with the relocation section %s for %s",
			strtab + grub_target_to_host32 (s->sh_name),
			strtab + grub_target_to_host32 (target_section->sh_name));

	rtab_size = grub_target_to_host32 (s->sh_size);
	r_size = grub_target_to_host32 (s->sh_entsize);
	rtab_offset = grub_target_to_host32 (s->sh_offset);
	num_rs = rtab_size / r_size;

	for (j = 0, r = (Elf_Rela *) ((char *) e + rtab_offset);
	     j < num_rs;
	     j++, r = (Elf_Rela *) ((char *) r + r_size))
	  {
            Elf_Addr info;
	    Elf_Addr offset;
	    Elf_Addr sym_addr;
	    Elf_Addr *target;
	    Elf_Addr addend;

	    offset = grub_target_to_host (r->r_offset);
	    target = SUFFIX (get_target_address) (e, target_section,
						  offset, image_target);
	    info = grub_target_to_host (r->r_info);
	    sym_addr = SUFFIX (get_symbol_address) (e, symtab_section,
						    ELF_R_SYM (info), image_target);

            addend = (s->sh_type == grub_target_to_host32 (SHT_RELA)) ?
	      r->r_addend : 0;

	   switch (image_target->elf_target)
	     {
	     case EM_386:
	      switch (ELF_R_TYPE (info))
		{
		case R_386_NONE:
		  break;

		case R_386_32:
		  /* This is absolute.  */
		  *target = grub_host_to_target32 (grub_target_to_host32 (*target)
						   + addend + sym_addr);
		  grub_util_info ("relocating an R_386_32 entry to 0x%llx at the offset 0x%llx",
				  (unsigned long long) *target,
				  (unsigned long long) offset);
		  break;

		case R_386_PC32:
		  /* This is relative.  */
		  *target = grub_host_to_target32 (grub_target_to_host32 (*target)
						   + addend + sym_addr
						   - target_section_addr - offset
						   - image_target->vaddr_offset);
		  grub_util_info ("relocating an R_386_PC32 entry to 0x%llx at the offset 0x%llx",
				  (unsigned long long) *target,
				  (unsigned long long) offset);
		  break;
		default:
		  grub_util_error (_("relocation 0x%x is not implemented yet"),
				   ELF_R_TYPE (info));
		  break;
		}
	      break;
#ifdef MKIMAGE_ELF64
	     case EM_X86_64:
	      switch (ELF_R_TYPE (info))
		{

		case R_X86_64_NONE:
		  break;

		case R_X86_64_64:
		  *target = grub_host_to_target64 (grub_target_to_host64 (*target)
						   + addend + sym_addr);
		  grub_util_info ("relocating an R_X86_64_64 entry to 0x%llx at the offset 0x%llx",
				  (unsigned long long) *target,
				  (unsigned long long) offset);
		  break;

		case R_X86_64_PC32:
		  {
		    grub_uint32_t *t32 = (grub_uint32_t *) target;
		    *t32 = grub_host_to_target64 (grub_target_to_host32 (*t32)
						  + addend + sym_addr
						  - target_section_addr - offset
						  - image_target->vaddr_offset);
		    grub_util_info ("relocating an R_X86_64_PC32 entry to 0x%x at the offset 0x%llx",
				    *t32, (unsigned long long) offset);
		    break;
		  }

		case R_X86_64_32:
		case R_X86_64_32S:
		  {
		    grub_uint32_t *t32 = (grub_uint32_t *) target;
		    *t32 = grub_host_to_target64 (grub_target_to_host32 (*t32)
						  + addend + sym_addr);
		    grub_util_info ("relocating an R_X86_64_32(S) entry to 0x%x at the offset 0x%llx",
				    *t32, (unsigned long long) offset);
		    break;
		  }

		default:
		  grub_util_error (_("relocation 0x%x is not implemented yet"),
				   ELF_R_TYPE (info));
		  break;
		}
	      break;
	     case EM_IA_64:
	      switch (ELF_R_TYPE (info))
		{
		case R_IA64_PCREL21B:
		  {
		    grub_uint64_t noff;
		    make_trampoline (tr, addend + sym_addr);
		    noff = ((char *) tr - (char *) pe_target
			    - target_section_addr - (offset & ~3)) >> 4;
		    tr++;
		    if (noff & ~MASK19)
		      grub_util_error ("trampoline offset too big (%"
				       PRIxGRUB_UINT64_T ")", noff);
		    add_value_to_slot_20b ((grub_addr_t) target, noff);
		  }
		  break;

		case R_IA64_LTOFF22X:
		case R_IA64_LTOFF22:
		  {
		    Elf_Sym *sym;

		    sym = (Elf_Sym *) ((char *) e
				       + grub_target_to_host32 (symtab_section->sh_offset)
				       + ELF_R_SYM (info) * grub_target_to_host32 (symtab_section->sh_entsize));
		    if (ELF_ST_TYPE (sym->st_info) == STT_FUNC)
		      sym_addr = grub_target_to_host64 (*(grub_uint64_t *) (pe_target
									    + sym->st_value
									    - image_target->vaddr_offset));
		  }
		case R_IA64_LTOFF_FPTR22:
		  *gpptr = grub_host_to_target64 (addend + sym_addr);
		  add_value_to_slot_21 ((grub_addr_t) target,
					(char *) gpptr - (char *) pe_target
					+ image_target->vaddr_offset);
		  gpptr++;
		  break;

		case R_IA64_GPREL22:
		  add_value_to_slot_21 ((grub_addr_t) target,
					addend + sym_addr);
		  break;
		case R_IA64_PCREL64LSB:
		  *target = grub_host_to_target64 (grub_target_to_host64 (*target)
						   + addend + sym_addr
						   - target_section_addr - offset
						   - image_target->vaddr_offset);
		  break;

		case R_IA64_SEGREL64LSB:
		  *target = grub_host_to_target64 (grub_target_to_host64 (*target)
						   + addend + sym_addr - target_section_addr);
		  break;
		case R_IA64_DIR64LSB:
		case R_IA64_FPTR64LSB:
		  *target = grub_host_to_target64 (grub_target_to_host64 (*target)
						   + addend + sym_addr);
		  grub_util_info ("relocating a direct entry to 0x%"
				  PRIxGRUB_UINT64_T " at the offset 0x%llx",
				  *target, (unsigned long long) offset);
		  break;

		  /* We treat LTOFF22X as LTOFF22, so we can ignore LDXMOV.  */
		case R_IA64_LDXMOV:
		  break;

		default:
		  grub_util_error (_("relocation 0x%x is not implemented yet"),
		  		   ELF_R_TYPE (info));
		  break;
		}
	       break;
#endif
	     default:
	       grub_util_error ("unknown architecture type %d",
				image_target->elf_target);
	     }
	  }
      }
}

/* Add a PE32's fixup entry for a relocation. Return the resulting address
   after having written to the file OUT.  */
static Elf_Addr
SUFFIX (add_fixup_entry) (struct fixup_block_list **cblock, grub_uint16_t type,
			  Elf_Addr addr, int flush, Elf_Addr current_address,
			  struct image_target_desc *image_target)
{
  struct grub_pe32_fixup_block *b;

  b = &((*cblock)->b);

  /* First, check if it is necessary to write out the current block.  */
  if ((*cblock)->state)
    {
      if (flush || addr < b->page_rva || b->page_rva + 0x1000 <= addr)
	{
	  grub_uint32_t size;

	  if (flush)
	    {
	      /* Add as much padding as necessary to align the address
		 with a section boundary.  */
	      Elf_Addr next_address;
	      unsigned padding_size;
              size_t cur_index;

	      next_address = current_address + b->block_size;
	      padding_size = ((ALIGN_UP (next_address, image_target->section_align)
			       - next_address)
			      >> 1);
              cur_index = ((b->block_size - sizeof (*b)) >> 1);
              grub_util_info ("adding %d padding fixup entries", padding_size);
	      while (padding_size--)
		{
		  b->entries[cur_index++] = 0;
		  b->block_size += 2;
		}
	    }
          else while (b->block_size & (8 - 1))
            {
	      /* If not aligned with a 32-bit boundary, add
		 a padding entry.  */
              size_t cur_index;

              grub_util_info ("adding a padding fixup entry");
              cur_index = ((b->block_size - sizeof (*b)) >> 1);
              b->entries[cur_index] = 0;
              b->block_size += 2;
            }

          /* Flush it.  */
          grub_util_info ("writing %d bytes of a fixup block starting at 0x%x",
                          b->block_size, b->page_rva);
          size = b->block_size;
	  current_address += size;
	  b->page_rva = grub_host_to_target32 (b->page_rva);
	  b->block_size = grub_host_to_target32 (b->block_size);
	  (*cblock)->next = xmalloc (sizeof (**cblock) + 2 * 0x1000);
	  memset ((*cblock)->next, 0, sizeof (**cblock) + 2 * 0x1000);
	  *cblock = (*cblock)->next;
	}
    }

  b = &((*cblock)->b);

  if (! flush)
    {
      grub_uint16_t entry;
      size_t cur_index;

      /* If not allocated yet, allocate a block with enough entries.  */
      if (! (*cblock)->state)
	{
	  (*cblock)->state = 1;

	  /* The spec does not mention the requirement of a Page RVA.
	     Here, align the address with a 4K boundary for safety.  */
	  b->page_rva = (addr & ~(0x1000 - 1));
	  b->block_size = sizeof (*b);
	}

      /* Sanity check.  */
      if (b->block_size >= sizeof (*b) + 2 * 0x1000)
	grub_util_error ("too many fixup entries");

      /* Add a new entry.  */
      cur_index = ((b->block_size - sizeof (*b)) >> 1);
      entry = GRUB_PE32_FIXUP_ENTRY (type, addr - b->page_rva);
      b->entries[cur_index] = grub_host_to_target16 (entry);
      b->block_size += 2;
    }

  return current_address;
}

/* Make a .reloc section.  */
static Elf_Addr
SUFFIX (make_reloc_section) (Elf_Ehdr *e, void **out,
			     Elf_Addr *section_addresses, Elf_Shdr *sections,
			     Elf_Half section_entsize, Elf_Half num_sections,
			     const char *strtab,
			     Elf_Addr jumpers, grub_size_t njumpers,
			     struct image_target_desc *image_target)
{
  unsigned i;
  Elf_Shdr *s;
  struct fixup_block_list *lst, *lst0;
  Elf_Addr current_address = 0;

  lst = lst0 = xmalloc (sizeof (*lst) + 2 * 0x1000);
  memset (lst, 0, sizeof (*lst) + 2 * 0x1000);

  for (i = 0, s = sections; i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if ((s->sh_type == grub_cpu_to_le32 (SHT_REL)) ||
        (s->sh_type == grub_cpu_to_le32 (SHT_RELA)))
      {
	Elf_Rel *r;
	Elf_Word rtab_size, r_size, num_rs;
	Elf_Off rtab_offset;
	Elf_Addr section_address;
	Elf_Word j;

	grub_util_info ("translating the relocation section %s",
			strtab + grub_le_to_cpu32 (s->sh_name));

	rtab_size = grub_le_to_cpu32 (s->sh_size);
	r_size = grub_le_to_cpu32 (s->sh_entsize);
	rtab_offset = grub_le_to_cpu32 (s->sh_offset);
	num_rs = rtab_size / r_size;

	section_address = section_addresses[grub_le_to_cpu32 (s->sh_info)];

	for (j = 0, r = (Elf_Rel *) ((char *) e + rtab_offset);
	     j < num_rs;
	     j++, r = (Elf_Rel *) ((char *) r + r_size))
	  {
	    Elf_Addr info;
	    Elf_Addr offset;

	    offset = grub_le_to_cpu32 (r->r_offset);
	    info = grub_le_to_cpu32 (r->r_info);

	    /* Necessary to relocate only absolute addresses.  */
	    switch (image_target->elf_target)
	      {
	      case EM_386:
		if (ELF_R_TYPE (info) == R_386_32)
		  {
		    Elf_Addr addr;

		    addr = section_address + offset;
		    grub_util_info ("adding a relocation entry for 0x%llx",
				    (unsigned long long) addr);
		    current_address
		      = SUFFIX (add_fixup_entry) (&lst,
						  GRUB_PE32_REL_BASED_HIGHLOW,
						  addr, 0, current_address,
						  image_target);
		  }
		break;
	      case EM_X86_64:
		if ((ELF_R_TYPE (info) == R_X86_64_32) ||
		    (ELF_R_TYPE (info) == R_X86_64_32S))
		  {
		    grub_util_error ("can\'t add fixup entry for R_X86_64_32(S)");
		  }
		else if (ELF_R_TYPE (info) == R_X86_64_64)
		  {
		    Elf_Addr addr;

		    addr = section_address + offset;
		    grub_util_info ("adding a relocation entry for 0x%llx",
				    (unsigned long long) addr);
		    current_address
		      = SUFFIX (add_fixup_entry) (&lst,
						  GRUB_PE32_REL_BASED_DIR64,
						  addr,
						  0, current_address,
						  image_target);
		  }
		break;
	      case EM_IA_64:
	      switch (ELF_R_TYPE (info))
		{
		case R_IA64_PCREL64LSB:
		case R_IA64_LDXMOV:
		case R_IA64_PCREL21B:
		case R_IA64_LTOFF_FPTR22:
		case R_IA64_LTOFF22X:
		case R_IA64_LTOFF22:
		case R_IA64_GPREL22:
		case R_IA64_SEGREL64LSB:
		  break;

		case R_IA64_FPTR64LSB:
		case R_IA64_DIR64LSB:
#if 1
		  {
		    Elf_Addr addr;

		    addr = section_address + offset;
		    grub_util_info ("adding a relocation entry for 0x%llx",
				    (unsigned long long) addr);
		    current_address
		      = SUFFIX (add_fixup_entry) (&lst,
						  GRUB_PE32_REL_BASED_DIR64,
						  addr,
						  0, current_address,
						  image_target);
		  }
#endif
		  break;
		default:
		  grub_util_error (_("relocation 0x%x is not implemented yet"),
		  		   ELF_R_TYPE (info));
		  break;
		}
		break;
	      default:
		grub_util_error ("unknown machine type 0x%x", image_target->elf_target);
	      }
	  }
      }

  if (image_target->elf_target == EM_IA_64)
    for (i = 0; i < njumpers; i++)
      current_address = SUFFIX (add_fixup_entry) (&lst,
						  GRUB_PE32_REL_BASED_DIR64,
						  jumpers + 8 * i,
						  0, current_address,
						  image_target);

  current_address = SUFFIX (add_fixup_entry) (&lst, 0, 0, 1, current_address, image_target);

  {
    grub_uint8_t *ptr;
    ptr = *out = xmalloc (current_address);
    for (lst = lst0; lst; lst = lst->next)
      if (lst->state)
	{
	  memcpy (ptr, &lst->b, grub_target_to_host32 (lst->b.block_size));
	  ptr += grub_target_to_host32 (lst->b.block_size);
	}
    assert ((current_address + (grub_uint8_t *) *out) == ptr);
  }

  return current_address;
}

/* Determine if this section is a text section. Return false if this
   section is not allocated.  */
static int
SUFFIX (is_text_section) (Elf_Shdr *s, struct image_target_desc *image_target)
{
  if (image_target->id != IMAGE_EFI 
      && grub_target_to_host32 (s->sh_type) != SHT_PROGBITS)
    return 0;
  return ((grub_target_to_host (s->sh_flags) & (SHF_EXECINSTR | SHF_ALLOC))
	  == (SHF_EXECINSTR | SHF_ALLOC));
}

/* Determine if this section is a data section. This assumes that
   BSS is also a data section, since the converter initializes BSS
   when producing PE32 to avoid a bug in EFI implementations.  */
static int
SUFFIX (is_data_section) (Elf_Shdr *s, struct image_target_desc *image_target)
{
  if (image_target->id != IMAGE_EFI 
      && grub_target_to_host32 (s->sh_type) != SHT_PROGBITS)
    return 0;
  return ((grub_target_to_host (s->sh_flags) & (SHF_EXECINSTR | SHF_ALLOC))
	  == SHF_ALLOC);
}

/* Return if the ELF header is valid.  */
static int
SUFFIX (check_elf_header) (Elf_Ehdr *e, size_t size, struct image_target_desc *image_target)
{
  if (size < sizeof (*e)
      || e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_ident[EI_CLASS] != ELFCLASSXX
      || e->e_version != grub_host_to_target32 (EV_CURRENT))
    return 0;

  return 1;
}

/* Locate section addresses by merging code sections and data sections
   into .text and .data, respectively. Return the array of section
   addresses.  */
static Elf_Addr *
SUFFIX (locate_sections) (Elf_Shdr *sections, Elf_Half section_entsize,
			  Elf_Half num_sections, const char *strtab,
			  grub_size_t *exec_size, grub_size_t *kernel_sz,
			  grub_size_t *all_align,
			  struct image_target_desc *image_target)
{
  int i;
  Elf_Addr current_address;
  Elf_Addr *section_addresses;
  Elf_Shdr *s;

  *all_align = 1;

  section_addresses = xmalloc (sizeof (*section_addresses) * num_sections);
  memset (section_addresses, 0, sizeof (*section_addresses) * num_sections);

  current_address = 0;

  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if ((grub_target_to_host (s->sh_flags) & SHF_ALLOC) 
	&& grub_host_to_target32 (s->sh_addralign) > *all_align)
      *all_align = grub_host_to_target32 (s->sh_addralign);


  /* .text */
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if (SUFFIX (is_text_section) (s, image_target))
      {
	Elf_Word align = grub_host_to_target_addr (s->sh_addralign);
	const char *name = strtab + grub_host_to_target32 (s->sh_name);
	if (align)
	  current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
				      align) - image_target->vaddr_offset;
	grub_util_info ("locating the section %s at 0x%llx",
			name, (unsigned long long) current_address);
	if (image_target->id != IMAGE_EFI)
	  current_address = grub_host_to_target_addr (s->sh_addr)
	    - image_target->link_addr;
	section_addresses[i] = current_address;
	current_address += grub_host_to_target_addr (s->sh_size);
      }

  current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
			      image_target->section_align)
    - image_target->vaddr_offset;
  *exec_size = current_address;

  /* .data */
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if (SUFFIX (is_data_section) (s, image_target))
      {
	Elf_Word align = grub_host_to_target_addr (s->sh_addralign);
	const char *name = strtab + grub_host_to_target32 (s->sh_name);

	if (align)
	  current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
				      align)
	    - image_target->vaddr_offset;

	grub_util_info ("locating the section %s at 0x%llx",
			name, (unsigned long long) current_address);
	if (image_target->id != IMAGE_EFI)
	  current_address = grub_host_to_target_addr (s->sh_addr)
	    - image_target->link_addr;
	section_addresses[i] = current_address;
	current_address += grub_host_to_target_addr (s->sh_size);
      }

  current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
			      image_target->section_align) - image_target->vaddr_offset;
  *kernel_sz = current_address;
  return section_addresses;
}

static char *
SUFFIX (load_image) (const char *kernel_path, grub_size_t *exec_size, 
		     grub_size_t *kernel_sz, grub_size_t *bss_size,
		     grub_size_t total_module_size, grub_uint64_t *start,
		     void **reloc_section, grub_size_t *reloc_size,
		     grub_size_t *align,
		     struct image_target_desc *image_target)
{
  char *kernel_img, *out_img;
  const char *strtab;
  Elf_Ehdr *e;
  Elf_Shdr *sections;
  Elf_Addr *section_addresses;
  Elf_Addr *section_vaddresses;
  int i;
  Elf_Shdr *s;
  Elf_Half num_sections;
  Elf_Off section_offset;
  Elf_Half section_entsize;
  grub_size_t kernel_size;
  grub_size_t ia64jmp_off = 0, ia64_toff = 0, ia64_got_off = 0;
  unsigned ia64jmpnum = 0;
  Elf_Shdr *symtab_section = 0;
  grub_size_t got = 0;

  *start = 0;

  kernel_size = grub_util_get_image_size (kernel_path);
  kernel_img = xmalloc (kernel_size);
  grub_util_load_image (kernel_path, kernel_img);

  e = (Elf_Ehdr *) kernel_img;
  if (! SUFFIX (check_elf_header) (e, kernel_size, image_target))
    grub_util_error ("invalid ELF header");

  section_offset = grub_target_to_host (e->e_shoff);
  section_entsize = grub_target_to_host16 (e->e_shentsize);
  num_sections = grub_target_to_host16 (e->e_shnum);

  if (kernel_size < section_offset + section_entsize * num_sections)
    grub_util_error (_("premature end of file %s"), kernel_path);

  sections = (Elf_Shdr *) (kernel_img + section_offset);

  /* Relocate sections then symbols in the virtual address space.  */
  s = (Elf_Shdr *) ((char *) sections
		      + grub_host_to_target16 (e->e_shstrndx) * section_entsize);
  strtab = (char *) e + grub_host_to_target_addr (s->sh_offset);

  section_addresses = SUFFIX (locate_sections) (sections, section_entsize,
						num_sections, strtab,
						exec_size, kernel_sz, align,
						image_target);

  section_vaddresses = xmalloc (sizeof (*section_addresses) * num_sections);

  for (i = 0; i < num_sections; i++)
    section_vaddresses[i] = section_addresses[i] + image_target->vaddr_offset;

  if (image_target->id != IMAGE_EFI)
    {
      Elf_Addr current_address = *kernel_sz;

      for (i = 0, s = sections;
	   i < num_sections;
	   i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
	if (grub_target_to_host32 (s->sh_type) == SHT_NOBITS)
	  {
	    Elf_Word sec_align = grub_host_to_target_addr (s->sh_addralign);
	    const char *name = strtab + grub_host_to_target32 (s->sh_name);

	    if (sec_align)
	      current_address = ALIGN_UP (current_address
					  + image_target->vaddr_offset,
					  sec_align)
		- image_target->vaddr_offset;
	
	    grub_util_info ("locating the section %s at 0x%llx",
			    name, (unsigned long long) current_address);
	    if (image_target->id != IMAGE_EFI)
	      current_address = grub_host_to_target_addr (s->sh_addr)
		- image_target->link_addr;

	    section_vaddresses[i] = current_address
	      + image_target->vaddr_offset;
	    current_address += grub_host_to_target_addr (s->sh_size);
	  }
      current_address = ALIGN_UP (current_address + image_target->vaddr_offset,
				  image_target->section_align)
	- image_target->vaddr_offset;
      *bss_size = current_address - *kernel_sz;
    }
  else
    *bss_size = 0;

  if (image_target->id == IMAGE_EFI)
    {
      symtab_section = NULL;
      for (i = 0, s = sections;
	   i < num_sections;
	   i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
	if (s->sh_type == grub_host_to_target32 (SHT_SYMTAB))
	  {
	    symtab_section = s;
	    break;
	  }

#ifdef MKIMAGE_ELF64
      if (image_target->elf_target == EM_IA_64)
	{
	  grub_size_t tramp;

	  *kernel_sz = ALIGN_UP (*kernel_sz, 16);

	  grub_ia64_dl_get_tramp_got_size (e, &tramp, &got);
	  tramp *= sizeof (struct ia64_kernel_trampoline);

	  ia64_toff = *kernel_sz;
	  *kernel_sz += ALIGN_UP (tramp, 16);

	  ia64jmp_off = *kernel_sz;
	  ia64jmpnum = SUFFIX (count_funcs) (e, symtab_section,
					     image_target);
	  *kernel_sz += 16 * ia64jmpnum;

	  ia64_got_off = *kernel_sz;
	  *kernel_sz += ALIGN_UP (got * sizeof (grub_uint64_t), 16);
	}
#endif

      if (! symtab_section)
	grub_util_error ("%s", _("no symbol table"));
    }
  else
    {
      *reloc_size = 0;
      *reloc_section = NULL;
    }

  out_img = xmalloc (*kernel_sz + total_module_size);

  if (image_target->id == IMAGE_EFI)
    {
      *start = SUFFIX (relocate_symbols) (e, sections, symtab_section,
					  section_vaddresses, section_entsize,
					  num_sections, 
					  (char *) out_img + ia64jmp_off, 
					  ia64jmp_off 
					  + image_target->vaddr_offset,
					  image_target);
      if (*start == 0)
	grub_util_error ("start symbol is not defined");
      
      /* Resolve addresses in the virtual address space.  */
      SUFFIX (relocate_addresses) (e, sections, section_addresses, 
				   section_entsize,
				   num_sections, strtab,
				   out_img, ia64_toff, ia64_got_off,
				   image_target);
	  
      *reloc_size = SUFFIX (make_reloc_section) (e, reloc_section,
						 section_vaddresses, sections,
						 section_entsize, num_sections,
						 strtab, ia64jmp_off
						 + image_target->vaddr_offset,
						 2 * ia64jmpnum + got,
						 image_target);
    }

  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if (SUFFIX (is_data_section) (s, image_target)
	|| SUFFIX (is_text_section) (s, image_target))
      {
	if (grub_target_to_host32 (s->sh_type) == SHT_NOBITS)
	  memset (out_img + section_addresses[i], 0,
		  grub_host_to_target_addr (s->sh_size));
	else
	  memcpy (out_img + section_addresses[i],
		  kernel_img + grub_host_to_target_addr (s->sh_offset),
		  grub_host_to_target_addr (s->sh_size));
      }
  free (kernel_img);

  return out_img;
}


#undef SUFFIX
#undef ELFCLASSXX
#undef Elf_Ehdr
#undef Elf_Phdr
#undef Elf_Shdr
#undef Elf_Addr
#undef Elf_Sym
#undef Elf_Off
#undef Elf_Rela
#undef Elf_Rel
#undef ELF_R_TYPE
#undef ELF_R_SYM
#undef Elf_Word
#undef Elf_Half
#undef Elf_Section
#undef ELF_ST_TYPE
