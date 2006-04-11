/* halt.c - command to halt the computer.  */
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
#include <grub/misc.h>
#include <grub/machine/kernel.h>

static grub_err_t
grub_cmd_halt (struct grub_arg_list *state __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))
{
  grub_halt ();
  return 0;
}


#ifdef GRUB_UTIL
void
grub_halt_init (void)
{
  grub_register_command ("halt", grub_cmd_halt, GRUB_COMMAND_FLAG_BOTH,
			 "halt", "halts the computer.  This command does not"
			 " work on all firmware.", 0);
}

void
grub_halt_fini (void)
{
  grub_unregister_command ("halt");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("halt", grub_cmd_halt, GRUB_COMMAND_FLAG_BOTH,
			 "halt", "halts the computer.  This command does not"
			 " work on all firmware.", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("halt");
}
#endif
