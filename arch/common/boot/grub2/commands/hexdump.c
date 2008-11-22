/* hexdump.c - command to dump the contents of a file or memory */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/gzio.h>
#include <grub/partition.h>
#include <grub/lib/hexdump.h>

static const struct grub_arg_option options[] = {
  {"skip", 's', 0, "skip offset bytes from the beginning of file.", 0,
   ARG_TYPE_INT},
  {"length", 'n', 0, "read only length bytes", 0, ARG_TYPE_INT},
  {0, 0, 0, 0, 0, 0}
};

static grub_err_t
grub_cmd_hexdump (struct grub_arg_list *state, int argc, char **args)
{
  char buf[GRUB_DISK_SECTOR_SIZE * 4];
  grub_ssize_t size, length;
  grub_addr_t skip;
  int namelen;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  namelen = grub_strlen (args[0]);
  skip = (state[0].set) ? grub_strtoul (state[0].arg, 0, 0) : 0;
  length = (state[1].set) ? grub_strtoul (state[1].arg, 0, 0) : 256;

  if (!grub_strcmp (args[0], "(mem)"))
    hexdump (skip, (char *) skip, length);
  else if ((args[0][0] == '(') && (args[0][namelen - 1] == ')'))
    {
      grub_disk_t disk;
      grub_disk_addr_t sector;
      grub_size_t ofs;

      args[0][namelen - 1] = 0;
      disk = grub_disk_open (&args[0][1]);
      if (! disk)
        return 0;

      if (disk->partition)
        skip += grub_partition_get_start (disk->partition) << GRUB_DISK_SECTOR_BITS;

      sector = (skip >> (GRUB_DISK_SECTOR_BITS + 2)) * 4;
      ofs = skip & (GRUB_DISK_SECTOR_SIZE * 4 - 1);
      while (length)
        {
          grub_size_t len, n;

          len = length;
          if (ofs + len > sizeof (buf))
            len = sizeof (buf) - ofs;

          n = ((ofs + len + GRUB_DISK_SECTOR_SIZE - 1)
               >> GRUB_DISK_SECTOR_BITS);
          if (disk->dev->read (disk, sector, n, buf))
            break;

          hexdump (skip, &buf[ofs], len);

          ofs = 0;
          skip += len;
          length -= len;
          sector += 4;
        }

      grub_disk_close (disk);
    }
  else
    {
      grub_file_t file;

      file = grub_gzfile_open (args[0], 1);
      if (! file)
	return 0;

      file->offset = skip;

      while ((size = grub_file_read (file, buf, sizeof (buf))) > 0)
	{
	  unsigned long len;

	  len = ((length) && (size > length)) ? length : size;
	  hexdump (skip, buf, len);
	  skip += len;
	  if (length)
	    {
	      length -= len;
	      if (!length)
		break;
	    }
	}

      grub_file_close (file);
    }

  return 0;
}


GRUB_MOD_INIT (hexdump)
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("hexdump", grub_cmd_hexdump, GRUB_COMMAND_FLAG_BOTH,
			 "hexdump [OPTIONS] FILE_OR_DEVICE",
			 "Dump the contents of a file or memory.", options);
}

GRUB_MOD_FINI (hexdump)
{
  grub_unregister_command ("hexdump");
}
