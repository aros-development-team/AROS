/* cat.c - command to show the contents of a file  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2007,2008  Free Software Foundation, Inc.
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
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/gzio.h>
#include <grub/command.h>

static grub_err_t
grub_cmd_cat (grub_command_t cmd __attribute__ ((unused)),
	      int argc, char **args)

{
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  grub_ssize_t size;
  int key = 0;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  file = grub_gzfile_open (args[0], 1);
  if (! file)
    return 0;

  while ((size = grub_file_read (file, buf, sizeof (buf))) > 0
	 && key != GRUB_TERM_ESC)
    {
      int i;

      for (i = 0; i < size; i++)
	{
	  unsigned char c = buf[i];

	  if ((grub_isprint (c) || grub_isspace (c)) && c != '\r')
	    grub_putchar (c);
	  else
	    {
	      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
	      grub_printf ("<%x>", (int) c);
	      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
	    }
	}

      while (grub_checkkey () >= 0 &&
	     (key = GRUB_TERM_ASCII_CHAR (grub_getkey ())) != GRUB_TERM_ESC)
	;
    }

  grub_putchar ('\n');
  grub_refresh ();
  grub_file_close (file);

  return 0;
}

static grub_command_t cmd;

GRUB_MOD_INIT(cat)
{
  cmd = grub_register_command_p1 ("cat", grub_cmd_cat,
				  "cat FILE", "Show the contents of a file.");
}

GRUB_MOD_FINI(cat)
{
  grub_unregister_command (cmd);
}
