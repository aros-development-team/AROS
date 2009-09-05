/* crc.c - command to calculate the crc32 checksum of a file  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/lib/crc.h>
#include <grub/command.h>

static grub_err_t
grub_cmd_crc (grub_command_t cmd __attribute__ ((unused)),
	      int argc, char **args)

{
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  grub_ssize_t size;
  grub_uint32_t crc;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  file = grub_file_open (args[0]);
  if (! file)
    return 0;

  crc = 0;
  while ((size = grub_file_read (file, buf, sizeof (buf))) > 0)
    crc = grub_getcrc32 (crc, buf, size);

  grub_file_close (file);

  grub_printf ("%08x\n", crc);

  return 0;
}

static grub_command_t cmd;

GRUB_MOD_INIT(crc)
{
  cmd = grub_register_command ("crc", grub_cmd_crc,
			       "crc FILE",
			       "Calculate the crc32 checksum of a file.");
}

GRUB_MOD_FINI(crc)
{
  grub_unregister_command (cmd);
}
