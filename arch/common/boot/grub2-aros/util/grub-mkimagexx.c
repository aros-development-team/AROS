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
# define ELF_R_SYM(val)		ELF32_R_SYM(val)
# define ELF_R_TYPE(val)		ELF32_R_TYPE(val)
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
# define ELF_R_SYM(val)		ELF64_R_SYM(val)
# define ELF_R_TYPE(val)		ELF64_R_TYPE(val)
#else
#error "I'm confused"
#endif

/* Relocate symbols; note that this function overwrites the symbol table.
   Return the address of a start symbol.  */
static Elf_Addr
SUFFIX (relocate_symbols) (Elf_Ehdr *e, Elf_Shdr *sections,
			   Elf_Shdr *symtab_section, Elf_Addr *section_addresses,
			   Elf_Half section_entsize, Elf_Half num_sections,
			   struct image_target_desc *image_target)
{
  Elf_Word symtab_size, sym_size, num_syms;
  Elf_Off symtab_offset;
  Elf_Addr start_address = 0;
  Elf_Sym *sym;
  Elf_Word i;
  Elf_Shdr *strtab_section;
  const char *strtab;

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
      Elf_Section index;
      const char *name;

      name = strtab + grub_target_to_host32 (sym->st_name);

      index = grub_target_to_host16 (sym->st_shndx);
      if (index == STN_ABS)
        {
          continue;
        }
      else if ((index == STN_UNDEF))
	{
	  if (sym->st_name)
	    grub_util_error ("undefined symbol %s", name);
	  else
	    continue;
	}
      else if (index >= num_sections)
	grub_util_error ("section %d does not exist", index);

      sym->st_value = (grub_target_to_host32 (sym->st_value)
		       + section_addresses[index]);
      grub_util_info ("locating %s at 0x%x", name, sym->st_value);

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

/* Deal with relocation information. This function relocates addresses
   within the virtual address space starting from 0. So only relative
   addresses can be fully resolved. Absolute addresses must be relocated
   again by a PE32 relocator when loaded.  */
static void
SUFFIX (relocate_addresses) (Elf_Ehdr *e, Elf_Shdr *sections,
			     Elf_Addr *section_addresses,
			     Elf_Half section_entsize, Elf_Half num_sections,
			     const char *strtab, struct image_target_desc *image_target)
{
  Elf_Half i;
  Elf_Shdr *s;

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

	    if (image_target->voidp_sizeof == 4)
	      switch (ELF_R_TYPE (info))
		{
		case R_386_NONE:
		  break;

		case R_386_32:
		  /* This is absolute.  */
		  *target = grub_host_to_target32 (grub_target_to_host32 (*target)
						   + addend + sym_addr);
		  grub_util_info ("relocating an R_386_32 entry to 0x%x at the offset 0x%x",
				  *target, offset);
		  break;

		case R_386_PC32:
		  /* This is relative.  */
		  *target = grub_host_to_target32 (grub_target_to_host32 (*target)
						   + addend + sym_addr
						   - target_section_addr - offset
						   - image_target->vaddr_offset);
		  grub_util_info ("relocating an R_386_PC32 entry to 0x%x at the offset 0x%x",
				  *target, offset);
		  break;
		default:
		  grub_util_error ("unknown relocation type %d",
				   ELF_R_TYPE (info));
		  break;
		}
	    else
	      switch (ELF_R_TYPE (info))
		{

		case R_X86_64_NONE:
		  break;

		case R_X86_64_64:
		  *target = grub_host_to_target64 (grub_target_to_host64 (*target)
						   + addend + sym_addr);
		  grub_util_info ("relocating an R_X86_64_64 entry to 0x%llx at the offset 0x%llx",
				  *target, offset);
		  break;

		case R_X86_64_PC32:
		  {
		    grub_uint32_t *t32 = (grub_uint32_t *) target;
		    *t32 = grub_host_to_target64 (grub_target_to_host32 (*t32)
						  + addend + sym_addr
						  - target_section_addr - offset
						  - image_target->vaddr_offset);
		    grub_util_info ("relocating an R_X86_64_PC32 entry to 0x%x at the offset 0x%llx",
				    *t32, offset);
		    break;
		  }

		case R_X86_64_32:
		case R_X86_64_32S:
		  {
		    grub_uint32_t *t32 = (grub_uint32_t *) target;
		    *t32 = grub_host_to_target64 (grub_target_to_host32 (*t32)
						  + addend + sym_addr);
		    grub_util_info ("relocating an R_X86_64_32(S) entry to 0x%x at the offset 0x%llx",
				    *t32, offset);
		    break;
		  }

		default:
		  grub_util_error ("unknown relocation type %d",
				   ELF_R_TYPE (info));
		  break;
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
              size_t index;

	      next_address = current_address + b->block_size;
	      padding_size = ((ALIGN_UP (next_address, image_target->section_align)
			       - next_address)
			      >> 1);
              index = ((b->block_size - sizeof (*b)) >> 1);
              grub_util_info ("adding %d padding fixup entries", padding_size);
	      while (padding_size--)
		{
		  b->entries[index++] = 0;
		  b->block_size += 2;
		}
	    }
          else if (b->block_size & (8 - 1))
            {
	      /* If not aligned with a 32-bit boundary, add
		 a padding entry.  */
              size_t index;

              grub_util_info ("adding a padding fixup entry");
              index = ((b->block_size - sizeof (*b)) >> 1);
              b->entries[index] = 0;
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
      size_t index;

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
      index = ((b->block_size - sizeof (*b)) >> 1);
      entry = GRUB_PE32_FIXUP_ENTRY (type, addr - b->page_rva);
      b->entries[index] = grub_host_to_target16 (entry);
      b->block_size += 2;
    }

  return current_address;
}

/* Make a .reloc section.  */
static Elf_Addr
SUFFIX (make_reloc_section) (Elf_Ehdr *e, void **out,
			     Elf_Addr *section_addresses, Elf_Shdr *sections,
			     Elf_Half section_entsize, Elf_Half num_sections,
			     const char *strtab, struct image_target_desc *image_target)
{
  Elf_Half i;
  Elf_Shdr *s;
  struct fixup_block_list *lst, *lst0;
  Elf_Addr current_address = 0;

  lst = lst0 = xmalloc (sizeof (*lst) + 2 * 0x1000);
  memset (lst, 0, sizeof (*lst) + 2 * 0x1000);

  for (i = 0, s = sections;
       i < num_sections;
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
	    if (image_target->voidp_sizeof == 4)
	      {
		if (ELF_R_TYPE (info) == R_386_32)
		  {
		    Elf_Addr addr;

		    addr = section_address + offset;
		    grub_util_info ("adding a relocation entry for 0x%x", addr);
		    current_address
		      = SUFFIX (add_fixup_entry) (&lst,
						  GRUB_PE32_REL_BASED_HIGHLOW,
						  addr, 0, current_address,
						  image_target);
		  }
	      }
	    else
	      {
		if ((ELF_R_TYPE (info) == R_X86_64_32) ||
		    (ELF_R_TYPE (info) == R_X86_64_32S))
		  {
		    grub_util_error ("can\'t add fixup entry for R_X86_64_32(S)");
		  }
		else if (ELF_R_TYPE (info) == R_X86_64_64)
		  {
		    Elf_Addr addr;

		    addr = section_address + offset;
		    grub_util_info ("adding a relocation entry for 0x%llx", addr);
		    current_address
		      = SUFFIX (add_fixup_entry) (&lst,
						  GRUB_PE32_REL_BASED_DIR64,
						  addr,
						  0, current_address,
						  image_target);
		  }
	      }
	  }
      }

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
    if (current_address + *out != ptr)
      {
	grub_util_error ("Bug detected %d != %d\n", ptr - (grub_uint8_t *) *out,
			 current_address);
      }
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
	grub_util_info ("locating the section %s at 0x%x",
			name, current_address);
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

	grub_util_info ("locating the section %s at 0x%x",
			name, current_address);
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
  Elf_Shdr *symtab_section;

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
    grub_util_error ("invalid ELF format");

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
	    Elf_Word align = grub_host_to_target_addr (s->sh_addralign);
	    const char *name = strtab + grub_host_to_target32 (s->sh_name);

	    if (align)
	      current_address = ALIGN_UP (current_address
					  + image_target->vaddr_offset, align)
		- image_target->vaddr_offset;
	
	    grub_util_info ("locating the section %s at 0x%x",
			    name, current_address);
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

      if (! symtab_section)
	grub_util_error ("no symbol table");

      *start = SUFFIX (relocate_symbols) (e, sections, symtab_section,
					  section_vaddresses, section_entsize,
					  num_sections, image_target);
      if (*start == 0)
	grub_util_error ("start symbol is not defined");
      
      /* Resolve addresses in the virtual address space.  */
      SUFFIX (relocate_addresses) (e, sections, section_addresses, section_entsize,
				   num_sections, strtab, image_target);
	  
      *reloc_size = SUFFIX (make_reloc_section) (e, reloc_section,
						 section_vaddresses, sections,
						 section_entsize, num_sections,
						 strtab, image_target);
    }
  else
    {
      *reloc_size = 0;
      *reloc_section = NULL;
    }

  out_img = xmalloc (*kernel_sz + total_module_size);

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
