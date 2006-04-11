/* help.c - command to show a help text.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
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
#include <grub/misc.h>

/* XXX: This has to be changed into a function so the screen can be
   optimally used.  */
#define TERM_WIDTH	80

static grub_err_t
grub_cmd_help (struct grub_arg_list *state __attribute__ ((unused)), int argc,
	       char **args)

{
  int cnt = 0;
  char *currarg;
  
  auto int print_command_info (grub_command_t cmd);
  auto int print_command_help (grub_command_t cmd);

  int print_command_info (grub_command_t cmd)
    {
      if (grub_command_find (cmd->name))
	{
	  if (cmd->flags & GRUB_COMMAND_FLAG_CMDLINE)
	    {
	      char description[TERM_WIDTH / 2];
	      int desclen = grub_strlen (cmd->summary);
	      
	      /* Make a string with a length of TERM_WIDTH / 2 - 1 filled
		 with the description followed by spaces.  */
	      grub_memset (description, ' ', TERM_WIDTH / 2 - 1);
	      description[TERM_WIDTH / 2 - 1] = '\0';
	      grub_memcpy (description, cmd->summary,
			   (desclen < TERM_WIDTH / 2 - 1 
			    ? desclen : TERM_WIDTH / 2 - 1));
	      
	      grub_printf ("%s%s", description, (cnt++) % 2 ? "\n" : " ");
	    }
	}
      return 0;
    }

  int print_command_help (grub_command_t cmd)
    {
      if (grub_command_find (cmd->name))
	{
	  if (! grub_strncmp (cmd->name, currarg, grub_strlen (currarg)))
	    {
	      if (cnt++ > 0)
		grub_printf ("\n\n");
	      
	      grub_arg_show_help (cmd);
	    }
	}
      return 0;
    }
  
  if (argc == 0)
    grub_iterate_commands (print_command_info);
  else
    {
      int i;
      
      for (i = 0; i < argc; i++)
	{
	  currarg = args[i];
	  grub_iterate_commands (print_command_help);	  
	}
    }
  
  return 0;
}



#ifdef GRUB_UTIL
void
grub_help_init (void)
{
  grub_register_command ("help", grub_cmd_help, GRUB_COMMAND_FLAG_CMDLINE,
			 "help [PATTERN ...]", "Show a help message.", 0);
}

void
grub_help_fini (void)
{
  grub_unregister_command ("help");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("help", grub_cmd_help, GRUB_COMMAND_FLAG_CMDLINE,
			 "help [PATTERN ...]", "Show a help message.", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("help");
}
#endif /* ! GRUB_UTIL */
