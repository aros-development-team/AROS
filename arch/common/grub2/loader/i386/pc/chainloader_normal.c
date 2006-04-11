/* chainloader_normal.c - boot another boot loader */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/machine/chainloader.h>
#include <grub/err.h>
#include <grub/normal.h>
#include <grub/dl.h>

static const struct grub_arg_option options[] =
  {
    {"force", 'f', 0, "skip bootsector magic number test", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
chainloader_command (struct grub_arg_list *state,
		     int argc, char **args)
{
  grub_chainloader_flags_t flags = state[0].set ? GRUB_CHAINLOADER_FORCE : 0;
  
  if (argc == 0)
    grub_error (GRUB_ERR_BAD_ARGUMENT, "no file specified");
  else
    grub_chainloader_cmd (args[0], flags);
  return grub_errno;
}

GRUB_MOD_INIT
{
  (void) mod; /* To stop warning.  */
  grub_register_command ("chainloader", chainloader_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "chainloader [-f] FILE",
			 "Prepare to boot another boot loader.", options);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("chainloader");
}
