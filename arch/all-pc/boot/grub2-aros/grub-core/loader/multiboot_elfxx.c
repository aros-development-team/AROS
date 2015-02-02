/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#if defined(MULTIBOOT_LOAD_ELF32)
# define XX		32
# define E_MACHINE	MULTIBOOT_ELF32_MACHINE
# define ELFCLASSXX	ELFCLASS32
# define Elf_Ehdr	Elf32_Ehdr
# define Elf_Phdr	Elf32_Phdr
# define Elf_Shdr	Elf32_Shdr
#elif defined(MULTIBOOT_LOAD_ELF64)
# define XX		64
# define E_MACHINE	MULTIBOOT_ELF64_MACHINE
# define ELFCLASSXX	ELFCLASS64
# define Elf_Ehdr	Elf64_Ehdr
# define Elf_Phdr	Elf64_Phdr
# define Elf_Shdr	Elf64_Shdr
#else
#error "I'm confused"
#endif

#include <grub/i386/relocator.h>

#define CONCAT(a,b)	CONCAT_(a, b)
#define CONCAT_(a,b)	a ## b

#pragma GCC diagnostic ignored "-Wcast-align"

/* Check if BUFFER contains ELF32 (or ELF64).  */
static int
CONCAT(grub_multiboot_is_elf, XX) (void *buffer)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) buffer;

  return ehdr->e_ident[EI_CLASS] == ELFCLASSXX;
}

static grub_err_t
CONCAT(grub_multiboot_load_elf, XX) (grub_file_t file, const char *filename, void *buffer)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) buffer;
  char *phdr_base;
  int i;

  if (ehdr->e_ident[EI_MAG0] != ELFMAG0
      || ehdr->e_ident[EI_MAG1] != ELFMAG1
      || ehdr->e_ident[EI_MAG2] != ELFMAG2
      || ehdr->e_ident[EI_MAG3] != ELFMAG3
      || ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    return grub_error(GRUB_ERR_UNKNOWN_OS, N_("invalid arch-independent ELF magic"));

  if (ehdr->e_ident[EI_CLASS] != ELFCLASSXX || ehdr->e_machine != E_MACHINE
      || ehdr->e_version != EV_CURRENT)
    return grub_error (GRUB_ERR_UNKNOWN_OS, N_("invalid arch-dependent ELF magic"));

  if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
    return grub_error (GRUB_ERR_UNKNOWN_OS, N_("this ELF file is not of the right type"));

  /* FIXME: Should we support program headers at strange locations?  */
  if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > MULTIBOOT_SEARCH)
    return grub_error (GRUB_ERR_BAD_OS, "program header at a too high offset");

  phdr_base = (char *) buffer + ehdr->e_phoff;
#define phdr(i)			((Elf_Phdr *) (phdr_base + (i) * ehdr->e_phentsize))

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      if (phdr(i)->p_type == PT_LOAD)
        {
	  grub_err_t err;
	  void *source;

	  if (phdr(i)->p_paddr + phdr(i)->p_memsz > highest_load)
	    highest_load = phdr(i)->p_paddr + phdr(i)->p_memsz;

	  grub_dprintf ("multiboot_loader", "segment %d: paddr=0x%lx, memsz=0x%lx, vaddr=0x%lx\n",
			i, (long) phdr(i)->p_paddr, (long) phdr(i)->p_memsz, (long) phdr(i)->p_vaddr);

	  {
	    grub_relocator_chunk_t ch;
	    err = grub_relocator_alloc_chunk_addr (grub_multiboot_relocator, 
						   &ch, phdr(i)->p_paddr,
						   phdr(i)->p_memsz);
	    if (err)
	      {
		grub_dprintf ("multiboot_loader", "Error loading phdr %d\n", i);
		return err;
	      }
	    source = get_virtual_current_address (ch);
	  }

	  if (phdr(i)->p_filesz != 0)
	    {
	      if (grub_file_seek (file, (grub_off_t) phdr(i)->p_offset)
		  == (grub_off_t) -1)
		return grub_errno;

	      if (grub_file_read (file, source, phdr(i)->p_filesz)
		  != (grub_ssize_t) phdr(i)->p_filesz)
		{
		  if (!grub_errno)
		    grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
				filename);
		  return grub_errno;
		}
	    }

          if (phdr(i)->p_filesz < phdr(i)->p_memsz)
            grub_memset ((grub_uint8_t *) source + phdr(i)->p_filesz, 0,
			 phdr(i)->p_memsz - phdr(i)->p_filesz);
        }
    }

  for (i = 0; i < ehdr->e_phnum; i++)
    if (phdr(i)->p_vaddr <= ehdr->e_entry
	&& phdr(i)->p_vaddr + phdr(i)->p_memsz > ehdr->e_entry)
      {
	grub_multiboot_payload_eip = (ehdr->e_entry - phdr(i)->p_vaddr)
	  + phdr(i)->p_paddr;
#ifdef MULTIBOOT_LOAD_ELF64
# ifdef __mips
  /* We still in 32-bit mode.  */
  if ((ehdr->e_entry - phdr(i)->p_vaddr)
      + phdr(i)->p_paddr < 0xffffffff80000000ULL)
    return grub_error (GRUB_ERR_BAD_OS, "invalid entry point for ELF64");
# else
  /* We still in 32-bit mode.  */
  if ((ehdr->e_entry - phdr(i)->p_vaddr)
      + phdr(i)->p_paddr > 0xffffffff)
    return grub_error (GRUB_ERR_BAD_OS, "invalid entry point for ELF64");
# endif
#endif
	break;
      }

  if (i == ehdr->e_phnum)
    return grub_error (GRUB_ERR_BAD_OS, "entry point isn't in a segment");

#if defined (__i386__) || defined (__x86_64__)
  
#elif defined (__mips)
  grub_multiboot_payload_eip |= 0x80000000;
#else
#error Please complete this
#endif

  if (ehdr->e_shnum)
    {
      grub_uint8_t *shdr, *shdrptr;

      shdr = grub_malloc (ehdr->e_shnum * ehdr->e_shentsize);
      if (!shdr)
	return grub_errno;
      
      if (grub_file_seek (file, ehdr->e_shoff) == (grub_off_t) -1)
	return grub_errno;

      if (grub_file_read (file, shdr, ehdr->e_shnum * ehdr->e_shentsize)
              != (grub_ssize_t) ehdr->e_shnum * ehdr->e_shentsize)
	{
	  if (!grub_errno)
	    grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			filename);
	  return grub_errno;
	}
      
      for (shdrptr = shdr, i = 0; i < ehdr->e_shnum;
	   shdrptr += ehdr->e_shentsize, i++)
	{
	  Elf_Shdr *sh = (Elf_Shdr *) shdrptr;
	  void *src;
	  grub_addr_t target;
	  grub_err_t err;

	  /* This section is a loaded section,
	     so we don't care.  */
	  if (sh->sh_addr != 0)
	    continue;
		      
	  /* This section is empty, so we don't care.  */
	  if (sh->sh_size == 0)
	    continue;

	  {
	    grub_relocator_chunk_t ch;
	    err = grub_relocator_alloc_chunk_align (grub_multiboot_relocator,
						    &ch, 0,
						    (0xffffffff - sh->sh_size)
						    + 1, sh->sh_size,
						    sh->sh_addralign,
						    GRUB_RELOCATOR_PREFERENCE_NONE,
						    0);
	    if (err)
	      {
		grub_dprintf ("multiboot_loader", "Error loading shdr %d\n", i);
		return err;
	      }
	    src = get_virtual_current_address (ch);
	    target = get_physical_target_address (ch);
	  }

	  if (grub_file_seek (file, sh->sh_offset) == (grub_off_t) -1)
	    return grub_errno;

          if (grub_file_read (file, src, sh->sh_size)
              != (grub_ssize_t) sh->sh_size)
	    {
	      if (!grub_errno)
		grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			    filename);
	      return grub_errno;
	    }
	  sh->sh_addr = target;
	}
      grub_multiboot_add_elfsyms (ehdr->e_shnum, ehdr->e_shentsize,
				  ehdr->e_shstrndx, shdr);
    }

#undef phdr

  return grub_errno;
}

#undef XX
#undef E_MACHINE
#undef ELFCLASSXX
#undef Elf_Ehdr
#undef Elf_Phdr
#undef Elf_Shdr
