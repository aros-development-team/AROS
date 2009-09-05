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
# define E_MACHINE	EM_386
# define ELFCLASSXX	ELFCLASS32
# define Elf_Ehdr	Elf32_Ehdr
# define Elf_Phdr	Elf32_Phdr
#elif defined(MULTIBOOT_LOAD_ELF64)
# define XX		64
# define E_MACHINE	EM_X86_64
# define ELFCLASSXX	ELFCLASS64
# define Elf_Ehdr	Elf64_Ehdr
# define Elf_Phdr	Elf64_Phdr
#else
#error "I'm confused"
#endif

#define CONCAT(a,b)	CONCAT_(a, b)
#define CONCAT_(a,b)	a ## b

/* Check if BUFFER contains ELF32 (or ELF64).  */
static int
CONCAT(grub_multiboot_is_elf, XX) (void *buffer)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) buffer;

  return ehdr->e_ident[EI_CLASS] == ELFCLASSXX;
}

static grub_err_t
CONCAT(grub_multiboot_load_elf, XX) (grub_file_t file, void *buffer)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) buffer;
  char *phdr_base;
  int lowest_segment = -1, highest_segment = -1;
  int i;

  if (ehdr->e_ident[EI_CLASS] != ELFCLASSXX)
    return grub_error (GRUB_ERR_UNKNOWN_OS, "invalid ELF class");

  if (ehdr->e_ident[EI_MAG0] != ELFMAG0
      || ehdr->e_ident[EI_MAG1] != ELFMAG1
      || ehdr->e_ident[EI_MAG2] != ELFMAG2
      || ehdr->e_ident[EI_MAG3] != ELFMAG3
      || ehdr->e_version != EV_CURRENT
      || ehdr->e_ident[EI_DATA] != ELFDATA2LSB
      || ehdr->e_machine != E_MACHINE)
    return grub_error(GRUB_ERR_UNKNOWN_OS, "no valid ELF header found");

  if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
    return grub_error (GRUB_ERR_UNKNOWN_OS, "invalid ELF file type");

  /* FIXME: Should we support program headers at strange locations?  */
  if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > MULTIBOOT_SEARCH)
    return grub_error (GRUB_ERR_BAD_OS, "program header at a too high offset");

#ifdef MULTIBOOT_LOAD_ELF64
  /* We still in 32-bit mode.  */
  if (ehdr->e_entry > 0xffffffff)
    return grub_error (GRUB_ERR_BAD_OS, "invalid entry point for ELF64");
#endif

  phdr_base = (char *) buffer + ehdr->e_phoff;
#define phdr(i)			((Elf_Phdr *) (phdr_base + (i) * ehdr->e_phentsize))

  for (i = 0; i < ehdr->e_phnum; i++)
    if (phdr(i)->p_type == PT_LOAD && phdr(i)->p_filesz != 0)
      {
	/* Beware that segment 0 isn't necessarily loadable */
	if (lowest_segment == -1
	    || phdr(i)->p_paddr < phdr(lowest_segment)->p_paddr)
	  lowest_segment = i;
	if (highest_segment == -1
	    || phdr(i)->p_paddr > phdr(highest_segment)->p_paddr)
	  highest_segment = i;
      }

  if (lowest_segment == -1)
    return grub_error (GRUB_ERR_BAD_OS, "ELF contains no loadable segments");

  code_size = (phdr(highest_segment)->p_paddr + phdr(highest_segment)->p_memsz) - phdr(lowest_segment)->p_paddr;
  grub_multiboot_payload_dest = phdr(lowest_segment)->p_paddr;

  grub_multiboot_payload_size += code_size;
  playground = grub_malloc (RELOCATOR_SIZEOF(forward) + grub_multiboot_payload_size + RELOCATOR_SIZEOF(backward));
  if (! playground)
    return grub_errno;

  grub_multiboot_payload_orig = (long) playground + RELOCATOR_SIZEOF(forward);

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      if (phdr(i)->p_type == PT_LOAD && phdr(i)->p_filesz != 0)
        {
	  char *load_this_module_at = (char *) (grub_multiboot_payload_orig + (long) (phdr(i)->p_paddr - phdr(lowest_segment)->p_paddr));

	  grub_dprintf ("multiboot_loader", "segment %d: paddr=0x%lx, memsz=0x%lx, vaddr=0x%lx\n",
			i, (long) phdr(i)->p_paddr, (long) phdr(i)->p_memsz, (long) phdr(i)->p_vaddr);

	  if (grub_file_seek (file, (grub_off_t) phdr(i)->p_offset)
	      == (grub_off_t) -1)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "invalid offset in program header");

          if (grub_file_read (file, load_this_module_at, phdr(i)->p_filesz)
              != (grub_ssize_t) phdr(i)->p_filesz)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "couldn't read segment from file");

          if (phdr(i)->p_filesz < phdr(i)->p_memsz)
            grub_memset (load_this_module_at + phdr(i)->p_filesz, 0,
			 phdr(i)->p_memsz - phdr(i)->p_filesz);
        }
    }

  for (i = 0; i < ehdr->e_phnum; i++)
    if (phdr(i)->p_vaddr <= ehdr->e_entry
	&& phdr(i)->p_vaddr + phdr(i)->p_memsz > ehdr->e_entry)
      {
	grub_multiboot_payload_entry_offset = (ehdr->e_entry - phdr(i)->p_vaddr)
	  + (phdr(i)->p_paddr  - phdr(lowest_segment)->p_paddr);
	break;
      }

  if (i == ehdr->e_phnum)
    return grub_error (GRUB_ERR_BAD_OS, "entry point isn't in a segment");

#undef phdr

  return grub_errno;
}

#undef XX
#undef E_MACHINE
#undef ELFCLASSXX
#undef Elf_Ehdr
#undef Elf_Phdr
