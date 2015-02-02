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
# define Elf_Nhdr	Elf32_Nhdr
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
#define XEN_NOTE_SIZE 132
#elif defined(MKIMAGE_ELF64)
# define SUFFIX(x)	x ## 64
# define ELFCLASSXX	ELFCLASS64
# define Elf_Ehdr	Elf64_Ehdr
# define Elf_Phdr	Elf64_Phdr
# define Elf_Nhdr	Elf64_Nhdr
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
#define XEN_NOTE_SIZE 120
#else
#error "I'm confused"
#endif

static Elf_Addr SUFFIX (entry_point);

static void
SUFFIX (generate_elf) (const struct grub_install_image_target_desc *image_target,
		       int note, char **core_img, size_t *core_size,
		       Elf_Addr target_addr, grub_size_t align,
		       size_t kernel_size, size_t bss_size)
{
  char *elf_img;
  size_t program_size;
  Elf_Ehdr *ehdr;
  Elf_Phdr *phdr;
  Elf_Shdr *shdr;
  int header_size, footer_size = 0;
  int phnum = 1;
  int shnum = 4;
  int string_size = sizeof (".text") + sizeof ("mods") + 1;

  if (image_target->id != IMAGE_LOONGSON_ELF)
    phnum += 2;

  if (note)
    {
      phnum++;
      footer_size += sizeof (struct grub_ieee1275_note);
    }
  if (image_target->id == IMAGE_XEN)
    {
      phnum++;
      shnum++;
      string_size += sizeof (".xen");
      footer_size += XEN_NOTE_SIZE;
    }
  header_size = ALIGN_UP (sizeof (*ehdr) + phnum * sizeof (*phdr)
			  + shnum * sizeof (*shdr) + string_size, align);

  program_size = ALIGN_ADDR (*core_size);

  elf_img = xmalloc (program_size + header_size + footer_size);
  memset (elf_img, 0, program_size + header_size);
  memcpy (elf_img  + header_size, *core_img, *core_size);
  ehdr = (void *) elf_img;
  phdr = (void *) (elf_img + sizeof (*ehdr));
  shdr = (void *) (elf_img + sizeof (*ehdr) + phnum * sizeof (*phdr));
  memcpy (ehdr->e_ident, ELFMAG, SELFMAG);
  ehdr->e_ident[EI_CLASS] = ELFCLASSXX;
  if (!image_target->bigendian)
    ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
  else
    ehdr->e_ident[EI_DATA] = ELFDATA2MSB;
  ehdr->e_ident[EI_VERSION] = EV_CURRENT;
  ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
  ehdr->e_type = grub_host_to_target16 (ET_EXEC);
  ehdr->e_machine = grub_host_to_target16 (image_target->elf_target);
  ehdr->e_version = grub_host_to_target32 (EV_CURRENT);

  ehdr->e_phoff = grub_host_to_target32 ((char *) phdr - (char *) ehdr);
  ehdr->e_phentsize = grub_host_to_target16 (sizeof (*phdr));
  ehdr->e_phnum = grub_host_to_target16 (phnum);

  ehdr->e_shoff = grub_host_to_target32 ((grub_uint8_t *) shdr
					 - (grub_uint8_t *) ehdr);
  if (image_target->id == IMAGE_LOONGSON_ELF)
    ehdr->e_shentsize = grub_host_to_target16 (0);
  else
    ehdr->e_shentsize = grub_host_to_target16 (sizeof (Elf_Shdr));
  ehdr->e_shnum = grub_host_to_target16 (shnum);
  ehdr->e_shstrndx = grub_host_to_target16 (1);

  ehdr->e_ehsize = grub_host_to_target16 (sizeof (*ehdr));

  phdr->p_type = grub_host_to_target32 (PT_LOAD);
  phdr->p_offset = grub_host_to_target32 (header_size);
  phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);

  ehdr->e_entry = grub_host_to_target32 (target_addr);
  phdr->p_vaddr = grub_host_to_target32 (target_addr);
  phdr->p_paddr = grub_host_to_target32 (target_addr);
  phdr->p_align = grub_host_to_target32 (align > image_target->link_align ? align : image_target->link_align);
  if (image_target->id == IMAGE_LOONGSON_ELF)
    ehdr->e_flags = grub_host_to_target32 (0x1000 | EF_MIPS_NOREORDER 
					   | EF_MIPS_PIC | EF_MIPS_CPIC);
  else
    ehdr->e_flags = 0;
  if (image_target->id == IMAGE_LOONGSON_ELF)
    {
      phdr->p_filesz = grub_host_to_target32 (*core_size);
      phdr->p_memsz = grub_host_to_target32 (*core_size);
    }
  else
    {
      grub_uint32_t target_addr_mods;
      phdr->p_filesz = grub_host_to_target32 (kernel_size);
      phdr->p_memsz = grub_host_to_target32 (kernel_size + bss_size);

      phdr++;
      phdr->p_type = grub_host_to_target32 (PT_GNU_STACK);
      phdr->p_offset = grub_host_to_target32 (header_size + kernel_size);
      phdr->p_paddr = phdr->p_vaddr = phdr->p_filesz = phdr->p_memsz = 0;
      phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);
      phdr->p_align = grub_host_to_target32 (image_target->link_align);

      phdr++;
      phdr->p_type = grub_host_to_target32 (PT_LOAD);
      phdr->p_offset = grub_host_to_target32 (header_size + kernel_size);
      phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);
      phdr->p_filesz = phdr->p_memsz
	= grub_host_to_target32 (*core_size - kernel_size);

      if (image_target->id == IMAGE_COREBOOT)
	target_addr_mods = GRUB_KERNEL_I386_COREBOOT_MODULES_ADDR;
      else
	target_addr_mods = ALIGN_UP (target_addr + kernel_size + bss_size
				     + image_target->mod_gap,
				     image_target->mod_align);
      phdr->p_vaddr = grub_host_to_target_addr (target_addr_mods);
      phdr->p_paddr = grub_host_to_target_addr (target_addr_mods);
      phdr->p_align = grub_host_to_target32 (image_target->link_align);
    }

  if (image_target->id == IMAGE_XEN)
    {
      char *note_start = (elf_img + program_size + header_size);
      Elf_Nhdr *note_ptr;
      char *ptr = (char *) note_start;

      grub_util_info ("adding XEN NOTE segment");

      /* Guest OS.  */
      note_ptr = (Elf_Nhdr *) ptr;
      note_ptr->n_namesz = grub_host_to_target32 (sizeof (GRUB_XEN_NOTE_NAME));
      note_ptr->n_descsz = grub_host_to_target32 (sizeof (PACKAGE_NAME));
      note_ptr->n_type = grub_host_to_target32 (6);
      ptr += sizeof (Elf_Nhdr);
      memcpy (ptr, GRUB_XEN_NOTE_NAME, sizeof (GRUB_XEN_NOTE_NAME));
      ptr += ALIGN_UP (sizeof (GRUB_XEN_NOTE_NAME), 4);
      memcpy (ptr, PACKAGE_NAME, sizeof (PACKAGE_NAME));
      ptr += ALIGN_UP (sizeof (PACKAGE_NAME), 4);

      /* Loader.  */
      note_ptr = (Elf_Nhdr *) ptr;
      note_ptr->n_namesz = grub_host_to_target32 (sizeof (GRUB_XEN_NOTE_NAME));
      note_ptr->n_descsz = grub_host_to_target32 (sizeof ("generic"));
      note_ptr->n_type = grub_host_to_target32 (8);
      ptr += sizeof (Elf_Nhdr);
      memcpy (ptr, GRUB_XEN_NOTE_NAME, sizeof (GRUB_XEN_NOTE_NAME));
      ptr += ALIGN_UP (sizeof (GRUB_XEN_NOTE_NAME), 4);
      memcpy (ptr, "generic", sizeof ("generic"));
      ptr += ALIGN_UP (sizeof ("generic"), 4);

      /* Version.  */
      note_ptr = (Elf_Nhdr *) ptr;
      note_ptr->n_namesz = grub_host_to_target32 (sizeof (GRUB_XEN_NOTE_NAME));
      note_ptr->n_descsz = grub_host_to_target32 (sizeof ("xen-3.0"));
      note_ptr->n_type = grub_host_to_target32 (5);
      ptr += sizeof (Elf_Nhdr);
      memcpy (ptr, GRUB_XEN_NOTE_NAME, sizeof (GRUB_XEN_NOTE_NAME));
      ptr += ALIGN_UP (sizeof (GRUB_XEN_NOTE_NAME), 4);
      memcpy (ptr, "xen-3.0", sizeof ("xen-3.0"));
      ptr += ALIGN_UP (sizeof ("xen-3.0"), 4);

      /* Entry.  */
      note_ptr = (Elf_Nhdr *) ptr;
      note_ptr->n_namesz = grub_host_to_target32 (sizeof (GRUB_XEN_NOTE_NAME));
      note_ptr->n_descsz = grub_host_to_target32 (image_target->voidp_sizeof);
      note_ptr->n_type = grub_host_to_target32 (1);
      ptr += sizeof (Elf_Nhdr);
      memcpy (ptr, GRUB_XEN_NOTE_NAME, sizeof (GRUB_XEN_NOTE_NAME));
      ptr += ALIGN_UP (sizeof (GRUB_XEN_NOTE_NAME), 4);
      memset (ptr, 0, image_target->voidp_sizeof);
      ptr += image_target->voidp_sizeof;

      /* Virt base.  */
      note_ptr = (Elf_Nhdr *) ptr;
      note_ptr->n_namesz = grub_host_to_target32 (sizeof (GRUB_XEN_NOTE_NAME));
      note_ptr->n_descsz = grub_host_to_target32 (image_target->voidp_sizeof);
      note_ptr->n_type = grub_host_to_target32 (3);
      ptr += sizeof (Elf_Nhdr);
      memcpy (ptr, GRUB_XEN_NOTE_NAME, sizeof (GRUB_XEN_NOTE_NAME));
      ptr += ALIGN_UP (sizeof (GRUB_XEN_NOTE_NAME), 4);
      memset (ptr, 0, image_target->voidp_sizeof);
      ptr += image_target->voidp_sizeof;

      /* PAE.  */
      if (image_target->elf_target == EM_386)
	{
	  note_ptr = (Elf_Nhdr *) ptr;
	  note_ptr->n_namesz = grub_host_to_target32 (sizeof (GRUB_XEN_NOTE_NAME));
	  note_ptr->n_descsz = grub_host_to_target32 (sizeof ("yes,bimodal"));
	  note_ptr->n_type = grub_host_to_target32 (9);
	  ptr += sizeof (Elf_Nhdr);
	  memcpy (ptr, GRUB_XEN_NOTE_NAME, sizeof (GRUB_XEN_NOTE_NAME));
	  ptr += ALIGN_UP (sizeof (GRUB_XEN_NOTE_NAME), 4);
	  memcpy (ptr, "yes", sizeof ("yes"));
	  ptr += ALIGN_UP (sizeof ("yes"), 4);
	}

      assert (XEN_NOTE_SIZE == (ptr - note_start));

      phdr++;
      phdr->p_type = grub_host_to_target32 (PT_NOTE);
      phdr->p_flags = grub_host_to_target32 (PF_R);
      phdr->p_align = grub_host_to_target32 (image_target->voidp_sizeof);
      phdr->p_vaddr = 0;
      phdr->p_paddr = 0;
      phdr->p_filesz = grub_host_to_target32 (XEN_NOTE_SIZE);
      phdr->p_memsz = 0;
      phdr->p_offset = grub_host_to_target32 (header_size + program_size);
    }

  if (note)
    {
      int note_size = sizeof (struct grub_ieee1275_note);
      struct grub_ieee1275_note *note_ptr = (struct grub_ieee1275_note *) 
	(elf_img + program_size + header_size);

      grub_util_info ("adding CHRP NOTE segment");

      note_ptr->header.n_namesz = grub_host_to_target32 (sizeof (GRUB_IEEE1275_NOTE_NAME));
      note_ptr->header.n_descsz = grub_host_to_target32 (note_size);
      note_ptr->header.n_type = grub_host_to_target32 (GRUB_IEEE1275_NOTE_TYPE);
      strcpy (note_ptr->name, GRUB_IEEE1275_NOTE_NAME);
      note_ptr->descriptor.real_mode = grub_host_to_target32 (0xffffffff);
      note_ptr->descriptor.real_base = grub_host_to_target32 (0x00c00000);
      note_ptr->descriptor.real_size = grub_host_to_target32 (0xffffffff);
      note_ptr->descriptor.virt_base = grub_host_to_target32 (0xffffffff);
      note_ptr->descriptor.virt_size = grub_host_to_target32 (0xffffffff);
      note_ptr->descriptor.load_base = grub_host_to_target32 (0x00004000);

      phdr++;
      phdr->p_type = grub_host_to_target32 (PT_NOTE);
      phdr->p_flags = grub_host_to_target32 (PF_R);
      phdr->p_align = grub_host_to_target32 (image_target->voidp_sizeof);
      phdr->p_vaddr = 0;
      phdr->p_paddr = 0;
      phdr->p_filesz = grub_host_to_target32 (note_size);
      phdr->p_memsz = 0;
      phdr->p_offset = grub_host_to_target32 (header_size + program_size);
    }

  {
    char *str_start = (elf_img + sizeof (*ehdr) + phnum * sizeof (*phdr)
		       + shnum * sizeof (*shdr));
    char *ptr = str_start + 1;

    shdr++;

    shdr->sh_name = grub_host_to_target32 (0);
    shdr->sh_type = grub_host_to_target32 (SHT_STRTAB);
    shdr->sh_addr = grub_host_to_target_addr (0);
    shdr->sh_offset = grub_host_to_target_addr (str_start - elf_img);
    shdr->sh_size = grub_host_to_target32 (string_size);
    shdr->sh_link = grub_host_to_target32 (0);
    shdr->sh_info = grub_host_to_target32 (0);
    shdr->sh_addralign = grub_host_to_target32 (align);
    shdr->sh_entsize = grub_host_to_target32 (0);
    shdr++;

    memcpy (ptr, ".text", sizeof (".text"));

    shdr->sh_name = grub_host_to_target32 (ptr - str_start);
    ptr += sizeof (".text");
    shdr->sh_type = grub_host_to_target32 (SHT_PROGBITS);
    shdr->sh_addr = grub_host_to_target_addr (target_addr);
    shdr->sh_offset = grub_host_to_target_addr (header_size);
    shdr->sh_size = grub_host_to_target32 (kernel_size);
    shdr->sh_link = grub_host_to_target32 (0);
    shdr->sh_info = grub_host_to_target32 (0);
    shdr->sh_addralign = grub_host_to_target32 (align);
    shdr->sh_entsize = grub_host_to_target32 (0);
    shdr++;

    memcpy (ptr, "mods", sizeof ("mods"));
    shdr->sh_name = grub_host_to_target32 (ptr - str_start);
    ptr += sizeof ("mods");
    shdr->sh_type = grub_host_to_target32 (SHT_PROGBITS);
    shdr->sh_addr = grub_host_to_target_addr (target_addr + kernel_size);
    shdr->sh_offset = grub_host_to_target_addr (header_size + kernel_size);
    shdr->sh_size = grub_host_to_target32 (*core_size - kernel_size);
    shdr->sh_link = grub_host_to_target32 (0);
    shdr->sh_info = grub_host_to_target32 (0);
    shdr->sh_addralign = grub_host_to_target32 (image_target->voidp_sizeof);
    shdr->sh_entsize = grub_host_to_target32 (0);
    shdr++;

    if (image_target->id == IMAGE_XEN)
      {
	memcpy (ptr, ".xen", sizeof (".xen"));
	shdr->sh_name = grub_host_to_target32 (ptr - str_start);
	ptr += sizeof (".xen");
	shdr->sh_type = grub_host_to_target32 (SHT_PROGBITS);
	shdr->sh_addr = grub_host_to_target_addr (target_addr + kernel_size);
	shdr->sh_offset = grub_host_to_target_addr (program_size + header_size);
	shdr->sh_size = grub_host_to_target32 (XEN_NOTE_SIZE);
	shdr->sh_link = grub_host_to_target32 (0);
	shdr->sh_info = grub_host_to_target32 (0);
	shdr->sh_addralign = grub_host_to_target32 (image_target->voidp_sizeof);
	shdr->sh_entsize = grub_host_to_target32 (0);
	shdr++;
      }
  }

  free (*core_img);
  *core_img = elf_img;
  *core_size = program_size + header_size + footer_size;
}

/* Relocate symbols; note that this function overwrites the symbol table.
   Return the address of a start symbol.  */
static Elf_Addr
SUFFIX (relocate_symbols) (Elf_Ehdr *e, Elf_Shdr *sections,
			   Elf_Shdr *symtab_section, Elf_Addr *section_addresses,
			   Elf_Half section_entsize, Elf_Half num_sections,
			   void *jumpers, Elf_Addr jumpers_addr,
			   const struct grub_install_image_target_desc *image_target)
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
      else if (cur_index == STN_UNDEF)
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
	  *jptr = grub_host_to_target64 (sym->st_value);
	  sym->st_value = (char *) jptr - (char *) jumpers + jumpers_addr;
	  jptr++;
	  *jptr = 0;
	  jptr++;
	}
      grub_util_info ("locating %s at 0x%"  GRUB_HOST_PRIxLONG_LONG
		      " (0x%"  GRUB_HOST_PRIxLONG_LONG ")", name,
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
			     const struct grub_install_image_target_desc *image_target)
{
  Elf_Sym *sym;

  sym = (Elf_Sym *) ((char *) e
		       + grub_target_to_host (s->sh_offset)
		       + i * grub_target_to_host (s->sh_entsize));
  return sym->st_value;
}

/* Return the address of a modified value.  */
static Elf_Addr *
SUFFIX (get_target_address) (Elf_Ehdr *e, Elf_Shdr *s, Elf_Addr offset,
		    const struct grub_install_image_target_desc *image_target)
{
  return (Elf_Addr *) ((char *) e + grub_target_to_host (s->sh_offset) + offset);
}

#ifdef MKIMAGE_ELF64
static Elf_Addr
SUFFIX (count_funcs) (Elf_Ehdr *e, Elf_Shdr *symtab_section,
		      const struct grub_install_image_target_desc *image_target)
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

#ifdef MKIMAGE_ELF32
/* Deal with relocation information. This function relocates addresses
   within the virtual address space starting from 0. So only relative
   addresses can be fully resolved. Absolute addresses must be relocated
   again by a PE32 relocator when loaded.  */
static grub_size_t
arm_get_trampoline_size (Elf_Ehdr *e,
			 Elf_Shdr *sections,
			 Elf_Half section_entsize,
			 Elf_Half num_sections,
			 const struct grub_install_image_target_desc *image_target)
{
  Elf_Half i;
  Elf_Shdr *s;
  grub_size_t ret = 0;

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
	Elf_Word j;

	symtab_section = (Elf_Shdr *) ((char *) sections
					 + (grub_target_to_host32 (s->sh_link)
					    * section_entsize));

	rtab_size = grub_target_to_host (s->sh_size);
	r_size = grub_target_to_host (s->sh_entsize);
	rtab_offset = grub_target_to_host (s->sh_offset);
	num_rs = rtab_size / r_size;

	for (j = 0, r = (Elf_Rela *) ((char *) e + rtab_offset);
	     j < num_rs;
	     j++, r = (Elf_Rela *) ((char *) r + r_size))
	  {
            Elf_Addr info;
	    Elf_Addr sym_addr;

	    info = grub_target_to_host (r->r_info);
	    sym_addr = SUFFIX (get_symbol_address) (e, symtab_section,
						    ELF_R_SYM (info), image_target);

            sym_addr += (s->sh_type == grub_target_to_host32 (SHT_RELA)) ?
	      grub_target_to_host (r->r_addend) : 0;

	    switch (ELF_R_TYPE (info))
	      {
	      case R_ARM_ABS32:
	      case R_ARM_V4BX:
		break;
	      case R_ARM_THM_CALL:
	      case R_ARM_THM_JUMP24:
	      case R_ARM_THM_JUMP19:
		if (!(sym_addr & 1))
		  ret += 8;
		break;

	      case R_ARM_CALL:
	      case R_ARM_JUMP24:
		if (sym_addr & 1)
		  ret += 16;
		break;

	      default:
		grub_util_error (_("relocation 0x%x is not implemented yet"),
				 (unsigned int) ELF_R_TYPE (info));
		break;
	      }
	  }
      }
  return ret;
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
			     const struct grub_install_image_target_desc *image_target)
{
  Elf_Half i;
  Elf_Shdr *s;
#ifdef MKIMAGE_ELF64
  struct grub_ia64_trampoline *tr = (void *) (pe_target + tramp_off);
  grub_uint64_t *gpptr = (void *) (pe_target + got_off);
#define MASK19 ((1 << 19) - 1)
#else
  grub_uint32_t *tr = (void *) (pe_target + tramp_off);
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

	rtab_size = grub_target_to_host (s->sh_size);
	r_size = grub_target_to_host (s->sh_entsize);
	rtab_offset = grub_target_to_host (s->sh_offset);
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
	      grub_target_to_host (r->r_addend) : 0;

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
		  grub_util_info ("relocating an R_386_32 entry to 0x%"
				  GRUB_HOST_PRIxLONG_LONG " at the offset 0x%"
				  GRUB_HOST_PRIxLONG_LONG,
				  (unsigned long long) *target,
				  (unsigned long long) offset);
		  break;

		case R_386_PC32:
		  /* This is relative.  */
		  *target = grub_host_to_target32 (grub_target_to_host32 (*target)
						   + addend + sym_addr
						   - target_section_addr - offset
						   - image_target->vaddr_offset);
		  grub_util_info ("relocating an R_386_PC32 entry to 0x%"
				  GRUB_HOST_PRIxLONG_LONG " at the offset 0x%"
				  GRUB_HOST_PRIxLONG_LONG,
				  (unsigned long long) *target,
				  (unsigned long long) offset);
		  break;
		default:
		  grub_util_error (_("relocation 0x%x is not implemented yet"),
				   (unsigned int) ELF_R_TYPE (info));
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
		  grub_util_info ("relocating an R_X86_64_64 entry to 0x%"
				  GRUB_HOST_PRIxLONG_LONG " at the offset 0x%"
				  GRUB_HOST_PRIxLONG_LONG,
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
		    grub_util_info ("relocating an R_X86_64_PC32 entry to 0x%x at the offset 0x%"
				    GRUB_HOST_PRIxLONG_LONG,
				    *t32, (unsigned long long) offset);
		    break;
		  }

		case R_X86_64_PC64:
		  {
		    *target = grub_host_to_target64 (grub_target_to_host64 (*target)
						     + addend + sym_addr
						     - target_section_addr - offset
						     - image_target->vaddr_offset);
		    grub_util_info ("relocating an R_X86_64_PC64 entry to 0x%"
				    GRUB_HOST_PRIxLONG_LONG " at the offset 0x%"
				    GRUB_HOST_PRIxLONG_LONG,
				    (unsigned long long) *target,
				    (unsigned long long) offset);
		    break;
		  }

		case R_X86_64_32:
		case R_X86_64_32S:
		  {
		    grub_uint32_t *t32 = (grub_uint32_t *) target;
		    *t32 = grub_host_to_target64 (grub_target_to_host32 (*t32)
						  + addend + sym_addr);
		    grub_util_info ("relocating an R_X86_64_32(S) entry to 0x%x at the offset 0x%"
				    GRUB_HOST_PRIxLONG_LONG,
				    *t32, (unsigned long long) offset);
		    break;
		  }

		default:
		  grub_util_error (_("relocation 0x%x is not implemented yet"),
				   (unsigned int) ELF_R_TYPE (info));
		  break;
		}
	      break;
	     case EM_IA_64:
	      switch (ELF_R_TYPE (info))
		{
		case R_IA64_PCREL21B:
		  {
		    grub_uint64_t noff;
		    grub_ia64_make_trampoline (tr, addend + sym_addr);
		    noff = ((char *) tr - (char *) pe_target
			    - target_section_addr - (offset & ~3)) >> 4;
		    tr++;
		    if (noff & ~MASK19)
		      grub_util_error ("trampoline offset too big (%"
				       GRUB_HOST_PRIxLONG_LONG ")",
				       (unsigned long long) noff);
		    grub_ia64_add_value_to_slot_20b ((grub_addr_t) target, noff);
		  }
		  break;

		case R_IA64_LTOFF22X:
		case R_IA64_LTOFF22:
		  {
		    Elf_Sym *sym;

		    sym = (Elf_Sym *) ((char *) e
				       + grub_target_to_host (symtab_section->sh_offset)
				       + ELF_R_SYM (info) * grub_target_to_host (symtab_section->sh_entsize));
		    if (ELF_ST_TYPE (sym->st_info) == STT_FUNC)
		      sym_addr = grub_target_to_host64 (*(grub_uint64_t *) (pe_target
									    + sym->st_value
									    - image_target->vaddr_offset));
		  }
		case R_IA64_LTOFF_FPTR22:
		  *gpptr = grub_host_to_target64 (addend + sym_addr);
		  grub_ia64_add_value_to_slot_21 ((grub_addr_t) target,
						  (char *) gpptr - (char *) pe_target
						  + image_target->vaddr_offset);
		  gpptr++;
		  break;

		case R_IA64_GPREL22:
		  grub_ia64_add_value_to_slot_21 ((grub_addr_t) target,
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
				  GRUB_HOST_PRIxLONG_LONG " at the offset 0x%"
				  GRUB_HOST_PRIxLONG_LONG,
				  (unsigned long long)
				  grub_target_to_host64 (*target),
				  (unsigned long long) offset);
		  break;

		  /* We treat LTOFF22X as LTOFF22, so we can ignore LDXMOV.  */
		case R_IA64_LDXMOV:
		  break;

		default:
		  grub_util_error (_("relocation 0x%x is not implemented yet"),
				   (unsigned int) ELF_R_TYPE (info));
		  break;
		}
	       break;
	     case EM_AARCH64:
	       {
		 sym_addr += addend;
		 switch (ELF_R_TYPE (info))
		   {
		   case R_AARCH64_ABS64:
		     {
		       *target = grub_host_to_target64 (grub_target_to_host64 (*target) + sym_addr);
		     }
		     break;
		   case R_AARCH64_JUMP26:
		   case R_AARCH64_CALL26:
		     {
		       sym_addr -= offset;
		       sym_addr -= SUFFIX (entry_point);
		       if (!grub_arm_64_check_xxxx26_offset (sym_addr))
			 grub_util_error ("%s", "CALL26 Relocation out of range");

		       grub_arm64_set_xxxx26_offset((grub_uint32_t *)target,
						     sym_addr);
		     }
		     break;
		   default:
		     grub_util_error (_("relocation 0x%x is not implemented yet"),
				      (unsigned int) ELF_R_TYPE (info));
		     break;
		   }
	       break;
	       }
#endif
#if defined(MKIMAGE_ELF32)
	     case EM_ARM:
	       {
		 sym_addr += addend;
		 sym_addr -= SUFFIX (entry_point);
		 switch (ELF_R_TYPE (info))
		   {
		   case R_ARM_ABS32:
		     {
		       grub_util_info ("  ABS32:\toffset=%d\t(0x%08x)",
				       (int) sym_addr, (int) sym_addr);
		       /* Data will be naturally aligned */
		       sym_addr += 0x400;
		       *target = grub_host_to_target32 (grub_target_to_host32 (*target) + sym_addr);
		     }
		     break;
		     /* Happens when compiled with -march=armv4.
			Since currently we need at least armv5, keep bx as-is.
		     */
		   case R_ARM_V4BX:
		     break;
		   case R_ARM_THM_CALL:
		   case R_ARM_THM_JUMP24:
		   case R_ARM_THM_JUMP19:
		     {
		       grub_err_t err;
		       grub_util_info ("  THM_JUMP24:\ttarget=0x%08lx\toffset=(0x%08x)",
				       (unsigned long) ((char *) target
							- (char *) e),
				       sym_addr);
		       if (!(sym_addr & 1))
			 {
			   grub_uint32_t tr_addr;
			   grub_int32_t new_offset;
			   tr_addr = (char *) tr - (char *) pe_target
			     - target_section_addr;
			   new_offset = sym_addr - tr_addr - 12;

			   if (!grub_arm_jump24_check_offset (new_offset))
			     return grub_util_error ("jump24 relocation out of range");

			   tr[0] = grub_host_to_target32 (0x46c04778); /* bx pc; nop  */
			   tr[1] = grub_host_to_target32 (((new_offset >> 2) & 0xffffff) | 0xea000000); /* b new_offset */
			   tr += 2;
			   sym_addr = tr_addr | 1;
			 }
		       sym_addr -= offset;
		       /* Thumb instructions can be 16-bit aligned */
		       if (ELF_R_TYPE (info) == R_ARM_THM_JUMP19)
			 err = grub_arm_reloc_thm_jump19 ((grub_uint16_t *) target, sym_addr);
		       else
			 err = grub_arm_reloc_thm_call ((grub_uint16_t *) target,
							sym_addr);
		       if (err)
			 grub_util_error ("%s", grub_errmsg);
		     }
		     break;

		   case R_ARM_CALL:
		   case R_ARM_JUMP24:
		     {
		       grub_err_t err;
		       grub_util_info ("  JUMP24:\ttarget=0x%08lx\toffset=(0x%08x)",  (unsigned long) ((char *) target - (char *) e), sym_addr);
		       if (sym_addr & 1)
			 {
			   grub_uint32_t tr_addr;
			   grub_int32_t new_offset;
			   tr_addr = (char *) tr - (char *) pe_target
			     - target_section_addr;
			   new_offset = sym_addr - tr_addr - 12;

			   /* There is no immediate version of bx, only register one...  */
			   tr[0] = grub_host_to_target32 (0xe59fc004); /* ldr	ip, [pc, #4] */
			   tr[1] = grub_host_to_target32 (0xe08cc00f); /* add	ip, ip, pc */
			   tr[2] = grub_host_to_target32 (0xe12fff1c); /* bx	ip */
			   tr[3] = grub_host_to_target32 (new_offset | 1);
			   tr += 4;
			   sym_addr = tr_addr;
			 }
		       sym_addr -= offset;
		       err = grub_arm_reloc_jump24 (target,
						    sym_addr);
		       if (err)
			 grub_util_error ("%s", grub_errmsg);
		     }
		     break;

		   default:
		     grub_util_error (_("relocation 0x%x is not implemented yet"),
				      (unsigned int) ELF_R_TYPE (info));
		     break;
		   }
		 break;
	       }
#endif /* MKIMAGE_ELF32 */
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
			  const struct grub_install_image_target_desc *image_target)
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
			     const struct grub_install_image_target_desc *image_target)
{
  unsigned i;
  Elf_Shdr *s;
  struct fixup_block_list *lst, *lst0;
  Elf_Addr current_address = 0;

  lst = lst0 = xmalloc (sizeof (*lst) + 2 * 0x1000);
  memset (lst, 0, sizeof (*lst) + 2 * 0x1000);

  for (i = 0, s = sections; i < num_sections;
       i++, s = (Elf_Shdr *) ((char *) s + section_entsize))
    if ((grub_target_to_host32 (s->sh_type) == SHT_REL) ||
        (grub_target_to_host32 (s->sh_type) == SHT_RELA))
      {
	Elf_Rel *r;
	Elf_Word rtab_size, r_size, num_rs;
	Elf_Off rtab_offset;
	Elf_Addr section_address;
	Elf_Word j;

	grub_util_info ("translating the relocation section %s",
			strtab + grub_le_to_cpu32 (s->sh_name));

	rtab_size = grub_target_to_host (s->sh_size);
	r_size = grub_target_to_host (s->sh_entsize);
	rtab_offset = grub_target_to_host (s->sh_offset);
	num_rs = rtab_size / r_size;

	section_address = section_addresses[grub_le_to_cpu32 (s->sh_info)];

	for (j = 0, r = (Elf_Rel *) ((char *) e + rtab_offset);
	     j < num_rs;
	     j++, r = (Elf_Rel *) ((char *) r + r_size))
	  {
	    Elf_Addr info;
	    Elf_Addr offset;

	    offset = grub_target_to_host (r->r_offset);
	    info = grub_target_to_host (r->r_info);

	    /* Necessary to relocate only absolute addresses.  */
	    switch (image_target->elf_target)
	      {
	      case EM_386:
		if (ELF_R_TYPE (info) == R_386_32)
		  {
		    Elf_Addr addr;

		    addr = section_address + offset;
		    grub_util_info ("adding a relocation entry for 0x%"
				    GRUB_HOST_PRIxLONG_LONG,
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
		    grub_util_info ("adding a relocation entry for 0x%"
				    GRUB_HOST_PRIxLONG_LONG,
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
		    grub_util_info ("adding a relocation entry for 0x%"
				    GRUB_HOST_PRIxLONG_LONG,
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
				   (unsigned int) ELF_R_TYPE (info));
		  break;
		}
		break;
	      case EM_AARCH64:
		switch (ELF_R_TYPE (info))
		  {
		  case R_AARCH64_ABS64:
		    {
		      Elf_Addr addr;

		      addr = section_address + offset;
		      current_address
			= SUFFIX (add_fixup_entry) (&lst,
						    GRUB_PE32_REL_BASED_DIR64,
						    addr, 0, current_address,
						    image_target);
		    }
		    break;
		    /* Relative relocations do not require fixup entries. */
		  case R_AARCH64_CALL26:
		  case R_AARCH64_JUMP26:
		    break;
		  default:
		    grub_util_error (_("relocation 0x%x is not implemented yet"),
				     (unsigned int) ELF_R_TYPE (info));
		    break;
		  }
		break;
		break;
#if defined(MKIMAGE_ELF32)
	      case EM_ARM:
		switch (ELF_R_TYPE (info))
		  {
		  case R_ARM_V4BX:
		    /* Relative relocations do not require fixup entries. */
		  case R_ARM_JUMP24:
		  case R_ARM_THM_CALL:
		  case R_ARM_THM_JUMP19:
		  case R_ARM_THM_JUMP24:
		  case R_ARM_CALL:
		    {
		      Elf_Addr addr;

		      addr = section_address + offset;
		      grub_util_info ("  %s:  not adding fixup: 0x%08x : 0x%08x", __FUNCTION__, (unsigned int) addr, (unsigned int) current_address);
		    }
		    break;
		    /* Create fixup entry for PE/COFF loader */
		  case R_ARM_ABS32:
		    {
		      Elf_Addr addr;

		      addr = section_address + offset;
		      current_address
			= SUFFIX (add_fixup_entry) (&lst,
						    GRUB_PE32_REL_BASED_HIGHLOW,
						    addr, 0, current_address,
						    image_target);
		    }
		    break;
		  default:
		    grub_util_error (_("relocation 0x%x is not implemented yet"),
				     (unsigned int) ELF_R_TYPE (info));
		    break;
		  }
		break;
#endif /* defined(MKIMAGE_ELF32) */
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

  for (lst = lst0; lst; )
    {
      struct fixup_block_list *next;
      next = lst->next;
      free (lst);
      lst = next;
    }

  return current_address;
}

/* Determine if this section is a text section. Return false if this
   section is not allocated.  */
static int
SUFFIX (is_text_section) (Elf_Shdr *s, const struct grub_install_image_target_desc *image_target)
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
SUFFIX (is_data_section) (Elf_Shdr *s, const struct grub_install_image_target_desc *image_target)
{
  if (image_target->id != IMAGE_EFI 
      && grub_target_to_host32 (s->sh_type) != SHT_PROGBITS)
    return 0;
  return ((grub_target_to_host (s->sh_flags) & (SHF_EXECINSTR | SHF_ALLOC))
	  == SHF_ALLOC);
}

/* Return if the ELF header is valid.  */
static int
SUFFIX (check_elf_header) (Elf_Ehdr *e, size_t size, const struct grub_install_image_target_desc *image_target)
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
SUFFIX (locate_sections) (const char *kernel_path,
			  Elf_Shdr *sections, Elf_Half section_entsize,
			  Elf_Half num_sections, const char *strtab,
			  size_t *exec_size, size_t *kernel_sz,
			  size_t *all_align,
			  const struct grub_install_image_target_desc *image_target)
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
	grub_util_info ("locating the section %s at 0x%"
			GRUB_HOST_PRIxLONG_LONG,
			name, (unsigned long long) current_address);
	if (image_target->id != IMAGE_EFI)
	  {
	    current_address = grub_host_to_target_addr (s->sh_addr)
	      - image_target->link_addr;
	    if (grub_host_to_target_addr (s->sh_addr)
		!= image_target->link_addr)
	      {
		char *msg
		  = grub_xasprintf (_("`%s' is miscompiled: its start address is 0x%llx"
				      " instead of 0x%llx: ld.gold bug?"),
				    kernel_path,
				    (unsigned long long) grub_host_to_target_addr (s->sh_addr),
				    (unsigned long long) image_target->link_addr);
		grub_util_error ("%s", msg);
	      }
	  }
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

	grub_util_info ("locating the section %s at 0x%"
			GRUB_HOST_PRIxLONG_LONG,
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
SUFFIX (load_image) (const char *kernel_path, size_t *exec_size, 
		     size_t *kernel_sz, size_t *bss_size,
		     size_t total_module_size, grub_uint64_t *start,
		     void **reloc_section, size_t *reloc_size,
		     size_t *align,
		     const struct grub_install_image_target_desc *image_target)
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
  grub_size_t ia64jmp_off = 0, tramp_off = 0, ia64_got_off = 0;
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

  section_addresses = SUFFIX (locate_sections) (kernel_path,
						sections, section_entsize,
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
	
	    grub_util_info ("locating the section %s at 0x%"
			    GRUB_HOST_PRIxLONG_LONG,
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

  if (image_target->id == IMAGE_SPARC64_AOUT
      || image_target->id == IMAGE_SPARC64_RAW
      || image_target->id == IMAGE_SPARC64_CDCORE)
    *kernel_sz = ALIGN_UP (*kernel_sz, image_target->mod_align);

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
	grub_util_error ("%s", _("no symbol table"));

#ifdef MKIMAGE_ELF32
      if (image_target->elf_target == EM_ARM)
	{
	  grub_size_t tramp;

	  *kernel_sz = ALIGN_UP (*kernel_sz, 16);

	  tramp = arm_get_trampoline_size (e, sections, section_entsize,
					   num_sections, image_target);

	  tramp_off = *kernel_sz;
	  *kernel_sz += ALIGN_UP (tramp, 16);
	}
#endif

#ifdef MKIMAGE_ELF64
      if (image_target->elf_target == EM_IA_64)
	{
	  grub_size_t tramp;

	  *kernel_sz = ALIGN_UP (*kernel_sz, 16);

	  grub_ia64_dl_get_tramp_got_size (e, &tramp, &got);

	  tramp_off = *kernel_sz;
	  *kernel_sz += ALIGN_UP (tramp, 16);

	  ia64jmp_off = *kernel_sz;
	  ia64jmpnum = SUFFIX (count_funcs) (e, symtab_section,
					     image_target);
	  *kernel_sz += 16 * ia64jmpnum;

	  ia64_got_off = *kernel_sz;
	  *kernel_sz += ALIGN_UP (got, 16);
	}
#endif

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

      SUFFIX (entry_point) = (Elf_Addr) *start;

      /* Resolve addresses in the virtual address space.  */
      SUFFIX (relocate_addresses) (e, sections, section_addresses, 
				   section_entsize,
				   num_sections, strtab,
				   out_img, tramp_off, ia64_got_off,
				   image_target);

      *reloc_size = SUFFIX (make_reloc_section) (e, reloc_section,
						 section_vaddresses, sections,
						 section_entsize, num_sections,
						 strtab, ia64jmp_off
						 + image_target->vaddr_offset,
						 2 * ia64jmpnum + (got / 8),
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

  free (section_vaddresses);
  free (section_addresses);

  return out_img;
}


#undef SUFFIX
#undef ELFCLASSXX
#undef Elf_Ehdr
#undef Elf_Phdr
#undef Elf_Nhdr
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
#undef XEN_NOTE_SIZE
