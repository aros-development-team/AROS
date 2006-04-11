/* linux.c - boot Linux */
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

#include <grub/elf.h>
#include <grub/loader.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/rescue.h>
#include <grub/misc.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/machine/loader.h>

static grub_dl_t my_mod;

static int loaded;
static int vmlinux;

static grub_addr_t initrd_addr;
static grub_size_t initrd_size;

static grub_addr_t linux_addr;
static grub_size_t linux_size;

static char *linux_args;

typedef void (*kernel_entry_t) (void *, unsigned long, int (void *),
				unsigned long, unsigned long);

static grub_err_t
grub_linux_boot (void)
{
  kernel_entry_t linuxmain;
  grub_size_t actual;

  /* Set the command line arguments.  */
  grub_ieee1275_set_property (grub_ieee1275_chosen, "bootargs", linux_args,
			      grub_strlen (linux_args) + 1, &actual);

  grub_dprintf ("loader", "Entry point: 0x%x\n", linux_addr);
  grub_dprintf ("loader", "Initrd at: 0x%x, size 0x%x\n", initrd_addr,
		initrd_size);
  grub_dprintf ("loader", "Boot arguments: %s\n", linux_args);
  grub_dprintf ("loader", "Jumping to Linux...\n");

  /* Boot the kernel.  */
  linuxmain = (kernel_entry_t) linux_addr;
  linuxmain ((void *) initrd_addr, initrd_size, grub_ieee1275_entry_fn, 0, 0);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_release_mem (void)
{
  grub_free (linux_args);
  linux_args = 0;

  if (linux_addr && grub_ieee1275_release (linux_addr, linux_size))
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "Can not release memory");

  if (initrd_addr && grub_ieee1275_release (initrd_addr, initrd_size))
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "Can not release memory");

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

void
grub_rescue_cmd_linux (int argc, char *argv[])
{
  grub_file_t file = 0;
  Elf32_Ehdr ehdr;
  Elf32_Phdr *phdrs = 0;
  int i;
  int offset = 0;
  grub_addr_t entry;
  int found_addr = 0;
  int size;
  char *dest;

  grub_dl_ref (my_mod);

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  if (grub_file_read (file, (char *) &ehdr, sizeof (ehdr)) != sizeof (ehdr))
    {
      grub_error (GRUB_ERR_READ_ERROR, "cannot read the linux elf header");
      goto fail;
    }

  if (grub_dl_check_header (&ehdr, sizeof(ehdr)))
    {
      grub_error (GRUB_ERR_UNKNOWN_OS, "No valid ELF header found");
      goto fail;
    }

  if (ehdr.e_type != ET_EXEC)
    {
      grub_error (GRUB_ERR_UNKNOWN_OS,
		  "This ELF file is not of the right type\n");
      goto fail;
    }

  /* Read the sections.  */
  entry = ehdr.e_entry;
  if (entry == 0xc0000000)
    {
      entry = 0x01400000;
      vmlinux = 1;
    }
  else
    vmlinux = 0;

  phdrs = (Elf32_Phdr *) grub_malloc (ehdr.e_phnum * ehdr.e_phentsize);
  grub_file_read (file, (void *) phdrs, ehdr.e_phnum * ehdr.e_phentsize);

  /* Release the previously used memory.  */
  grub_loader_unset ();

  /* Determine the amount of memory that is required.  */
  linux_size = 0;
  for (i = 0; i < ehdr.e_phnum; i++)
    {
      Elf32_Phdr *phdr = phdrs + i;
      /* XXX: Is this calculation correct?  */
      linux_size += phdr->p_memsz + phdr->p_filesz;
    }

  /* Reserve memory for the kernel.  */
  linux_size += 0x100000;

  /* For some vmlinux kernels the address set above won't work.  Just
     try some other addresses just like yaboot does.  */
  for (linux_addr = entry; linux_addr < entry + 200 * 0x100000; linux_addr += 0x100000)
    {
      found_addr = grub_claimmap (linux_addr, linux_size);
      if (found_addr != -1)
	break;
    }

  if (found_addr == -1)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "Can not claim memory");
      goto fail;
    }
  entry = linux_addr;

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr.e_phnum; i++)
    {
      Elf32_Phdr *phdr = phdrs + i;

      if (phdr->p_type == PT_LOAD)
	{
	  void *segment_addr = ((char *) entry) + offset;

	  if (grub_file_seek (file, phdr->p_offset) == -1)
	    {
	      grub_error (GRUB_ERR_BAD_OS, "Invalid offset in program header");
	      goto fail;
	    }

	  grub_dprintf ("loader", "Loading segment %d at %p, size 0x%x\n", i,
			segment_addr, phdr->p_filesz);

	  if (grub_file_read (file, segment_addr, phdr->p_filesz)
	      != (grub_ssize_t) phdr->p_filesz)
	    goto fail;

	  if (phdr->p_filesz < phdr->p_memsz)
	    grub_memset ((char *) (((char *) entry) + offset) + phdr->p_filesz, 0,
			 phdr->p_memsz - phdr->p_filesz);

	  offset += phdr->p_filesz;
	}
    }

  size = sizeof ("BOOT_IMAGE=") + grub_strlen (argv[0]);
  for (i = 0; i < argc; i++)
    size += grub_strlen (argv[i]) + 1;

  linux_args = grub_malloc (size);
  if (! linux_args)
    goto fail;

  /* Specify the boot file.  */
  dest = grub_stpcpy (linux_args, "BOOT_IMAGE=");
  dest = grub_stpcpy (dest, argv[0]);
  
  for (i = 1; i < argc; i++)
    {
      *dest++ = ' ';
      dest = grub_stpcpy (dest, argv[i]);
    }

 fail:

  if (file)
    grub_file_close (file);

  grub_free (phdrs);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_linux_release_mem ();
      grub_dl_unref (my_mod);
      loaded = 0;
    }
  else
    {
      grub_loader_set (grub_linux_boot, grub_linux_unload);
      initrd_addr = 0;
      loaded = 1;
    }

  return;
}

void
grub_rescue_cmd_initrd (int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  grub_addr_t addr;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no initrd specified");
      goto fail;
    }

  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "You need to load the kernel first.");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  addr = linux_addr + linux_size;
  size = grub_file_size (file);

  if (grub_claimmap (addr, size) == -1)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "Can not claim memory");
      goto fail;
    }

  grub_dprintf ("loader", "Loading initrd at 0x%x, size 0x%x\n", addr, size);

  if (grub_file_read (file, (void *) addr, size) != size)
    {
      grub_ieee1275_release (addr, size);
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  initrd_addr = addr;
  initrd_size = size;

 fail:
  if (file)
    grub_file_close (file);
}



GRUB_MOD_INIT
{
  grub_rescue_register_command ("linux", grub_rescue_cmd_linux,
				"load a linux kernel");
  grub_rescue_register_command ("initrd", grub_rescue_cmd_initrd,
				"load an initrd");
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_rescue_unregister_command ("linux");
  grub_rescue_unregister_command ("initrd");
}
