/* cat.c - command to show the contents of a file  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/gzio.h>

static grub_err_t
grub_cmd_cat (struct grub_arg_list *state __attribute__ ((unused)),
	      int argc, char **args)

{
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  grub_ssize_t size;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  file = grub_gzfile_open (args[0], 1);
  if (! file)
    return 0;
  
  while ((size = grub_file_read (file, buf, sizeof (buf))) > 0)
    {
      int i;
      
      for (i = 0; i < size; i++)
	{
	  unsigned char c = buf[i];
	  
	  if (grub_isprint (c) || grub_isspace (c))
	    grub_putchar (c);
	  else
	    {
	      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
	      grub_printf ("<%x>", (int) c);
	      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
	    }
	}
    }

  grub_putchar ('\n');
  grub_refresh ();
  grub_file_close (file);
  
  return 0;
}


#ifdef GRUB_UTIL
void
grub_cat_init (void)
{
  grub_register_command ("cat", grub_cmd_cat, GRUB_COMMAND_FLAG_BOTH,
			 "cat FILE", "Show the contents of a file.", 0);
}

void
grub_cat_fini (void)
{
  grub_unregister_command ("cat");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("cat", grub_cmd_cat, GRUB_COMMAND_FLAG_BOTH,
			 "cat FILE", "Show the contents of a file.", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("cat");
}
#endif /* ! GRUB_UTIL */
