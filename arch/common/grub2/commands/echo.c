/* echo.c - Command to display a line of text  */
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

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/misc.h>

static const struct grub_arg_option options[] =
  {
    {0, 'n', 0, "do not output the trailing newline", 0, 0},
    {0, 'e', 0, "enable interpretation of backslash escapes", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };


static grub_err_t
grub_cmd_echo (struct grub_arg_list *state, int argc, char **args)
{
  int newline = 1;
  int i;

  /* Check if `-n' was used.  */
  if (state[0].set)
    newline = 0;

  for (i = 0; i < argc; i++)
    {
      char *arg = *args;
      args++;

      while (*arg)
	{
	  /* In case `-e' is used, parse backslashes.  */
	  if (*arg == '\\' && state[1].set)
	    {
	      arg++;
	      if (*arg == '\0')
		break;

	      switch (*arg)
		{
		case '\\':
		  grub_printf ("\\");
		  break;

		case 'a':
		  grub_printf ("\a");
		  break;

		case 'c':
		  newline = 0;
		  break;

		case 'f':
		  grub_printf ("\f");
		  break;

		case 'n':
		  grub_printf ("\n");
		  break;

		case 'r':
		  grub_printf ("\r");
		  break;

		case 't':
		  grub_printf ("\t");
		  break;

		case 'v':
		  grub_printf ("\v");
		  break;
		}
	      arg++;
	      continue;
	    }
	  
	  /* This was not an escaped character, or escaping is not
	     enabled.  */
	  grub_printf ("%c", *arg);
	  arg++;
	}

      /* If another argument follows, insert a space.  */
      if (i != argc - 1)
	grub_printf (" " );
    }

  if (newline)
    grub_printf ("\n");

  return 0;
}


GRUB_MOD_INIT(echo)
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("echo", grub_cmd_echo, GRUB_COMMAND_FLAG_BOTH,
			 "echo [-e|-n] FILE", "Display a line of text.",
			 options);
}

GRUB_MOD_FINI(echo)
{
  grub_unregister_command ("echo");
}
