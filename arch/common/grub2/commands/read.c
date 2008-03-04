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
#include <grub/normal.h>
#include <grub/term.h>
#include <grub/types.h>

static char *
grub_getline (void)
{
  int i;
  char *line;
  char *tmp;

  i = 0;
  line = grub_malloc (1 + i + sizeof('\0'));
  if (! line)
    return NULL;

  while ((line[i - 1] != '\n') && (line[i - 1] != '\r'))
    {
      line[i] = grub_getkey ();
      if (grub_isprint (line[i]))
	grub_putchar (line[i]);
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
grub_cmd_read (struct grub_arg_list *state UNUSED, int argc, char **args)
{
  char *line = grub_getline ();
  if (! line)
    return grub_errno;
  if (argc > 0)
    grub_env_set (args[0], line);

  grub_free (line);
  return 0;
}


GRUB_MOD_INIT(read)
{
  grub_register_command ("read", grub_cmd_read, GRUB_COMMAND_FLAG_CMDLINE,
			 "read [ENVVAR]", "Set variable with user input", 0);
}

GRUB_MOD_FINI(read)
{
  grub_unregister_command ("read");
}
