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
#include <grub/extcmd.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static const struct grub_arg_option options[] =
  {
    {"dos", -1, 0, N_("Accept DOS-style CR/NL line endings."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
grub_cmd_cat (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;
  int dos = 0;
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  grub_ssize_t size;
  int key = GRUB_TERM_NO_KEY;

  if (state[0].set)
    dos = 1;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = grub_file_open (args[0]);
  if (! file)
    return grub_errno;

  while ((size = grub_file_read (file, buf, sizeof (buf))) > 0
	 && key != GRUB_TERM_ESC)
    {
      int i;

      for (i = 0; i < size; i++)
	{
	  unsigned char c = buf[i];

	  if ((grub_isprint (c) || grub_isspace (c)) && c != '\r')
	    grub_printf ("%c", c);
	  else if (dos && c == '\r' && i + 1 < size && buf[i + 1] == '\n')
	    {
	      grub_printf ("\n");
	      i++;
	    }
	  else
	    {
	      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
	      grub_printf ("<%x>", (int) c);
	      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
	    }
	}

      do
	key = grub_getkey_noblock ();
      while (key != GRUB_TERM_ESC && key != GRUB_TERM_NO_KEY);
    }

  grub_xputs ("\n");
  grub_refresh ();
  grub_file_close (file);

  return 0;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(cat)
{
  cmd = grub_register_extcmd ("cat", grub_cmd_cat, 0,
			      N_("FILE"), N_("Show the contents of a file."),
			      options);
}

GRUB_MOD_FINI(cat)
{
  grub_unregister_extcmd (cmd);
}
