/* multiboot_loader.c - boot multiboot 1 or 2 OS image */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008  Free Software Foundation, Inc.
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

#include <grub/machine/machine.h>
#include <grub/multiboot.h>
#include <grub/multiboot2.h>
#include <multiboot2.h>
#include <grub/elf.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gzio.h>
#include <grub/command.h>

grub_dl_t my_mod;

/* This tracks which version of multiboot to use when using
 * the module command. By default use multiboot version 1.
 * values:
 *      1 - Multiboot version 1
 *      2 - Multiboot version 2
 */

static unsigned int module_version_status = 1;

static int
find_multi_boot1_header (grub_file_t file)
{
  struct grub_multiboot_header *header;
  char buffer[MULTIBOOT_SEARCH];
  int found_status = 0;
  grub_ssize_t len;

  len = grub_file_read (file, buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    return found_status;

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct grub_multiboot_header *) buffer;
      ((char *) header <= buffer + len - 12) || (header = 0);
      header = (struct grub_multiboot_header *) ((char *) header + 4))
    {
      if (header->magic == MULTIBOOT_MAGIC
          && !(header->magic + header->flags + header->checksum))
        {
           found_status = 1;
           break;
        }
     }

   return found_status;
}

static int
find_multi_boot2_header (grub_file_t file)
{
  struct multiboot_header *header;
  char buffer[MULTIBOOT_SEARCH];
  int found_status = 0;
  grub_ssize_t len;

  len = grub_file_read (file, buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    return found_status;

  /* Look for the multiboot header in the buffer.  The header should
     be at least 8 bytes and aligned on a 8-byte boundary.  */
  for (header = (struct multiboot_header *) buffer;
      ((char *) header <= buffer + len - 8) || (header = 0);
      header = (struct multiboot_header *) ((char *) header + 8))
    {
      if (header->magic == MULTIBOOT2_HEADER_MAGIC)
        {
           found_status = 1;
           break;
        }
     }

   return found_status;
}

static grub_err_t
grub_cmd_multiboot_loader (grub_command_t cmd __attribute__ ((unused)),
			   int argc, char *argv[])
{
  grub_file_t file = 0;
  int header_multi_ver_found = 0;

  grub_dl_ref (my_mod);

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

  /* find which header is in the file */
  if (find_multi_boot1_header (file))
    header_multi_ver_found = 1;
  else if (find_multi_boot2_header (file))
    header_multi_ver_found = 2;
  else
    {
      grub_error (GRUB_ERR_BAD_OS, "Multiboot header not found");
      goto fail;
    }

  /* close file before calling functions */
  if (file)
    grub_file_close (file);

  /* Launch multi boot with header */

  /* XXX Find a better way to identify this.
     This is for i386-pc */
#if defined(GRUB_MACHINE_PCBIOS) || defined(GRUB_MACHINE_COREBOOT) || \
    defined(GRUB_MACHINE_QEMU)
  if (header_multi_ver_found == 1)
    {
      grub_dprintf ("multiboot_loader",
		    "Launching multiboot 1 grub_multiboot() function\n");
      grub_multiboot (argc, argv);
      module_version_status = 1;
    }
#endif
  if (header_multi_ver_found == 0 || header_multi_ver_found == 2)
    {
      grub_dprintf ("multiboot_loader",
		    "Launching multiboot 2 grub_multiboot2() function\n");
      grub_multiboot2 (argc, argv);
      module_version_status = 2;
    }

  return grub_errno;

fail:
  if (file)
    grub_file_close (file);

  grub_dl_unref (my_mod);

  return grub_errno;
}

static grub_err_t
grub_cmd_module_loader (grub_command_t cmd __attribute__ ((unused)),
			int argc, char *argv[])
{

#if defined(GRUB_MACHINE_PCBIOS) || defined(GRUB_MACHINE_COREBOOT) || \
    defined(GRUB_MACHINE_QEMU)
  if (module_version_status == 1)
    {
      grub_dprintf("multiboot_loader",
           "Launching multiboot 1 grub_module() function\n");
      grub_module (argc, argv);
    }
#endif
  if (module_version_status == 2)
    {
      grub_dprintf("multiboot_loader",
          "Launching multiboot 2 grub_module2() function\n");
      grub_module2 (argc, argv);
    }

  return grub_errno;
}

static grub_command_t cmd_multiboot, cmd_module;

GRUB_MOD_INIT(multiboot)
{
  cmd_multiboot =
    grub_register_command ("multiboot", grub_cmd_multiboot_loader,
			   0, "load a multiboot kernel");
  cmd_module =
    grub_register_command ("module", grub_cmd_module_loader,
			   0, "load a multiboot module");

  my_mod = mod;
}

GRUB_MOD_FINI(multiboot)
{
  grub_unregister_command (cmd_multiboot);
  grub_unregister_command (cmd_module);
}
