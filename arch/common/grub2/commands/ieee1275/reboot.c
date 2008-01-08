/* reboot.c - command to reboot the computer.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/machine/kernel.h>

static grub_err_t
grub_cmd_reboot (struct grub_arg_list *state __attribute__ ((unused)),
		 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))
{
  grub_reboot ();
  return 0;
}


GRUB_MOD_INIT(ieee1275_reboot)
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("reboot", grub_cmd_reboot, GRUB_COMMAND_FLAG_BOTH,
			 "reboot", "Reboot the computer", 0);
}

GRUB_MOD_FINI(ieee1275_reboot)
{
  grub_unregister_command ("reboot");
}
