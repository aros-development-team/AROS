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
#include <grub/hexdump.h>

static const struct grub_arg_option options[] = {
  {"skip", 's', 0, "skip offset bytes from the beginning of file.", 0,
   ARG_TYPE_INT},
  {"length", 'n', 0, "read only length bytes", 0, ARG_TYPE_INT},
  {0, 0, 0, 0, 0, 0}
};

void
hexdump (unsigned long bse, char *buf, int len)
{
  int pos;
  char line[80];

  while (len > 0)
    {
      int cnt, i;

      pos = grub_sprintf (line, "%08lx  ", bse);
      cnt = 16;
      if (cnt > len)
	cnt = len;

      for (i = 0; i < cnt; i++)
	{
	  pos += grub_sprintf (&line[pos], "%02x ", (unsigned char) buf[i]);
	  if ((i & 7) == 7)
	    line[pos++] = ' ';
	}

      for (; i < 16; i++)
	{
	  pos += grub_sprintf (&line[pos], "   ");
	  if ((i & 7) == 7)
	    line[pos++] = ' ';
	}

      line[pos++] = '|';

      for (i = 0; i < cnt; i++)
	line[pos++] = ((buf[i] >= 32) && (buf[i] < 127)) ? buf[i] : '.';

      line[pos++] = '|';

      line[pos] = 0;

      grub_printf ("%s\n", line);

      bse += 16;
      buf += 16;
      len -= cnt;
    }
}

static grub_err_t
grub_cmd_hexdump (struct grub_arg_list *state, int argc, char **args)
{
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  grub_ssize_t size, length;
  unsigned long skip;
  int is_file;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  skip = (state[0].set) ? grub_strtoul (state[0].arg, 0, 0) : 0;
  length = (state[1].set) ? grub_strtoul (state[1].arg, 0, 0) : 0;

  is_file = (grub_strcmp (args[0], "(mem)"));
  if ((!is_file) && (!length))
    length = 256;

  if (is_file)
    {
      file = grub_gzfile_open (args[0], 1);
      if (!file)
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
  else
    hexdump (skip, (char *) skip, length);

  return 0;
}


GRUB_MOD_INIT (hexdump)
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("hexdump", grub_cmd_hexdump, GRUB_COMMAND_FLAG_BOTH,
			 "hexdump  [ -s offset ] [-n length] { FILE | (mem) }",
			 "Dump the contents of a file or memory.", options);
}

GRUB_MOD_FINI (hexdump)
{
  grub_unregister_command ("hexdump");
}
