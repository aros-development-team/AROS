/* timeout.c - set the timeout */
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

#include <grub/arg.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/err.h>
#include <grub/dl.h>

static grub_err_t
grub_cmd_timeout (struct grub_arg_list *state __attribute__ ((unused)),
		  int argc, char **args)
{
  grub_menu_t menu;
  
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "timeout value required");

  menu = grub_context_get_current_menu ();
  if (menu)
    menu->timeout = grub_strtoul (args[0], 0, 0);

  return grub_errno;
}



#ifdef GRUB_UTIL
void
grub_timeout_init (void)
{
  grub_register_command ("timeout", grub_cmd_timeout, GRUB_COMMAND_FLAG_MENU,
			 "timeout SECS", "Set the timeout", 0);
}

void
grub_timeout_fini (void)
{
  grub_unregister_command ("timeout");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("timeout", grub_cmd_timeout, GRUB_COMMAND_FLAG_MENU,
			 "timeout SECS", "Set the timeout", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("timeout");
}
#endif /* ! GRUB_UTIL */
