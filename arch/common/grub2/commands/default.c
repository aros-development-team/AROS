/* default.c - set the default boot entry */
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

/* This is a simple version. This should support a saved boot entry,
   a label, etc.  */
static grub_err_t
grub_cmd_default (struct grub_arg_list *state __attribute__ ((unused)),
		  int argc, char **args)
{
  grub_menu_t menu;
  
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "entry number required");

  menu = grub_context_get_current_menu ();
  if (menu)
    menu->default_entry = grub_strtoul (args[0], 0, 0);

  return grub_errno;
}



#ifdef GRUB_UTIL
void
grub_default_init (void)
{
  grub_register_command ("default", grub_cmd_default, GRUB_COMMAND_FLAG_MENU,
			 "default ENTRY", "Set the default entry", 0);
}

void
grub_default_fini (void)
{
  grub_unregister_command ("default");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("default", grub_cmd_default, GRUB_COMMAND_FLAG_MENU,
			 "default ENTRY", "Set the default entry", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("default");
}
#endif /* ! GRUB_UTIL */
