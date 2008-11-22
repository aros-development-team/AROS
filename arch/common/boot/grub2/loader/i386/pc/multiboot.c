/* multiboot.c - boot a multiboot OS image. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008  Free Software Foundation, Inc.
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

/*
 *  FIXME: The following features from the Multiboot specification still
 *         need to be implemented:
 *  - VBE support
 *  - symbol table
 *  - drives table
 *  - ROM configuration table
 *  - APM table
 */

#include <grub/loader.h>
#include <grub/machine/loader.h>
#include <grub/multiboot.h>
#include <grub/machine/init.h>
#include <grub/machine/memory.h>
#include <grub/elf.h>
#include <grub/aout.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/rescue.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gzio.h>
#include <grub/env.h>

extern grub_dl_t my_mod;
static struct grub_multiboot_info *mbi;
static grub_addr_t entry;

static char *playground = NULL;

static grub_err_t
grub_multiboot_boot (void)
{
  grub_multiboot_real_boot (entry, mbi);

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_multiboot_unload (void)
{
  if (mbi)
    {
      unsigned int i;
      for (i = 0; i < mbi->mods_count; i++)
	{
	  grub_free ((void *)
		     ((struct grub_mod_list *) mbi->mods_addr)[i].mod_start);
	  grub_free ((void *)
		     ((struct grub_mod_list *) mbi->mods_addr)[i].cmdline);
	}
      grub_free ((void *) mbi->mods_addr);
      grub_free ((void *) mbi->cmdline);
      grub_free (mbi);
    }
  
  mbi = 0;
  grub_dl_unref (my_mod);

  return GRUB_ERR_NONE;
}

/* Return the length of the Multiboot mmap that will be needed to allocate
   our platform's map.  */
static grub_uint32_t
grub_get_multiboot_mmap_len (void)
{
  grub_size_t count = 0;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr __attribute__ ((unused)),
			     grub_uint64_t size __attribute__ ((unused)),
			     grub_uint32_t type __attribute__ ((unused)))
    {
      count++;
      return 0;
    }
  
  grub_machine_mmap_iterate (hook);
  
  return count * sizeof (struct grub_multiboot_mmap_entry);
}

/* Fill previously allocated Multiboot mmap.  */
static void
grub_fill_multiboot_mmap (struct grub_multiboot_mmap_entry *first_entry)
{
  struct grub_multiboot_mmap_entry *mmap_entry = (struct grub_multiboot_mmap_entry *) first_entry;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size, grub_uint32_t type)
    {
      mmap_entry->addr = addr;
      mmap_entry->len = size;
      mmap_entry->type = type;
      mmap_entry->size = sizeof (struct grub_multiboot_mmap_entry) - sizeof (mmap_entry->size);
      mmap_entry++;

      return 0;
    }

  grub_machine_mmap_iterate (hook);
}

/* Check if BUFFER contains ELF32.  */
static int
grub_multiboot_is_elf32 (void *buffer)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *) buffer;

  return ehdr->e_ident[EI_CLASS] == ELFCLASS32;
}

static grub_err_t
grub_multiboot_load_elf32 (grub_file_t file, void *buffer)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *) buffer;
  char *phdr_base;
  int lowest_segment = 0, highest_segment = 0;
  int i;

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
    return grub_error (GRUB_ERR_UNKNOWN_OS, "invalid ELF class");

  if (grub_dl_check_header (ehdr, sizeof(Elf32_Ehdr)))
    return grub_error (GRUB_ERR_UNKNOWN_OS, "no valid ELF header found");

  if (ehdr->e_type != ET_EXEC)
    return grub_error (GRUB_ERR_UNKNOWN_OS, "invalid ELF file type");

  /* FIXME: Should we support program headers at strange locations?  */
  if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > MULTIBOOT_SEARCH)
    return grub_error (GRUB_ERR_BAD_OS, "program header at a too high offset");

  phdr_base = (char *) buffer + ehdr->e_phoff;
#define phdr(i)			((Elf32_Phdr *) (phdr_base + (i) * ehdr->e_phentsize))

  for (i = 0; i < ehdr->e_phnum; i++)
    if (phdr(i)->p_type == PT_LOAD && phdr(i)->p_filesz != 0)
      {
	if (phdr(i)->p_paddr < phdr(lowest_segment)->p_paddr)
	  lowest_segment = i;
	if (phdr(i)->p_paddr > phdr(highest_segment)->p_paddr)
	  highest_segment = i;
      }
  grub_multiboot_payload_size += (phdr(highest_segment)->p_paddr + phdr(highest_segment)->p_memsz) - phdr(lowest_segment)->p_paddr;
  grub_multiboot_payload_dest = phdr(lowest_segment)->p_paddr;

  playground = grub_malloc (RELOCATOR_SIZEOF(forward) + grub_multiboot_payload_size + RELOCATOR_SIZEOF(backward));
  if (! playground)
    return grub_errno;

  grub_multiboot_payload_orig = (long) playground + RELOCATOR_SIZEOF(forward);

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      if (phdr(i)->p_type == PT_LOAD && phdr(i)->p_filesz != 0)
        {
	  char *load_this_module_at = (char *) (grub_multiboot_payload_orig + (phdr(i)->p_paddr - phdr(lowest_segment)->p_paddr));

	  grub_dprintf ("multiboot_loader", "segment %d: paddr=%p, memsz=0x%x\n",
			i, (void *) phdr(i)->p_paddr, phdr(i)->p_memsz);

	  if (grub_file_seek (file, (grub_off_t) phdr(i)->p_offset)
	      == (grub_off_t) -1)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "invalid offset in program header");

          if ((phdr(i)->p_filesz > 0) && (grub_file_read (file, load_this_module_at, phdr(i)->p_filesz)
              != (grub_ssize_t) phdr(i)->p_filesz))
	    return grub_error (GRUB_ERR_BAD_OS,
			       "couldn't read segment from file");

          if (phdr(i)->p_filesz < phdr(i)->p_memsz)
            grub_memset (load_this_module_at + phdr(i)->p_filesz, 0,
			 phdr(i)->p_memsz - phdr(i)->p_filesz);
        }
    }

  grub_multiboot_payload_entry_offset = ehdr->e_entry - phdr(lowest_segment)->p_vaddr;

#undef phdr

  return grub_errno;
}

/* Check if BUFFER contains ELF64.  */
static int
grub_multiboot_is_elf64 (void *buffer)
{
  Elf64_Ehdr *ehdr = (Elf64_Ehdr *) buffer;

  return ehdr->e_ident[EI_CLASS] == ELFCLASS64;
}

static grub_err_t
grub_multiboot_load_elf64 (grub_file_t file, void *buffer)
{
  Elf64_Ehdr *ehdr = (Elf64_Ehdr *) buffer;
  char *phdr_base;
  grub_addr_t physical_entry_addr = 0;
  int i;

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    return grub_error (GRUB_ERR_UNKNOWN_OS, "invalid ELF class");

  if (ehdr->e_ident[EI_MAG0] != ELFMAG0
      || ehdr->e_ident[EI_MAG1] != ELFMAG1
      || ehdr->e_ident[EI_MAG2] != ELFMAG2
      || ehdr->e_ident[EI_MAG3] != ELFMAG3
      || ehdr->e_version != EV_CURRENT
      || ehdr->e_ident[EI_DATA] != ELFDATA2LSB
      || ehdr->e_machine != EM_X86_64)
    return grub_error(GRUB_ERR_UNKNOWN_OS, "no valid ELF header found");

  if (ehdr->e_type != ET_EXEC)
    return grub_error (GRUB_ERR_UNKNOWN_OS, "invalid ELF file type");

  /* FIXME: Should we support program headers at strange locations?  */
  if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > MULTIBOOT_SEARCH)
    return grub_error (GRUB_ERR_BAD_OS, "program header at a too high offset");

  /* We still in 32-bit mode */
  if (ehdr->e_entry > 0xffffffff)
    return grub_error (GRUB_ERR_BAD_OS, "invalid entry point for ELF64");

  entry = ehdr->e_entry;

  phdr_base = (char *) buffer + ehdr->e_phoff;
#define phdr(i)			((Elf64_Phdr *) (phdr_base + (i) * ehdr->e_phentsize))

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      if (phdr(i)->p_type == PT_LOAD)
        {
          /* The segment should fit in the area reserved for the OS.  */
          if (phdr(i)->p_paddr < (grub_uint64_t) grub_os_area_addr)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "segment doesn't fit in memory reserved for the OS (0x%lx < 0x%lx)",
			       phdr(i)->p_paddr, (grub_uint64_t) grub_os_area_addr);
          if (phdr(i)->p_paddr + phdr(i)->p_memsz
		  > (grub_uint64_t) grub_os_area_addr + (grub_uint64_t) grub_os_area_size)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "segment doesn't fit in memory reserved for the OS (0x%lx > 0x%lx)",
			       phdr(i)->p_paddr + phdr(i)->p_memsz,
			       (grub_uint64_t) grub_os_area_addr + (grub_uint64_t) grub_os_area_size);

	  if (grub_file_seek (file, (grub_off_t) phdr(i)->p_offset)
	      == (grub_off_t) -1)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "invalid offset in program header");

	  if ((phdr(i)->p_filesz > 0) && (grub_file_read (file, (void *) ((grub_uint32_t) phdr(i)->p_paddr),
			      phdr(i)->p_filesz)
              != (grub_ssize_t) phdr(i)->p_filesz))
	    return grub_error (GRUB_ERR_BAD_OS,
			       "couldn't read segment from file");

          if (phdr(i)->p_filesz < phdr(i)->p_memsz)
	    grub_memset (((char *) ((grub_uint32_t) phdr(i)->p_paddr)
			  + phdr(i)->p_filesz),
			 0,
			 phdr(i)->p_memsz - phdr(i)->p_filesz);

	  if ((entry >= phdr(i)->p_vaddr) &&
	      (entry < phdr(i)->p_vaddr + phdr(i)->p_memsz))
	    physical_entry_addr = entry + phdr(i)->p_paddr - phdr(i)->p_vaddr;
        }
    }
#undef phdr

  if (physical_entry_addr)
    entry = physical_entry_addr;

  return grub_errno;
}

/* Load ELF32 or ELF64.  */
static grub_err_t
grub_multiboot_load_elf (grub_file_t file, void *buffer)
{
  if (grub_multiboot_is_elf32 (buffer))
    return grub_multiboot_load_elf32 (file, buffer);
  else if (grub_multiboot_is_elf64 (buffer))
    return grub_multiboot_load_elf64 (file, buffer);

  return grub_error (GRUB_ERR_UNKNOWN_OS, "unknown ELF class");
}

static int
grub_multiboot_get_bootdev (grub_uint32_t *bootdev)
{
  char *p;

  p = grub_env_get ("root");
  if ((p) && ((p[0] == 'h') || (p[0] == 'f')) && (p[1] == 'd') &&
      (p[2] >= '0') && (p[2] <= '9'))
    {
      grub_uint32_t bd;

      bd = (p[0] == 'h') ? 0x80 : 0;
      bd += grub_strtoul (p + 2, &p, 0);
      bd <<= 24;

      if ((p) && (p[0] == ','))
	{
	  if ((p[1] >= '0') && (p[1] <= '9'))
	    {

	      bd += ((grub_strtoul (p + 1, &p, 0) - 1) & 0xFF) << 16;

	      if ((p) && (p[0] == ','))
		p++;
	    }
          else
            bd += 0xFF0000;

	  if ((p[0] >= 'a') && (p[0] <= 'z'))
            bd += (p[0] - 'a') << 8;
          else
            bd += 0xFF00;
	}
      else
        bd += 0xFFFF00;

      bd += 0xFF;

      *bootdev = bd;
      return 1;
    }

  return 0;
}

void
grub_multiboot (int argc, char *argv[])
{
  grub_file_t file = 0;
  char buffer[MULTIBOOT_SEARCH], *cmdline = 0, *p;
  struct grub_multiboot_header *header;
  grub_ssize_t len;
  int i;

  grub_loader_unset ();

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No kernel specified");
      goto fail;
    }

  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "Couldn't open file");
      goto fail;
    }

  len = grub_file_read (file, buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    {
      grub_error (GRUB_ERR_BAD_OS, "File too small");
      goto fail;
    }

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct grub_multiboot_header *) buffer;
       ((char *) header <= buffer + len - 12) || (header = 0);
       header = (struct grub_multiboot_header *) ((char *) header + 4))
    {
      if (header->magic == MULTIBOOT_MAGIC
	  && !(header->magic + header->flags + header->checksum))
	break;
    }

  if (header == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No multiboot header found");
      goto fail;
    }

  if (header->flags & MULTIBOOT_UNSUPPORTED)
    {
      grub_error (GRUB_ERR_UNKNOWN_OS,
		  "Unsupported flag: 0x%x", header->flags);
      goto fail;
    }

  if (playground)
    {
      grub_free (playground);
      playground = NULL;
    }

  mbi = grub_malloc (sizeof (struct grub_multiboot_info));
  if (! mbi)
    goto fail;

  grub_memset (mbi, 0, sizeof (struct grub_multiboot_info));

  mbi->mmap_length = grub_get_multiboot_mmap_len ();
  grub_multiboot_payload_size = mbi->mmap_length;

  if (header->flags & MULTIBOOT_AOUT_KLUDGE)
    {
      int offset = ((char *) header - buffer -
		    (header->header_addr - header->load_addr));
      int load_size = ((header->load_end_addr == 0) ? file->size - offset :
		       header->load_end_addr - header->load_addr);

      if (header->bss_end_addr)
	grub_multiboot_payload_size += (header->bss_end_addr - header->load_addr);
      else
	grub_multiboot_payload_size += load_size;
      grub_multiboot_payload_dest = header->load_addr;

      playground = grub_malloc (RELOCATOR_SIZEOF(forward) + grub_multiboot_payload_size + RELOCATOR_SIZEOF(backward));
      if (! playground)
	goto fail;

      grub_multiboot_payload_orig = (long) playground + RELOCATOR_SIZEOF(forward);

      if ((grub_file_seek (file, offset)) == (grub_off_t) - 1)
	goto fail;

      grub_file_read (file, (void *) grub_multiboot_payload_orig, load_size);
      if (grub_errno)
	goto fail;
      
      if (header->bss_end_addr)
	grub_memset ((void *) (grub_multiboot_payload_orig + load_size), 0,
		     header->bss_end_addr - header->load_addr - load_size);
      
      grub_multiboot_payload_entry_offset = header->entry_addr - header->load_addr;

    }
  else if (grub_multiboot_load_elf (file, buffer) != GRUB_ERR_NONE)
    goto fail;

      
  grub_fill_multiboot_mmap ((struct grub_multiboot_mmap_entry *) (grub_multiboot_payload_orig
								  + grub_multiboot_payload_size
								  - mbi->mmap_length));

  /* FIXME: grub_uint32_t will break for addresses above 4 GiB, but is mandated
     by the spec.  Is there something we can do about it?  */
  mbi->mmap_addr = grub_multiboot_payload_dest + grub_multiboot_payload_size - mbi->mmap_length;
  mbi->flags |= MULTIBOOT_INFO_MEM_MAP;

  if (grub_multiboot_payload_dest >= grub_multiboot_payload_orig)
    {
      grub_memmove (playground, &grub_multiboot_forward_relocator, RELOCATOR_SIZEOF(forward));
      entry = (grub_addr_t) playground;
    }
  else
    {
      grub_memmove ((char *) (grub_multiboot_payload_orig + grub_multiboot_payload_size),
		    &grub_multiboot_backward_relocator, RELOCATOR_SIZEOF(backward));
      entry = (grub_addr_t) grub_multiboot_payload_orig + grub_multiboot_payload_size;
    }
  
  grub_dprintf ("multiboot_loader", "dest=%p, size=0x%x, entry_offset=0x%x\n",
		(void *) grub_multiboot_payload_dest,
		grub_multiboot_payload_size,
		grub_multiboot_payload_entry_offset);

  /* Convert from bytes to kilobytes.  */
  mbi->mem_lower = grub_lower_mem / 1024;
  mbi->mem_upper = grub_upper_mem / 1024;
  mbi->flags |= MULTIBOOT_INFO_MEMORY;

  for (i = 0, len = 0; i < argc; i++)
    len += grub_strlen (argv[i]) + 1;

  cmdline = p = grub_malloc (len);
  if (! cmdline)
    goto fail;

  for (i = 0; i < argc; i++)
    {
      p = grub_stpcpy (p, argv[i]);
      *(p++) = ' ';
    }

  /* Remove the space after the last word.  */
  *(--p) = '\0';

  mbi->flags |= MULTIBOOT_INFO_CMDLINE;
  mbi->cmdline = (grub_uint32_t) cmdline;

  mbi->flags |= MULTIBOOT_INFO_BOOT_LOADER_NAME;
  mbi->boot_loader_name = (grub_uint32_t) grub_strdup (PACKAGE_STRING);

  if (grub_multiboot_get_bootdev (&mbi->boot_device))
    mbi->flags |= MULTIBOOT_INFO_BOOTDEV;

  grub_loader_set (grub_multiboot_boot, grub_multiboot_unload, 1);

 fail:
  if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_free (cmdline);
      grub_free (mbi);
      grub_dl_unref (my_mod);
    }
}


void
grub_module  (int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size, len = 0;
  char *module = 0, *cmdline = 0, *p;
  int i;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No module specified");
      goto fail;
    }

  if (!mbi)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT,
		  "You need to load the multiboot kernel first");
      goto fail;
    }

  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    goto fail;

  size = grub_file_size (file);
  module = grub_memalign (MULTIBOOT_MOD_ALIGN, size);
  if (! module)
    goto fail;

  if (grub_file_read (file, module, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  for (i = 0; i < argc; i++)
    len += grub_strlen (argv[i]) + 1;

  cmdline = p = grub_malloc (len);
  if (! cmdline)
    goto fail;

  for (i = 0; i < argc; i++)
    {
      p = grub_stpcpy (p, argv[i]);
      *(p++) = ' ';
    }

  /* Remove the space after the last word.  */
  *(--p) = '\0';

  if (mbi->flags & MULTIBOOT_INFO_MODS)
    {
      struct grub_mod_list *modlist = (struct grub_mod_list *) mbi->mods_addr;

      modlist = grub_realloc (modlist, (mbi->mods_count + 1)
			               * sizeof (struct grub_mod_list));
      if (! modlist)
	goto fail;
      mbi->mods_addr = (grub_uint32_t) modlist;
      modlist += mbi->mods_count;
      modlist->mod_start = (grub_uint32_t) module;
      modlist->mod_end = (grub_uint32_t) module + size;
      modlist->cmdline = (grub_uint32_t) cmdline;
      modlist->pad = 0;
      mbi->mods_count++;
    }
  else
    {
      struct grub_mod_list *modlist = grub_malloc (sizeof (struct grub_mod_list));
      if (! modlist)
	goto fail;
      modlist->mod_start = (grub_uint32_t) module;
      modlist->mod_end = (grub_uint32_t) module + size;
      modlist->cmdline = (grub_uint32_t) cmdline;
      modlist->pad = 0;
      mbi->mods_count = 1;
      mbi->mods_addr = (grub_uint32_t) modlist;
      mbi->flags |= MULTIBOOT_INFO_MODS;
    }

 fail:
  if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_free (module);
      grub_free (cmdline);
    }
}
