/* boot.c - command to boot an operating system */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005  Free Software Foundation, Inc.
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
#include <grub/loader.h>

static grub_err_t
grub_cmd_boot (struct grub_arg_list *state __attribute__ ((unused)),
	       int argc, char **args __attribute__ ((unused)))
{
  if (argc)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "too many arguments");
  
  grub_loader_boot ();
  
  return 0;
}



#ifdef GRUB_UTIL
void
grub_boot_init (void)
{
  grub_register_command ("boot", grub_cmd_boot, GRUB_COMMAND_FLAG_BOTH,
			 "boot", "Boot an operating system.", 0);
}

void
grub_boot_fini (void)
{
  grub_unregister_command ("boot");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("boot", grub_cmd_boot, GRUB_COMMAND_FLAG_BOTH,
			 "boot", "Boot an operating system.", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("boot");
}
#endif /* ! GRUB_UTIL */
