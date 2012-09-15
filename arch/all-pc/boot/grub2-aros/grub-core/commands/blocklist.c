/* blocklist.c - print the block list of a file */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/command.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_err_t
grub_cmd_blocklist (grub_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  unsigned long start_sector = 0;
  unsigned num_sectors = 0;
  int num_entries = 0;
  grub_disk_addr_t part_start = 0;
  auto void NESTED_FUNC_ATTR read_blocklist (grub_disk_addr_t sector, unsigned offset,
			    unsigned length);
  auto void NESTED_FUNC_ATTR print_blocklist (grub_disk_addr_t sector, unsigned num,
			     unsigned offset, unsigned length);

  void NESTED_FUNC_ATTR read_blocklist (grub_disk_addr_t sector, unsigned offset,
		       unsigned length)
    {
      if (num_sectors > 0)
	{
	  if (start_sector + num_sectors == sector
	      && offset == 0 && length == GRUB_DISK_SECTOR_SIZE)
	    {
	      num_sectors++;
	      return;
	    }

	  print_blocklist (start_sector, num_sectors, 0, 0);
	  num_sectors = 0;
	}

      if (offset == 0 && length == GRUB_DISK_SECTOR_SIZE)
	{
	  start_sector = sector;
	  num_sectors++;
	}
      else
	print_blocklist (sector, 0, offset, length);
    }

  void NESTED_FUNC_ATTR print_blocklist (grub_disk_addr_t sector, unsigned num,
			unsigned offset, unsigned length)
    {
      if (num_entries++)
	grub_printf (",");

      grub_printf ("%llu", (unsigned long long) (sector - part_start));
      if (num > 0)
	grub_printf ("+%u", num);
      if (offset != 0 || length != 0)
	grub_printf ("[%u-%u]", offset, offset + length);
    }

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  grub_file_filter_disable_compression ();
  file = grub_file_open (args[0]);
  if (! file)
    return grub_errno;

  if (! file->device->disk)
    return grub_error (GRUB_ERR_BAD_DEVICE,
		       "this command is available only for disk devices");

  part_start = grub_partition_get_start (file->device->disk->partition);

  file->read_hook = read_blocklist;

  while (grub_file_read (file, buf, sizeof (buf)) > 0)
    ;

  if (num_sectors > 0)
    print_blocklist (start_sector, num_sectors, 0, 0);

  grub_file_close (file);

  return grub_errno;
}

static grub_command_t cmd;

GRUB_MOD_INIT(blocklist)
{
  cmd = grub_register_command ("blocklist", grub_cmd_blocklist,
			       N_("FILE"), N_("Print a block list."));
}

GRUB_MOD_FINI(blocklist)
{
  grub_unregister_command (cmd);
}
