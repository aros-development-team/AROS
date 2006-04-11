/* terminal.c - command to show and select a terminal */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003  Free Software Foundation, Inc.
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
#include <grub/term.h>

static grub_err_t
grub_cmd_terminal (struct grub_arg_list *state __attribute__ ((unused)),
		   int argc, char **args)
{
  grub_term_t term = 0;
  
  auto int print_terminal (grub_term_t);
  auto int find_terminal (grub_term_t);
  
  int print_terminal (grub_term_t t)
    {
      grub_printf (" %s", t->name);
      return 0;
    }

  int find_terminal (grub_term_t t)
    {
      if (grub_strcmp (t->name, args[0]) == 0)
	{
	  term = t;
	  return 1;
	}

      return 0;
    }
  
  if (argc == 0)
    {
      grub_printf ("Available terminal(s):");
      grub_term_iterate (print_terminal);
      grub_putchar ('\n');
      
      grub_printf ("Current terminal: %s\n", grub_term_get_current ()->name);
    }
  else
    {
      grub_term_iterate (find_terminal);
      if (! term)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such terminal");

      grub_term_set_current (term);
    }

  return GRUB_ERR_NONE;
}


#ifdef GRUB_UTIL
void
grub_terminal_init (void)
{
  grub_register_command ("terminal", grub_cmd_terminal, GRUB_COMMAND_FLAG_BOTH,
			 "terminal [TERM...]", "Select a terminal.", 0);
}

void
grub_terminal_fini (void)
{
  grub_unregister_command ("terminal");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("terminal", grub_cmd_terminal, GRUB_COMMAND_FLAG_BOTH,
			 "terminal [TERM...]", "Select a terminal.", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("terminal");
}
#endif /* ! GRUB_UTIL */
