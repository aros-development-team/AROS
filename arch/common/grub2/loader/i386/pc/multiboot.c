/* multiboot.c - boot a multiboot OS image. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* 
 *  FIXME: The following features from the Multiboot specification still
 *         need to be implemented:
 *  - VBE support
 *  - a.out support
 *  - boot device
 *  - symbol table
 *  - memory map
 *  - drives table
 *  - ROM configuration table
 *  - APM table
 */

#include <grub/loader.h>
#include <grub/machine/loader.h>
#include <grub/machine/multiboot.h>
#include <grub/machine/init.h>
#include <grub/elf.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/rescue.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gzio.h>

static grub_dl_t my_mod;
static struct grub_multiboot_info *mbi;
static grub_addr_t entry;

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
  Elf32_Phdr *phdr;
  int i;

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
    return grub_error (GRUB_ERR_UNKNOWN_OS, "invalid ELF class");
  
  if (grub_dl_check_header (ehdr, sizeof(Elf32_Ehdr)))
    return grub_error (GRUB_ERR_UNKNOWN_OS, "no valid ELF header found");
  
  if (ehdr->e_type != ET_EXEC)
    return grub_error (GRUB_ERR_UNKNOWN_OS, "invalid ELF file type");
  
  /* FIXME: Should we support program headers at strange locations?  */
  if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > GRUB_MB_SEARCH)
    return grub_error (GRUB_ERR_BAD_OS, "program header at a too high offset");
  
  entry = ehdr->e_entry;
  
  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      phdr = (Elf32_Phdr *) ((char *) buffer + ehdr->e_phoff
			     + i * ehdr->e_phentsize);
      if (phdr->p_type == PT_LOAD)
        {
          /* The segment should fit in the area reserved for the OS.  */
          if ((phdr->p_paddr < grub_os_area_addr)
              || (phdr->p_paddr + phdr->p_memsz
		  > grub_os_area_addr + grub_os_area_size))
	    return grub_error (GRUB_ERR_BAD_OS,
			       "segment doesn't fit in memory reserved for the OS");

          if (grub_file_seek (file, phdr->p_offset) == -1)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "invalid offset in program header");
	  
          if (grub_file_read (file, (void *) phdr->p_paddr, phdr->p_filesz)
              != (grub_ssize_t) phdr->p_filesz)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "couldn't read segment from file");

          if (phdr->p_filesz < phdr->p_memsz)
            grub_memset ((char *) phdr->p_paddr + phdr->p_filesz, 0,
			 phdr->p_memsz - phdr->p_filesz);
        }
    }
  
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
  Elf64_Phdr *phdr;
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
  if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > GRUB_MB_SEARCH)
    return grub_error (GRUB_ERR_BAD_OS, "program header at a too high offset");

  /* We still in 32-bit mode */
  if (ehdr->e_entry > 0xffffffff)
    return grub_error (GRUB_ERR_BAD_OS, "invalid entry point for ELF64");

  entry = ehdr->e_entry;

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      phdr = (Elf64_Phdr *) ((char *) buffer + ehdr->e_phoff
			     + i * ehdr->e_phentsize);
      if (phdr->p_type == PT_LOAD)
        {
          /* The segment should fit in the area reserved for the OS.  */
          if ((phdr->p_paddr < (grub_uint64_t) grub_os_area_addr)
              || (phdr->p_paddr + phdr->p_memsz
		  > ((grub_uint64_t) grub_os_area_addr
		     + (grub_uint64_t) grub_os_area_size)))
	    return grub_error (GRUB_ERR_BAD_OS,
			       "segment doesn't fit in memory reserved for the OS");
	  
	  if (grub_file_seek (file, phdr->p_offset) == -1)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "invalid offset in program header");

	  if (grub_file_read (file, (void *) ((grub_uint32_t) phdr->p_paddr),
			      phdr->p_filesz)
              != (grub_ssize_t) phdr->p_filesz)
	    return grub_error (GRUB_ERR_BAD_OS,
			       "couldn't read segment from file");
	  
          if (phdr->p_filesz < phdr->p_memsz)
	    grub_memset (((char *) ((grub_uint32_t) phdr->p_paddr)
			  + phdr->p_filesz),
			 0,
			 phdr->p_memsz - phdr->p_filesz);
        }
    }
  
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

void
grub_rescue_cmd_multiboot (int argc, char *argv[])
{
  grub_file_t file = 0;
  char buffer[GRUB_MB_SEARCH], *cmdline = 0, *p;
  struct grub_multiboot_header *header;
  grub_ssize_t len;
  int i;

  grub_dl_ref (my_mod);

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

  len = grub_file_read (file, buffer, GRUB_MB_SEARCH);
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
      if (header->magic == GRUB_MB_MAGIC 
	  && !(header->magic + header->flags + header->checksum))
	break;
    }
  
  if (header == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No multiboot header found");
      goto fail;
    }

  if (header->flags & GRUB_MB_UNSUPPORTED)
    {
      grub_error (GRUB_ERR_UNKNOWN_OS,
		  "Unsupported flag: 0x%x", header->flags);
      goto fail;
    }

  if (grub_multiboot_load_elf (file, buffer) != GRUB_ERR_NONE)
    goto fail;
  
  mbi = grub_malloc (sizeof (struct grub_multiboot_info));
  if (! mbi)
    goto fail;

  mbi->flags = GRUB_MB_INFO_MEMORY;

  /* Convert from bytes to kilobytes.  */
  mbi->mem_lower = grub_lower_mem / 1024;
  mbi->mem_upper = grub_upper_mem / 1024;

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
  
  mbi->flags |= GRUB_MB_INFO_CMDLINE;
  mbi->cmdline = (grub_uint32_t) cmdline;

  mbi->flags |= GRUB_MB_INFO_BOOT_LOADER_NAME;
  mbi->boot_loader_name = (grub_uint32_t) grub_strdup (PACKAGE_STRING);

  grub_loader_set (grub_multiboot_boot, grub_multiboot_unload);

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
grub_rescue_cmd_module  (int argc, char *argv[])
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
  module = grub_memalign (GRUB_MB_MOD_ALIGN, size);
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

  if (mbi->flags & GRUB_MB_INFO_MODS)
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
      mbi->flags |= GRUB_MB_INFO_MODS;
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


GRUB_MOD_INIT
{
  grub_rescue_register_command ("multiboot", grub_rescue_cmd_multiboot,
				"load a multiboot kernel");
  grub_rescue_register_command ("module", grub_rescue_cmd_module,
				"load a multiboot module");
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_rescue_unregister_command ("multiboot");
  grub_rescue_unregister_command ("module");
}
