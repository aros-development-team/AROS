/* halt.c - command to halt the computer.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008  Free Software Foundation, Inc.
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
#include <grub/command.h>
#include <grub/misc.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_err_t __attribute__ ((noreturn))
grub_cmd_halt (grub_command_t cmd __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))
{
  grub_halt ();
}

static grub_command_t cmd;

GRUB_MOD_INIT(halt)
{
  cmd = grub_register_command ("halt", grub_cmd_halt,
			       0, N_("Halts the computer.  This command does" 
			       " not work on all firmware implementations."));
}

GRUB_MOD_FINI(halt)
{
  grub_unregister_command (cmd);
}
