/* linux.c - boot Linux */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007,2009  Free Software Foundation, Inc.
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

#include <grub/elf.h>
#include <grub/elfload.h>
#include <grub/loader.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/memory.h>
#include <grub/lib/cmdline.h>
#include <grub/cache.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define ELF32_LOADMASK (0xc0000000UL)
#define ELF64_LOADMASK (0xc000000000000000ULL)

static grub_dl_t my_mod;

static int loaded;

static grub_addr_t initrd_addr;
static grub_size_t initrd_size;

static grub_addr_t linux_addr;
static grub_addr_t linux_entry;
static grub_size_t linux_size;

static char *linux_args;

typedef void (*kernel_entry_t) (void *, unsigned long, int (void *),
				unsigned long, unsigned long);

static grub_addr_t
grub_linux_claimmap_iterate (grub_addr_t target, grub_size_t size,
			     grub_size_t align)
{
  grub_addr_t found_addr = (grub_addr_t) -1;

  auto int NESTED_FUNC_ATTR alloc_mem (grub_uint64_t addr, grub_uint64_t len,
				       grub_memory_type_t type);
  int NESTED_FUNC_ATTR alloc_mem (grub_uint64_t addr, grub_uint64_t len,
				  grub_memory_type_t type)
  {
    grub_uint64_t end = addr + len;
    addr = ALIGN_UP (addr, align);
    target = ALIGN_UP (target, align);

    /* Target above the memory chunk.  */
    if (type != GRUB_MEMORY_AVAILABLE || target > end)
      return 0;

    /* Target inside the memory chunk.  */
    if (target >= addr && target < end && size <= end - target)
      {
	if (grub_claimmap (target, size) == GRUB_ERR_NONE)
	  {
	    found_addr = target;
	    return 1;
	  }
	grub_print_error ();
      }
    /* Target below the memory chunk.  */
    if (target < addr && addr + size <= end)
      {
	if (grub_claimmap (addr, size) == GRUB_ERR_NONE)
	  {
	    found_addr = addr;
	    return 1;
	  }
	grub_print_error ();
      }
    return 0;
  }

  grub_machine_mmap_iterate (alloc_mem);

  return found_addr;
}

static grub_err_t
grub_linux_boot (void)
{
  kernel_entry_t linuxmain;
  grub_ssize_t actual;

  grub_arch_sync_caches ((void *) linux_addr, linux_size);
  /* Set the command line arguments.  */
  grub_ieee1275_set_property (grub_ieee1275_chosen, "bootargs", linux_args,
			      grub_strlen (linux_args) + 1, &actual);

  grub_dprintf ("loader", "Entry point: 0x%x\n", linux_entry);
  grub_dprintf ("loader", "Initrd at: 0x%x, size 0x%x\n", initrd_addr,
		initrd_size);
  grub_dprintf ("loader", "Boot arguments: %s\n", linux_args);
  grub_dprintf ("loader", "Jumping to Linux...\n");

  /* Boot the kernel.  */
  linuxmain = (kernel_entry_t) linux_entry;
  linuxmain ((void *) initrd_addr, initrd_size, grub_ieee1275_entry_fn, 0, 0);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_release_mem (void)
{
  grub_free (linux_args);
  linux_args = 0;

  if (linux_addr && grub_ieee1275_release (linux_addr, linux_size))
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "cannot release memory");

  if (initrd_addr && grub_ieee1275_release (initrd_addr, initrd_size))
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "cannot release memory");

  linux_addr = 0;
  initrd_addr = 0;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_unload (void)
{
  grub_err_t err;

  err = grub_linux_release_mem ();
  grub_dl_unref (my_mod);

  loaded = 0;

  return err;
}

static grub_err_t
grub_linux_load32 (grub_elf_t elf, const char *filename)
{
  Elf32_Addr base_addr;
  grub_addr_t seg_addr;
  grub_uint32_t align;
  grub_uint32_t offset;
  Elf32_Addr entry;

  linux_size = grub_elf32_size (elf, filename, &base_addr, &align);
  if (linux_size == 0)
    return grub_errno;
  /* Pad it; the kernel scribbles over memory beyond its load address.  */
  linux_size += 0x100000;

  /* Linux's entry point incorrectly contains a virtual address.  */
  entry = elf->ehdr.ehdr32.e_entry & ~ELF32_LOADMASK;

  /* Linux's incorrectly contains a virtual address.  */
  base_addr &= ~ELF32_LOADMASK;
  offset = entry - base_addr;

  /* On some systems, firmware occupies the memory we're trying to use.
   * Happily, Linux can be loaded anywhere (it relocates itself).  Iterate
   * until we find an open area.  */
  seg_addr = grub_linux_claimmap_iterate (base_addr & ~ELF32_LOADMASK, linux_size, align);
  if (seg_addr == (grub_addr_t) -1)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "couldn't claim memory");

  linux_entry = seg_addr + offset;
  linux_addr = seg_addr;

  /* Now load the segments into the area we claimed.  */
  auto grub_err_t offset_phdr (Elf32_Phdr *phdr, grub_addr_t *addr, int *do_load);
  grub_err_t offset_phdr (Elf32_Phdr *phdr, grub_addr_t *addr, int *do_load)
    {
      if (phdr->p_type != PT_LOAD)
	{
	  *do_load = 0;
	  return 0;
	}
      *do_load = 1;

      *addr = (phdr->p_paddr - base_addr) + seg_addr;
      return 0;
    }
  return grub_elf32_load (elf, filename, offset_phdr, 0, 0);
}

static grub_err_t
grub_linux_load64 (grub_elf_t elf, const char *filename)
{
  Elf64_Addr base_addr;
  grub_addr_t seg_addr;
  grub_uint64_t align;
  grub_uint64_t offset;
  Elf64_Addr entry;

  linux_size = grub_elf64_size (elf, filename, &base_addr, &align);
  if (linux_size == 0)
    return grub_errno;
  /* Pad it; the kernel scribbles over memory beyond its load address.  */
  linux_size += 0x100000;

  base_addr &= ~ELF64_LOADMASK;
  entry = elf->ehdr.ehdr64.e_entry & ~ELF64_LOADMASK;
  offset = entry - base_addr;
  /* Linux's incorrectly contains a virtual address.  */

  /* On some systems, firmware occupies the memory we're trying to use.
   * Happily, Linux can be loaded anywhere (it relocates itself).  Iterate
   * until we find an open area.  */
  seg_addr = grub_linux_claimmap_iterate (base_addr & ~ELF64_LOADMASK, linux_size, align);
  if (seg_addr == (grub_addr_t) -1)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "couldn't claim memory");

  linux_entry = seg_addr + offset;
  linux_addr = seg_addr;

  /* Now load the segments into the area we claimed.  */
  auto grub_err_t offset_phdr (Elf64_Phdr *phdr, grub_addr_t *addr, int *do_load);
  grub_err_t offset_phdr (Elf64_Phdr *phdr, grub_addr_t *addr, int *do_load)
    {
      if (phdr->p_type != PT_LOAD)
	{
	  *do_load = 0;
	  return 0;
	}
      *do_load = 1;

      *addr = (phdr->p_paddr - base_addr) + seg_addr;
      return 0;
    }
  return grub_elf64_load (elf, filename, offset_phdr, 0, 0);
}

static grub_err_t
grub_cmd_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_elf_t elf = 0;
  int size;

  grub_dl_ref (my_mod);

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto out;
    }

  elf = grub_elf_open (argv[0]);
  if (! elf)
    goto out;

  if (elf->ehdr.ehdr32.e_type != ET_EXEC && elf->ehdr.ehdr32.e_type != ET_DYN)
    {
      grub_error (GRUB_ERR_UNKNOWN_OS,
		  N_("this ELF file is not of the right type"));
      goto out;
    }

  /* Release the previously used memory.  */
  grub_loader_unset ();

  if (grub_elf_is_elf32 (elf))
    grub_linux_load32 (elf, argv[0]);
  else
  if (grub_elf_is_elf64 (elf))
    grub_linux_load64 (elf, argv[0]);
  else
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, N_("invalid arch-dependent ELF magic"));
      goto out;
    }

  size = grub_loader_cmdline_size(argc, argv);
  linux_args = grub_malloc (size + sizeof (LINUX_IMAGE));
  if (! linux_args)
    goto out;

  /* Create kernel command line.  */
  grub_memcpy (linux_args, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  grub_create_loader_cmdline (argc, argv, linux_args + sizeof (LINUX_IMAGE) - 1,
			      size);

out:

  if (elf)
    grub_elf_close (elf);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_linux_release_mem ();
      grub_dl_unref (my_mod);
      loaded = 0;
    }
  else
    {
      grub_loader_set (grub_linux_boot, grub_linux_unload, 1);
      initrd_addr = 0;
      loaded = 1;
    }

  return grub_errno;
}

static grub_err_t
grub_cmd_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_file_t *files = 0;
  grub_size_t size = 0;
  grub_addr_t first_addr;
  grub_addr_t addr;
  int i;
  int nfiles = 0;
  grub_uint8_t *ptr;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, N_("you need to load the kernel first"));
      goto fail;
    }

  files = grub_zalloc (argc * sizeof (files[0]));
  if (!files)
    goto fail;

  for (i = 0; i < argc; i++)
    {
      grub_file_filter_disable_compression ();
      files[i] = grub_file_open (argv[i]);
      if (! files[i])
	goto fail;
      nfiles++;
      size += ALIGN_UP (grub_file_size (files[i]), 4);
    }

  first_addr = linux_addr + linux_size;

  /* Attempt to claim at a series of addresses until successful in
     the same way that grub_rescue_cmd_linux does.  */
  addr = grub_linux_claimmap_iterate (first_addr, size, 0x100000);
  if (addr == (grub_addr_t) -1)
     goto fail;

  grub_dprintf ("loader", "Loading initrd at 0x%x, size 0x%x\n", addr, size);

  ptr = (void *) addr;
  for (i = 0; i < nfiles; i++)
    {
      grub_ssize_t cursize = grub_file_size (files[i]);
      if (grub_file_read (files[i], ptr, cursize) != cursize)
	{
	  grub_ieee1275_release (addr, size);
	  if (!grub_errno)
	    grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			argv[i]);
	  goto fail;
	}
      ptr += cursize;
      grub_memset (ptr, 0, ALIGN_UP_OVERHEAD (cursize, 4));
      ptr += ALIGN_UP_OVERHEAD (cursize, 4);
    }

  initrd_addr = addr;
  initrd_size = size;

 fail:
  for (i = 0; i < nfiles; i++)
    grub_file_close (files[i]);
  grub_free (files);

  return grub_errno;
}

static grub_command_t cmd_linux, cmd_initrd;

GRUB_MOD_INIT(linux)
{
  cmd_linux = grub_register_command ("linux", grub_cmd_linux,
				     0, N_("Load Linux."));
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd,
				      0, N_("Load initrd."));
  my_mod = mod;
}

GRUB_MOD_FINI(linux)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
}
