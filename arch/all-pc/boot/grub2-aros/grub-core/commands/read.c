/* read.c - Command to read variables from user.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/command.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static char *
grub_getline (void)
{
  int i;
  char *line;
  char *tmp;
  char c;

  i = 0;
  line = grub_malloc (1 + i + sizeof('\0'));
  if (! line)
    return NULL;

  while (1)
    {
      c = grub_getkey ();
      if ((c == '\n') || (c == '\r'))
	break;

      line[i] = c;
      if (grub_isprint (c))
	grub_printf ("%c", c);
      i++;
      tmp = grub_realloc (line, 1 + i + sizeof('\0'));
      if (! tmp)
	{
	  grub_free (line);
	  return NULL;
	}
      line = tmp;
    }
  line[i] = '\0';

  return line;
}

static grub_err_t
grub_cmd_read (grub_command_t cmd __attribute__ ((unused)), int argc, char **args)
{
  char *line = grub_getline ();
  if (! line)
    return grub_errno;
  if (argc > 0)
    grub_env_set (args[0], line);

  grub_free (line);
  return 0;
}

static grub_command_t cmd;

GRUB_MOD_INIT(read)
{
  cmd = grub_register_command ("read", grub_cmd_read,
			       N_("[ENVVAR]"),
			       N_("Set variable with user input."));
}

GRUB_MOD_FINI(read)
{
  grub_unregister_command (cmd);
}
