/* chainloader_normal.c - boot another boot loader */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2006,2007  Free Software Foundation, Inc.
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

#include <grub/efi/chainloader.h>
#include <grub/err.h>
#include <grub/normal.h>
#include <grub/dl.h>

static grub_err_t
chainloader_command (struct grub_arg_list *state __attribute__ ((unused)),
		     int argc, char **args)
{
  if (argc == 0)
    grub_error (GRUB_ERR_BAD_ARGUMENT, "no file specified");
  else
    grub_chainloader_cmd (args[0]);
  return grub_errno;
}

GRUB_MOD_INIT(chainloader_normal)
{
  (void) mod; /* To stop warning.  */
  grub_register_command ("chainloader", chainloader_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "chainloader FILE",
			 "Prepare to boot another boot loader.", 0);
}

GRUB_MOD_FINI(chainloader_normal)
{
  grub_unregister_command ("chainloader");
}
