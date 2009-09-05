/* multiboot.c - boot a multiboot OS image. */
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
#include <grub/cpu/multiboot.h>
#include <grub/elf.h>
#include <grub/aout.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gzio.h>
#include <grub/env.h>
#ifdef GRUB_MACHINE_PCBIOS
#include <grub/machine/biosnum.h>
#include <grub/disk.h>
#include <grub/device.h>
#include <grub/partition.h>
#endif

extern grub_dl_t my_mod;
static struct grub_multiboot_info *mbi, *mbi_dest;
static grub_addr_t entry;

static char *playground = 0;
static grub_size_t code_size;

static grub_err_t
grub_multiboot_boot (void)
{
  grub_multiboot_real_boot (entry, mbi_dest);

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_multiboot_unload (void)
{
  if (playground)
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
      grub_free (playground);
    }

  mbi = NULL;
  playground = NULL;
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

  grub_mmap_iterate (hook);

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

  grub_mmap_iterate (hook);
}

#define MULTIBOOT_LOAD_ELF64
#include "multiboot_elfxx.c"
#undef MULTIBOOT_LOAD_ELF64

#define MULTIBOOT_LOAD_ELF32
#include "multiboot_elfxx.c"
#undef MULTIBOOT_LOAD_ELF32

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
#ifdef GRUB_MACHINE_PCBIOS
  char *p;
  grub_uint32_t biosdev, slice = ~0, part = ~0;
  grub_device_t dev;

  biosdev = grub_get_root_biosnumber ();

  dev = grub_device_open (0);
  if (dev && dev->disk && dev->disk->partition)
    {

      p = dev->disk->partition->partmap->get_name (dev->disk->partition);
      if (p)
	{
	  if ((p[0] >= '0') && (p[0] <= '9'))
	    {
	      slice = grub_strtoul (p, &p, 0) - 1;

	      if ((p) && (p[0] == ','))
		p++;
	    }

	  if ((p[0] >= 'a') && (p[0] <= 'z'))
	    part = p[0] - 'a';
	}
    }
  if (dev)
    grub_device_close (dev);

  *bootdev = ((biosdev & 0xff) << 24) | ((slice & 0xff) << 16) 
    | ((part & 0xff) << 8) | 0xff;
  return (biosdev != ~0UL);
#else
  *bootdev = 0xffffffff;
  return 0;
#endif
}

void
grub_multiboot (int argc, char *argv[])
{
  grub_file_t file = 0;
  char buffer[MULTIBOOT_SEARCH], *cmdline = 0, *p;
  struct grub_multiboot_header *header;
  grub_ssize_t len, cmdline_length, boot_loader_name_length;
  grub_uint32_t mmap_length;
  int i;
  int cmdline_argc;
  char **cmdline_argv;

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

  mmap_length = grub_get_multiboot_mmap_len ();

  /* Figure out cmdline length.  */
  /* Skip filename.  */
  cmdline_argc = argc - 1;
  cmdline_argv = argv + 1;

  for (i = 0, cmdline_length = 0; i < cmdline_argc; i++)
    cmdline_length += grub_strlen (cmdline_argv[i]) + 1;

  if (cmdline_length == 0)
    cmdline_length = 1;

  boot_loader_name_length = sizeof(PACKAGE_STRING);

#define cmdline_addr(x)		((void *) ((x) + code_size))
#define boot_loader_name_addr(x) \
				((void *) ((x) + code_size + cmdline_length))
#define mbi_addr(x)		((void *) ((x) + code_size + cmdline_length + boot_loader_name_length))
#define mmap_addr(x)		((void *) ((x) + code_size + cmdline_length + boot_loader_name_length + sizeof (struct grub_multiboot_info)))

  grub_multiboot_payload_size = cmdline_length
    /* boot_loader_name_length might need to grow for mbi,etc to be aligned (see below) */
    + boot_loader_name_length + 3
    + sizeof (struct grub_multiboot_info) + mmap_length;

  if (header->flags & MULTIBOOT_AOUT_KLUDGE)
    {
      int offset = ((char *) header - buffer -
		    (header->header_addr - header->load_addr));
      int load_size = ((header->load_end_addr == 0) ? file->size - offset :
		       header->load_end_addr - header->load_addr);

      if (header->bss_end_addr)
	code_size = (header->bss_end_addr - header->load_addr);
      else
	code_size = load_size;
      grub_multiboot_payload_dest = header->load_addr;

      grub_multiboot_payload_size += code_size;
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

  /* This provides alignment for the MBI, the memory map and the backward relocator.  */
  boot_loader_name_length += (0x04 - ((unsigned long) mbi_addr (grub_multiboot_payload_dest) & 0x03));

  mbi = mbi_addr (grub_multiboot_payload_orig);
  mbi_dest = mbi_addr (grub_multiboot_payload_dest);
  grub_memset (mbi, 0, sizeof (struct grub_multiboot_info));
  mbi->mmap_length = mmap_length;

  grub_fill_multiboot_mmap (mmap_addr (grub_multiboot_payload_orig));

  /* FIXME: grub_uint32_t will break for addresses above 4 GiB, but is mandated
     by the spec.  Is there something we can do about it?  */
  mbi->mmap_addr = (grub_uint32_t) mmap_addr (grub_multiboot_payload_dest);
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
  mbi->mem_lower = grub_mmap_get_lower () / 1024;
  mbi->mem_upper = grub_mmap_get_upper () / 1024;
  mbi->flags |= MULTIBOOT_INFO_MEMORY;

  cmdline = p = cmdline_addr (grub_multiboot_payload_orig);
  if (! cmdline)
    goto fail;

  for (i = 0; i < cmdline_argc; i++)
    {
      p = grub_stpcpy (p, cmdline_argv[i]);
      *(p++) = ' ';
    }

  /* Remove the space after the last word.  */
  if (p != cmdline)
    p--;
  *p = 0;

  mbi->flags |= MULTIBOOT_INFO_CMDLINE;
  mbi->cmdline = (grub_uint32_t) cmdline_addr (grub_multiboot_payload_dest);


  grub_strcpy (boot_loader_name_addr (grub_multiboot_payload_orig), PACKAGE_STRING);
  mbi->flags |= MULTIBOOT_INFO_BOOT_LOADER_NAME;
  mbi->boot_loader_name = (grub_uint32_t) boot_loader_name_addr (grub_multiboot_payload_dest);

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
  int cmdline_argc;
  char **cmdline_argv;

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

  /* Skip module name.  */
  cmdline_argc = argc - 1;
  cmdline_argv = argv + 1;

  for (i = 0; i < cmdline_argc; i++)
    len += grub_strlen (cmdline_argv[i]) + 1;

  if (len == 0)
    len = 1;

  cmdline = p = grub_malloc (len);
  if (! cmdline)
    goto fail;

  for (i = 0; i < cmdline_argc; i++)
    {
      p = grub_stpcpy (p, cmdline_argv[i]);
      *(p++) = ' ';
    }

  /* Remove the space after the last word.  */
  if (p != cmdline)
    p--;
  *p = '\0';

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
      struct grub_mod_list *modlist = grub_zalloc (sizeof (struct grub_mod_list));
      if (! modlist)
	goto fail;
      modlist->mod_start = (grub_uint32_t) module;
      modlist->mod_end = (grub_uint32_t) module + size;
      modlist->cmdline = (grub_uint32_t) cmdline;
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
