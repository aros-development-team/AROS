/* terminal.c - command to show and select a terminal */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2007  Free Software Foundation, Inc.
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
#include <grub/term.h>

static grub_err_t
grub_cmd_terminal_input (struct grub_arg_list *state __attribute__ ((unused)),
			 int argc, char **args)
{
  grub_term_input_t term = 0;
  
  auto int print_terminal (grub_term_input_t);
  auto int find_terminal (grub_term_input_t);
  
  int print_terminal (grub_term_input_t t)
    {
      grub_printf (" %s", t->name);
      return 0;
    }

  int find_terminal (grub_term_input_t t)
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
      grub_printf ("Available input terminal(s):");
      grub_term_iterate_input (print_terminal);
      grub_putchar ('\n');
      
      grub_printf ("Current input terminal: %s\n", grub_term_get_current_input ()->name);
    }
  else
    {
      grub_term_iterate_input (find_terminal);
      if (! term)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such input terminal");

      grub_term_set_current_input (term);
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_terminal_output (struct grub_arg_list *state __attribute__ ((unused)),
			  int argc, char **args)
{
  grub_term_output_t term = 0;
  
  auto int print_terminal (grub_term_output_t);
  auto int find_terminal (grub_term_output_t);
  
  int print_terminal (grub_term_output_t t)
    {
      grub_printf (" %s", t->name);
      return 0;
    }

  int find_terminal (grub_term_output_t t)
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
      grub_printf ("Available output terminal(s):");
      grub_term_iterate_output (print_terminal);
      grub_putchar ('\n');
      
      grub_printf ("Current output terminal: %s\n", grub_term_get_current_output ()->name);
    }
  else
    {
      grub_term_iterate_output (find_terminal);
      if (! term)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such output terminal");

      grub_term_set_current_output (term);
    }

  return GRUB_ERR_NONE;
}


GRUB_MOD_INIT(terminal)
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("terminal_input", grub_cmd_terminal_input, GRUB_COMMAND_FLAG_BOTH,
			 "terminal_input [TERM...]", "Select an input terminal.", 0);
  grub_register_command ("terminal_output", grub_cmd_terminal_output, GRUB_COMMAND_FLAG_BOTH,
			 "terminal_output [TERM...]", "Select an output terminal.", 0);
}

GRUB_MOD_FINI(terminal)
{
  grub_unregister_command ("terminal_input");
  grub_unregister_command ("terminal_output");
}
