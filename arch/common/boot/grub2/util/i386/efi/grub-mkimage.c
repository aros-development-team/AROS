/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2005,2006,2007  Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grub/elf.h>
#include <grub/util/misc.h>
#include <grub/util/resolve.h>
#include <grub/kernel.h>
#include <grub/efi/pe32.h>
#include <grub/machine/kernel.h>

static const grub_uint8_t stub[] = GRUB_PE32_MSDOS_STUB;

static inline Elf32_Addr
align_address (Elf32_Addr addr, unsigned alignment)
{
  return (addr + alignment - 1) & ~(alignment - 1);
}

static inline Elf32_Addr
align_pe32_section (Elf32_Addr addr)
{
  return align_address (addr, GRUB_PE32_SECTION_ALIGNMENT);
}

/* Read the whole kernel image. Return the pointer to a read image,
   and store the size in bytes in *SIZE.  */
static char *
read_kernel_module (const char *dir, char *prefix, size_t *size)
{
  char *kernel_image;
  char *kernel_path;
  
  kernel_path = grub_util_get_path (dir, "kernel.mod");
  *size = grub_util_get_image_size (kernel_path);
  kernel_image = grub_util_read_image (kernel_path);
  free (kernel_path);

  if (GRUB_KERNEL_MACHINE_PREFIX + strlen (prefix) + 1 > GRUB_KERNEL_MACHINE_DATA_END)
    grub_util_error ("prefix too long");
  strcpy (kernel_image + 0x34 + GRUB_KERNEL_MACHINE_PREFIX, prefix);

  return kernel_image;
}

/* Return if the ELF header is valid.  */
static int
check_elf_header (Elf32_Ehdr *e, size_t size)
{
  if (size < sizeof (*e)
      || e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_version != grub_cpu_to_le32 (EV_CURRENT)
      || e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != grub_cpu_to_le16 (EM_386))
    return 0;

  return 1;
}

/* Return the starting address right after the header,
   aligned by the section alignment. Allocate 4 section tables for
   .text, .data, .reloc, and mods.  */
static Elf32_Addr
get_starting_section_address (void)
{
  return align_pe32_section (sizeof (struct grub_pe32_header)
			     + 4 * sizeof (struct grub_pe32_section_table));
}

/* Determine if this section is a text section. Return false if this
   section is not allocated.  */
static int
is_text_section (Elf32_Shdr *s)
{
  return ((s->sh_flags & grub_cpu_to_le32 (SHF_EXECINSTR | SHF_ALLOC))
	  == grub_cpu_to_le32 (SHF_EXECINSTR | SHF_ALLOC));
}

/* Determine if this section is a data section. This assumes that
   BSS is also a data section, since the converter initializes BSS
   when producing PE32 to avoid a bug in EFI implementations.  */
static int
is_data_section (Elf32_Shdr *s)
{
  return (s->sh_flags & grub_cpu_to_le32 (SHF_ALLOC)
	  && ! (s->sh_flags & grub_cpu_to_le32 (SHF_EXECINSTR)));
}

/* Locate section addresses by merging code sections and data sections
   into .text and .data, respectively. Return the array of section
   addresses.  */
static Elf32_Addr *
locate_sections (Elf32_Shdr *sections, Elf32_Half section_entsize,
		 Elf32_Half num_sections, const char *strtab)
{
  int i;
  Elf32_Addr current_address;
  Elf32_Addr *section_addresses;
  Elf32_Shdr *s;
  
  section_addresses = xmalloc (sizeof (*section_addresses) * num_sections);
  memset (section_addresses, 0, sizeof (*section_addresses) * num_sections);

  current_address = get_starting_section_address ();

  /* .text */
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf32_Shdr *) ((char *) s + section_entsize))
    if (is_text_section (s))
      {
	Elf32_Word align = grub_le_to_cpu32 (s->sh_addralign);
	const char *name = strtab + grub_le_to_cpu32 (s->sh_name);
	
	if (align)
	  current_address = align_address (current_address, align);
	
	grub_util_info ("locating the section %s at 0x%x",
			name, current_address);
	section_addresses[i] = current_address;
	current_address += grub_le_to_cpu32 (s->sh_size);
      }

  current_address = align_pe32_section (current_address);

  /* .data */
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf32_Shdr *) ((char *) s + section_entsize))
    if (is_data_section (s))
      {
	Elf32_Word align = grub_le_to_cpu32 (s->sh_addralign);
	const char *name = strtab + grub_le_to_cpu32 (s->sh_name);
	
	if (align)
	  current_address = align_address (current_address, align);
	
	grub_util_info ("locating the section %s at 0x%x",
			name, current_address);
	section_addresses[i] = current_address;
	current_address += grub_le_to_cpu32 (s->sh_size);
      }

  return section_addresses;
}

/* Return the symbol table section, if any.  */
static Elf32_Shdr *
find_symtab_section (Elf32_Shdr *sections,
		     Elf32_Half section_entsize, Elf32_Half num_sections)
{
  int i;
  Elf32_Shdr *s;
  
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf32_Shdr *) ((char *) s + section_entsize))
    if (s->sh_type == grub_cpu_to_le32 (SHT_SYMTAB))
      return s;

  return 0;
}

/* Return the address of the string table.  */
static const char *
find_strtab (Elf32_Ehdr *e, Elf32_Shdr *sections, Elf32_Half section_entsize)
{
  Elf32_Shdr *s;
  char *strtab;
  
  s = (Elf32_Shdr *) ((char *) sections
		      + grub_le_to_cpu16 (e->e_shstrndx) * section_entsize);
  strtab = (char *) e + grub_le_to_cpu32 (s->sh_offset);
  return strtab;
}

/* Relocate symbols; note that this function overwrites the symbol table.
   Return the address of a start symbol.  */
static Elf32_Addr
relocate_symbols (Elf32_Ehdr *e, Elf32_Shdr *sections,
		  Elf32_Shdr *symtab_section, Elf32_Addr *section_addresses,
		  Elf32_Half section_entsize, Elf32_Half num_sections)
{
  Elf32_Word symtab_size, sym_size, num_syms;
  Elf32_Off symtab_offset;
  Elf32_Addr start_address = 0;
  Elf32_Sym *sym;
  Elf32_Word i;
  Elf32_Shdr *strtab_section;
  const char *strtab;
  
  strtab_section
    = (Elf32_Shdr *) ((char *) sections
		      + (grub_le_to_cpu32 (symtab_section->sh_link)
			 * section_entsize));
  strtab = (char *) e + grub_le_to_cpu32 (strtab_section->sh_offset);
  
  symtab_size = grub_le_to_cpu32 (symtab_section->sh_size);
  sym_size = grub_le_to_cpu32 (symtab_section->sh_entsize);
  symtab_offset = grub_le_to_cpu32 (symtab_section->sh_offset);
  num_syms = symtab_size / sym_size;

  for (i = 0, sym = (Elf32_Sym *) ((char *) e + symtab_offset);
       i < num_syms;
       i++, sym = (Elf32_Sym *) ((char *) sym + sym_size))
    {
      Elf32_Section index;
      const char *name;
      
      name = strtab + grub_le_to_cpu32 (sym->st_name);
      
      index = grub_le_to_cpu16 (sym->st_shndx);
      if (index == STN_UNDEF)
	{
	  if (sym->st_name)
	    grub_util_error ("undefined symbol %s", name);
	  else
	    continue;
	}
      else if (index >= num_sections)
	grub_util_error ("section %d does not exist", index);
      
      sym->st_value = (grub_le_to_cpu32 (sym->st_value)
		       + section_addresses[index]);
      grub_util_info ("locating %s at 0x%x", name, sym->st_value);

      if (! start_address)
	if (strcmp (name, "_start") == 0 || strcmp (name, "start") == 0)
	  start_address = sym->st_value;
    }

  return start_address;
}

/* Return the address of a symbol at the index I in the section S.  */
static Elf32_Addr
get_symbol_address (Elf32_Ehdr *e, Elf32_Shdr *s, Elf32_Word i)
{
  Elf32_Sym *sym;
  
  sym = (Elf32_Sym *) ((char *) e
		       + grub_le_to_cpu32 (s->sh_offset)
		       + i * grub_le_to_cpu32 (s->sh_entsize));
  return sym->st_value;
}

/* Return the address of a modified value.  */
static Elf32_Addr
get_target_address (Elf32_Ehdr *e, Elf32_Shdr *s, Elf32_Addr offset)
{
  return (Elf32_Addr) e + grub_le_to_cpu32 (s->sh_offset) + offset;
}

/* Deal with relocation information. This function relocates addresses
   within the virtual address space starting from 0. So only relative
   addresses can be fully resolved. Absolute addresses must be relocated
   again by a PE32 relocator when loaded.  */
static void
relocate_addresses (Elf32_Ehdr *e, Elf32_Shdr *sections,
		    Elf32_Addr *section_addresses,
		    Elf32_Half section_entsize, Elf32_Half num_sections,
		    const char *strtab)
{
  Elf32_Half i;
  Elf32_Shdr *s;
  
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf32_Shdr *) ((char *) s + section_entsize))
    if (s->sh_type == grub_cpu_to_le32 (SHT_REL))
      {
	Elf32_Rel *r;
	Elf32_Word rtab_size, r_size, num_rs;
	Elf32_Off rtab_offset;
	Elf32_Shdr *symtab_section;
	Elf32_Word target_section_index;
	Elf32_Addr target_section_addr;
	Elf32_Shdr *target_section;
	Elf32_Word j;

	symtab_section = (Elf32_Shdr *) ((char *) sections
					 + (grub_le_to_cpu32 (s->sh_link)
					    * section_entsize));
	target_section_index = grub_le_to_cpu32 (s->sh_info);
	target_section_addr = section_addresses[target_section_index];
	target_section = (Elf32_Shdr *) ((char *) sections
					 + (target_section_index
					    * section_entsize));

	grub_util_info ("dealing with the relocation section %s for %s",
			strtab + grub_le_to_cpu32 (s->sh_name),
			strtab + grub_le_to_cpu32 (target_section->sh_name));

	rtab_size = grub_le_to_cpu32 (s->sh_size);
	r_size = grub_le_to_cpu32 (s->sh_entsize);
	rtab_offset = grub_le_to_cpu32 (s->sh_offset);
	num_rs = rtab_size / r_size;

	for (j = 0, r = (Elf32_Rel *) ((char *) e + rtab_offset);
	     j < num_rs;
	     j++, r = (Elf32_Rel *) ((char *) r + r_size))
	  {
	    Elf32_Word info;
	    Elf32_Addr offset;
	    Elf32_Addr sym_addr;
	    Elf32_Addr *target;
	    
	    offset = grub_le_to_cpu32 (r->r_offset);
	    target = (Elf32_Addr *) get_target_address (e, target_section,
							offset);
	    info = grub_le_to_cpu32 (r->r_info);
	    sym_addr = get_symbol_address (e, symtab_section,
					   ELF32_R_SYM (info));

	    switch (ELF32_R_TYPE (info))
	      {
	      case R_386_NONE:
		break;

	      case R_386_32:
		/* This is absolute.  */
		*target = grub_cpu_to_le32 (grub_le_to_cpu32 (*target)
					    + sym_addr);
		grub_util_info ("relocating an R_386_32 entry to 0x%x at the offset 0x%x",
				*target, offset);
		break;

	      case R_386_PC32:
		/* This is relative.  */
		*target = grub_cpu_to_le32 (grub_le_to_cpu32 (*target)
					    + sym_addr
					    - target_section_addr - offset);
		grub_util_info ("relocating an R_386_PC32 entry to 0x%x at the offset 0x%x",
				*target, offset);
		break;

	      default:
		grub_util_error ("unknown relocation type %d",
				 ELF32_R_TYPE (info));
		break;
	      }
	  }
      }
}

void
write_padding (FILE *out, size_t size)
{
  size_t i;

  for (i = 0; i < size; i++)
    if (fputc (0, out) == EOF)
      grub_util_error ("padding failed");
}

/* Add a PE32's fixup entry for a relocation. Return the resulting address
   after having written to the file OUT.  */
Elf32_Addr
add_fixup_entry (struct grub_pe32_fixup_block **block, grub_uint16_t type,
		 Elf32_Addr addr, int flush, Elf32_Addr current_address,
		 FILE *out)
{
  struct grub_pe32_fixup_block *b = *block;
  
  /* First, check if it is necessary to write out the current block.  */
  if (b)
    {
      if (flush || addr < b->page_rva || b->page_rva + 0x1000 <= addr)
	{
	  grub_uint32_t size;

	  if (flush)
	    {
	      /* Add as much padding as necessary to align the address
		 with a section boundary.  */
	      Elf32_Addr next_address;
	      unsigned padding_size;
              size_t index;
	      
	      next_address = current_address + b->block_size;
	      padding_size = ((align_pe32_section (next_address)
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
	  b->page_rva = grub_cpu_to_le32 (b->page_rva);
	  b->block_size = grub_cpu_to_le32 (b->block_size);
	  if (fwrite (b, size, 1, out) != 1)
	    grub_util_error ("write failed");
	  free (b);
	  *block = b = 0;
	}
    }

  if (! flush)
    {
      grub_uint16_t entry;
      size_t index;
      
      /* If not allocated yet, allocate a block with enough entries.  */
      if (! b)
	{
	  *block = b = xmalloc (sizeof (*b) + 2 * 0x1000);

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
      b->entries[index] = grub_cpu_to_le16 (entry);
      b->block_size += 2;
    }

  return current_address;
}

/* Write out zeros to make space for the header.  */
static Elf32_Addr
make_header_space (FILE *out)
{
  Elf32_Addr addr;
  
  addr = get_starting_section_address ();
  write_padding (out, addr);

  return addr;
}

/* Write text sections.  */
static Elf32_Addr
write_text_sections (FILE *out, Elf32_Addr current_address,
		     Elf32_Ehdr *e, Elf32_Shdr *sections,
		     Elf32_Half section_entsize, Elf32_Half num_sections,
		     const char *strtab)
{
  Elf32_Half i;
  Elf32_Shdr *s;
  Elf32_Addr addr;
  
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf32_Shdr *) ((char *) s + section_entsize))
    if (is_text_section (s))
      {
	Elf32_Word align = grub_le_to_cpu32 (s->sh_addralign);
	Elf32_Off offset = grub_le_to_cpu32 (s->sh_offset);
	Elf32_Word size = grub_le_to_cpu32 (s->sh_size);
	const char *name = strtab + grub_le_to_cpu32 (s->sh_name);
	
	if (align)
	  {
	    addr = align_address (current_address, align);
	    if (current_address != addr)
	      {
		grub_util_info ("padding %d bytes for the ELF section alignment",
				addr - current_address);
		write_padding (out, addr - current_address);
		current_address = addr;
	      }
	  }
	
	grub_util_info ("writing the text section %s at 0x%x",
			name, current_address);
	
	if (fwrite ((char *) e + offset, size, 1, out) != 1)
	  grub_util_error ("write failed");
	
	current_address += size;
      }

  addr = align_pe32_section (current_address);
  if (addr != current_address)
    {
      grub_util_info ("padding %d bytes for the PE32 section alignment",
		      addr - current_address);
      write_padding (out, addr - current_address);
    }
  
  return addr;
}

/* Write data sections.  */
static Elf32_Addr
write_data_sections (FILE *out, Elf32_Addr current_address,
		     Elf32_Ehdr *e, Elf32_Shdr *sections,
		     Elf32_Half section_entsize, Elf32_Half num_sections,
		     const char *strtab)
{
  Elf32_Half i;
  Elf32_Shdr *s;
  Elf32_Addr addr;
  
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf32_Shdr *) ((char *) s + section_entsize))
    if (is_data_section (s))
      {
	Elf32_Word align = grub_le_to_cpu32 (s->sh_addralign);
	Elf32_Off offset = grub_le_to_cpu32 (s->sh_offset);
	Elf32_Word size = grub_le_to_cpu32 (s->sh_size);
	const char *name = strtab + grub_le_to_cpu32 (s->sh_name);
	
	if (align)
	  {
	    addr = align_address (current_address, align);
	    if (current_address != addr)
	      {
		grub_util_info ("padding %d bytes for the ELF section alignment",
				addr - current_address);
		write_padding (out, addr - current_address);
		current_address = addr;
	      }
	  }
	
	grub_util_info ("writing the data section %s at 0x%x",
			name, current_address);
	
	if (s->sh_type == grub_cpu_to_le32 (SHT_NOBITS))
	  write_padding (out, size);
	else
	  if (fwrite ((char *) e + offset, size, 1, out) != 1)
	    grub_util_error ("write failed");
	
	current_address += size;
      }
  
  addr = align_pe32_section (current_address);
  if (addr != current_address)
    {
      grub_util_info ("padding %d bytes for the PE32 section alignment",
		      addr - current_address);
      write_padding (out, addr - current_address);
    }
  
  return addr;
}

/* Write modules.  */
static Elf32_Addr
make_mods_section (FILE *out, Elf32_Addr current_address,
		   const char *dir, char *mods[])
{
  struct grub_util_path_list *path_list;
  grub_size_t total_module_size;
  struct grub_util_path_list *p;
  struct grub_module_info modinfo;
  Elf32_Addr addr;

  path_list = grub_util_resolve_dependencies (dir, "moddep.lst", mods);
  
  total_module_size = sizeof (struct grub_module_info);
  for (p = path_list; p; p = p->next)
    {
      total_module_size += (grub_util_get_image_size (p->name)
			    + sizeof (struct grub_module_header));
    }

  grub_util_info ("the total module size is 0x%x", total_module_size);

  modinfo.magic = grub_cpu_to_le32 (GRUB_MODULE_MAGIC);
  modinfo.offset = grub_cpu_to_le32 (sizeof (modinfo));
  modinfo.size = grub_cpu_to_le32 (total_module_size);

  if (fwrite (&modinfo, sizeof (modinfo), 1, out) != 1)
    grub_util_error ("write failed");

  for (p = path_list; p; p = p->next)
    {
      struct grub_module_header header;
      size_t mod_size;
      char *mod_image;
      
      grub_util_info ("adding module %s", p->name);
      
      mod_size = grub_util_get_image_size (p->name);
      header.offset = grub_cpu_to_le32 (sizeof (header));
      header.size = grub_cpu_to_le32 (mod_size + sizeof (header));
      
      mod_image = grub_util_read_image (p->name);

      if (fwrite (&header, sizeof (header), 1, out) != 1
	  || fwrite (mod_image, mod_size, 1, out) != 1)
	grub_util_error ("write failed");

      free (mod_image);
    }
  
  for (p = path_list; p; )
    {
      struct grub_util_path_list *q;

      q = p->next;
      free (p);
      p = q;
    }
      
  current_address += total_module_size;
  
  addr = align_pe32_section (current_address);
  if (addr != current_address)
    {
      grub_util_info ("padding %d bytes for the PE32 section alignment",
		      addr - current_address);
      write_padding (out, addr - current_address);
    }

  return addr;
}

/* Make a .reloc section.  */
static Elf32_Addr
make_reloc_section (FILE *out, Elf32_Addr current_address, Elf32_Ehdr *e, 
		    Elf32_Addr *section_addresses, Elf32_Shdr *sections,
		    Elf32_Half section_entsize, Elf32_Half num_sections,
		    const char *strtab)
{
  Elf32_Half i;
  Elf32_Shdr *s;
  struct grub_pe32_fixup_block *fixup_block = 0;
  
  for (i = 0, s = sections;
       i < num_sections;
       i++, s = (Elf32_Shdr *) ((char *) s + section_entsize))
    if (s->sh_type == grub_cpu_to_le32 (SHT_REL))
      {
	Elf32_Rel *r;
	Elf32_Word rtab_size, r_size, num_rs;
	Elf32_Off rtab_offset;
	Elf32_Addr section_address;
	Elf32_Word j;
	
	grub_util_info ("translating the relocation section %s",
			strtab + grub_le_to_cpu32 (s->sh_name));
			
	rtab_size = grub_le_to_cpu32 (s->sh_size);
	r_size = grub_le_to_cpu32 (s->sh_entsize);
	rtab_offset = grub_le_to_cpu32 (s->sh_offset);
	num_rs = rtab_size / r_size;
	
	section_address = section_addresses[grub_le_to_cpu32 (s->sh_info)];

	for (j = 0, r = (Elf32_Rel *) ((char *) e + rtab_offset);
	     j < num_rs;
	     j++, r = (Elf32_Rel *) ((char *) r + r_size))
	  {
	    Elf32_Word info;
	    Elf32_Addr offset;

	    offset = grub_le_to_cpu32 (r->r_offset);
	    info = grub_le_to_cpu32 (r->r_info);

	    /* Necessary to relocate only absolute addresses.  */
	    if (ELF32_R_TYPE (info) == R_386_32)
	      {
		Elf32_Addr addr;

		addr = section_address + offset;
		grub_util_info ("adding a relocation entry for 0x%x", addr);
		current_address = add_fixup_entry (&fixup_block,
						   GRUB_PE32_REL_BASED_HIGHLOW,
						   addr, 0, current_address,
						   out);
	      }
	  }
      }

  current_address = add_fixup_entry (&fixup_block, 0, 0, 1,
				     current_address, out);

  return current_address;
}

/* Create the header.  */
static void
make_header (FILE *out, Elf32_Addr text_address, Elf32_Addr data_address,
	     Elf32_Addr mods_address, Elf32_Addr reloc_address,
	     Elf32_Addr end_address, Elf32_Addr start_address)
{
  struct grub_pe32_header header;
  struct grub_pe32_coff_header *c;
  struct grub_pe32_optional_header *o;
  struct grub_pe32_section_table text_section, data_section;
  struct grub_pe32_section_table mods_section, reloc_section;

  /* The magic.  */
  memset (&header, 0, sizeof (header));
  memcpy (header.msdos_stub, stub, sizeof (header.msdos_stub));
  memcpy (header.signature, "PE\0\0", sizeof (header.signature));

  /* The COFF file header.  */
  c = &header.coff_header;
  c->machine = grub_cpu_to_le16 (GRUB_PE32_MACHINE_I386);
  c->num_sections = grub_cpu_to_le16 (4);
  c->time = grub_cpu_to_le32 (time (0));
  c->optional_header_size = grub_cpu_to_le16 (sizeof (header.optional_header));
  c->characteristics = grub_cpu_to_le16 (GRUB_PE32_EXECUTABLE_IMAGE
					 | GRUB_PE32_LINE_NUMS_STRIPPED
					 | GRUB_PE32_LOCAL_SYMS_STRIPPED
					 | GRUB_PE32_32BIT_MACHINE);

  /* The PE Optional header.  */
  o = &header.optional_header;
  o->magic = grub_cpu_to_le16 (GRUB_PE32_PE32_MAGIC);
  o->code_size = grub_cpu_to_le32 (data_address - text_address);
  o->data_size = grub_cpu_to_le32 (reloc_address - data_address);
  o->bss_size = 0;
  o->entry_addr = grub_cpu_to_le32 (start_address);
  o->code_base = grub_cpu_to_le32 (text_address);
  o->data_base = grub_cpu_to_le32 (data_address);
  o->image_base = 0;
  o->section_alignment = grub_cpu_to_le32 (GRUB_PE32_SECTION_ALIGNMENT);
  o->file_alignment = grub_cpu_to_le32 (GRUB_PE32_FILE_ALIGNMENT);
  o->image_size = grub_cpu_to_le32 (end_address);
  o->header_size = grub_cpu_to_le32 (text_address);
  o->subsystem = grub_cpu_to_le16 (GRUB_PE32_SUBSYSTEM_EFI_APPLICATION);
  
  /* Do these really matter? */
  o->stack_reserve_size = grub_cpu_to_le32 (0x10000);
  o->stack_commit_size = grub_cpu_to_le32 (0x10000);
  o->heap_reserve_size = grub_cpu_to_le32 (0x10000);
  o->heap_commit_size = grub_cpu_to_le32 (0x10000);

  o->num_data_directories = grub_cpu_to_le32 (GRUB_PE32_NUM_DATA_DIRECTORIES);
  
  o->base_relocation_table.rva = grub_cpu_to_le32 (reloc_address);
  o->base_relocation_table.size = grub_cpu_to_le32 (end_address
						    - reloc_address);

  /* The sections.  */
  memset (&text_section, 0, sizeof (text_section));
  strcpy (text_section.name, ".text");
  text_section.virtual_size = grub_cpu_to_le32 (data_address - text_address);
  text_section.virtual_address = grub_cpu_to_le32 (text_address);
  text_section.raw_data_size = grub_cpu_to_le32 (data_address - text_address);
  text_section.raw_data_offset = grub_cpu_to_le32 (text_address);
  text_section.characteristics = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_CODE
						   | GRUB_PE32_SCN_MEM_EXECUTE
						   | GRUB_PE32_SCN_MEM_READ);

  memset (&data_section, 0, sizeof (data_section));
  strcpy (data_section.name, ".data");
  data_section.virtual_size = grub_cpu_to_le32 (mods_address - data_address);
  data_section.virtual_address = grub_cpu_to_le32 (data_address);
  data_section.raw_data_size = grub_cpu_to_le32 (mods_address - data_address);
  data_section.raw_data_offset = grub_cpu_to_le32 (data_address);
  data_section.characteristics
    = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			| GRUB_PE32_SCN_MEM_READ
			| GRUB_PE32_SCN_MEM_WRITE);

  memset (&mods_section, 0, sizeof (mods_section));
  strcpy (mods_section.name, "mods");
  mods_section.virtual_size = grub_cpu_to_le32 (reloc_address - mods_address);
  mods_section.virtual_address = grub_cpu_to_le32 (mods_address);
  mods_section.raw_data_size = grub_cpu_to_le32 (reloc_address - mods_address);
  mods_section.raw_data_offset = grub_cpu_to_le32 (mods_address);
  mods_section.characteristics
    = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			| GRUB_PE32_SCN_MEM_READ
			| GRUB_PE32_SCN_MEM_WRITE);

  memset (&reloc_section, 0, sizeof (reloc_section));
  strcpy (reloc_section.name, ".reloc");
  reloc_section.virtual_size = grub_cpu_to_le32 (end_address - reloc_address);
  reloc_section.virtual_address = grub_cpu_to_le32 (reloc_address);
  reloc_section.raw_data_size = grub_cpu_to_le32 (end_address - reloc_address);
  reloc_section.raw_data_offset = grub_cpu_to_le32 (reloc_address);
  reloc_section.characteristics
    = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			| GRUB_PE32_SCN_MEM_DISCARDABLE
			| GRUB_PE32_SCN_MEM_READ);

  /* Write them out.  */
  if (fseek (out, 0, SEEK_SET) < 0)
    grub_util_error ("seek failed");

  if (fwrite (&header, sizeof (header), 1, out) != 1
      || fwrite (&text_section, sizeof (text_section), 1, out) != 1
      || fwrite (&data_section, sizeof (data_section), 1, out) != 1
      || fwrite (&mods_section, sizeof (mods_section), 1, out) != 1
      || fwrite (&reloc_section, sizeof (reloc_section), 1, out) != 1)
    grub_util_error ("write failed");
}

/* Convert an ELF relocatable object into an EFI Application (PE32).  */
void
convert_elf (const char *dir, char *prefix, FILE *out, char *mods[])
{
  char *kernel_image;
  size_t kernel_size;
  const char *strtab;
  Elf32_Ehdr *e;
  Elf32_Shdr *sections;
  Elf32_Off section_offset;
  Elf32_Half section_entsize;
  Elf32_Half num_sections;
  Elf32_Addr *section_addresses;
  Elf32_Shdr *symtab_section;
  Elf32_Addr start_address;
  Elf32_Addr text_address, data_address, reloc_address, mods_address;
  Elf32_Addr end_address;

  /* Get the kernel image and check the format.  */
  kernel_image = read_kernel_module (dir, prefix, &kernel_size);
  e = (Elf32_Ehdr *) kernel_image;
  if (! check_elf_header (e, kernel_size))
    grub_util_error ("invalid ELF header");
  
  section_offset = grub_cpu_to_le32 (e->e_shoff);
  section_entsize = grub_cpu_to_le16 (e->e_shentsize);
  num_sections = grub_cpu_to_le16 (e->e_shnum);
  if (kernel_size < section_offset + section_entsize * num_sections)
    grub_util_error ("invalid ELF format");

  sections = (Elf32_Shdr *) (kernel_image + section_offset);
  strtab = find_strtab (e, sections, section_entsize);

  /* Relocate sections then symbols in the virtual address space.  */
  section_addresses = locate_sections (sections, section_entsize,
				       num_sections, strtab);
  
  symtab_section = find_symtab_section (sections,
					section_entsize, num_sections);
  if (! symtab_section)
    grub_util_error ("no symbol table");
  
  start_address = relocate_symbols (e, sections, symtab_section,
				    section_addresses, section_entsize,
				    num_sections);
  if (start_address == 0)
    grub_util_error ("start symbol is not defined");

  /* Resolve addresses in the virtual address space.  */
  relocate_addresses (e, sections, section_addresses, section_entsize,
		      num_sections, strtab);

  /* Generate a PE32 image file. The strategy is to dump binary data first,
     then fill up the header.  */
  text_address = make_header_space (out);
  data_address = write_text_sections (out, text_address, e, sections,
				      section_entsize, num_sections,
				      strtab);
  mods_address = write_data_sections (out, data_address, e, sections,
				      section_entsize, num_sections,
				      strtab);
  reloc_address = make_mods_section (out, mods_address, dir, mods);
  end_address = make_reloc_section (out, reloc_address, e, section_addresses,
				    sections, section_entsize, num_sections,
				    strtab);
  make_header (out, text_address, data_address, mods_address,
	       reloc_address, end_address, start_address);

  /* Clean up.  */
  free (section_addresses);
  free (kernel_image);
}

static struct option options[] =
  {
    {"directory", required_argument, 0, 'd'},
    {"prefix", required_argument, 0, 'p'},
    {"output", required_argument, 0, 'o'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    { 0, 0, 0, 0 }
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``grub-mkimage --help'' for more information.\n");
  else
    printf ("\
Usage: grub-mkimage -o FILE [OPTION]... [MODULES]\n\
\n\
Make a bootable image of GRUB.\n\
\n\
-d, --directory=DIR     use images and modules under DIR [default=%s]\n\
-p, --prefix=DIR        set grub_prefix directory [default=%s]\n\
-o, --output=FILE       output a generated image to FILE\n\
-h, --help              display this message and exit\n\
-V, --version           print version information and exit\n\
-v, --verbose           print verbose messages\n\
\n\
Report bugs to <%s>.\n\
", GRUB_LIBDIR, DEFAULT_DIRECTORY, PACKAGE_BUGREPORT);

  exit (status);
}

int
main (int argc, char *argv[])
{
  FILE *fp;
  char *output = NULL;
  char *dir = NULL;
  char *prefix = NULL;

  progname = "grub-mkimage";

  while (1)
    {
      int c = getopt_long (argc, argv, "d:p:o:hVv", options, 0);
      if (c == -1)
	break;

      switch (c)
	{
	  case 'd':
	    if (dir)
	      free (dir);
	    dir = xstrdup (optarg);
	    break;
	  case 'h':
	    usage (0);
	    break;
	  case 'o':
	    if (output)
	      free (output);
	    output = xstrdup (optarg);
	    break;
	  case 'p':
	    if (prefix)
	      free (prefix);
	    prefix = xstrdup (optarg);
	    break;
	  case 'V':
	    printf ("grub-mkimage (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;
	  case 'v':
	    verbosity++;
	    break;
	  default:
	    usage (1);
	    break;
	}
  }

  if (! output)
    usage (1);

  fp = fopen (output, "wb");
  if (! fp)
    grub_util_error ("cannot open %s", output);

  convert_elf (dir ? : GRUB_LIBDIR, prefix ? : DEFAULT_DIRECTORY, fp, argv + optind);

  fclose (fp);

  return 0;
}
