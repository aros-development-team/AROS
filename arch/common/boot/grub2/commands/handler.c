/* handler.c - commands to list or select handlers */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/handler.h>
#include <grub/command.h>

static grub_err_t
grub_cmd_handler (struct grub_command *cmd,
		  int argc, char **args)
{
  char *class_name;
  void *curr_item = 0;
  grub_handler_class_t head;

  auto int list_item (grub_named_list_t item);
  int list_item (grub_named_list_t item)
    {
      if (item == curr_item)
	grub_putchar ('*');

      grub_printf ("%s\n", item->name);

      return 0;
    }

  class_name = (grub_strcmp (cmd->name, "handler")) ? (char *) cmd->name : 0;

  head = grub_handler_class_list;
  if ((argc == 0) && (class_name == 0))
    {
      grub_list_iterate (GRUB_AS_LIST (head), (grub_list_hook_t) list_item);
    }
  else
    {
      grub_handler_class_t class;

      if (class_name == 0)
	{
	  class_name = args[0];
	  argc--;
	  args++;
	}

      class = grub_named_list_find (GRUB_AS_NAMED_LIST (head), class_name);
      if (! class)
	return grub_error (GRUB_ERR_FILE_NOT_FOUND, "class not found");

      if (argc == 0)
	{
	  curr_item = class->cur_handler;
	  grub_list_iterate (GRUB_AS_LIST (class->handler_list),
			     (grub_list_hook_t) list_item);
	}
      else
	{
	  grub_handler_t handler;

	  handler =
	    grub_named_list_find (GRUB_AS_NAMED_LIST (class->handler_list),
				  args[0]);

	  if (! handler)
	    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "handler not found");

	  grub_handler_set_current (class, handler);
	}
    }

  return 0;
}

static grub_command_t cmd_handler, cmd_terminal_input, cmd_terminal_output;

GRUB_MOD_INIT(handler)
{
  cmd_handler =
    grub_register_command ("handler", grub_cmd_handler,
			   "handler [class [handler]]",
			   "List or select a handler");
  cmd_terminal_input =
    grub_register_command ("terminal_input", grub_cmd_handler,
			   "terminal_input [handler]",
			   "List or select a handler");
  cmd_terminal_output =
    grub_register_command ("terminal_output", grub_cmd_handler,
			   "terminal_output [handler]",
			   "List or select a handler");
}

GRUB_MOD_FINI(handler)
{
  grub_unregister_command (cmd_handler);
  grub_unregister_command (cmd_terminal_input);
  grub_unregister_command (cmd_terminal_output);
}
