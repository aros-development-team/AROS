/* linux_normal.c - boot another boot loader */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2005  Free Software Foundation, Inc.
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

#include <grub/machine/loader.h>
#include <grub/err.h>
#include <grub/normal.h>
#include <grub/dl.h>

static grub_err_t
grub_normal_linux_command (struct grub_arg_list *state __attribute__ ((unused)),
			   int argc, char **args)
{
  grub_rescue_cmd_linux (argc, args);
  return grub_errno;
}


static grub_err_t
grub_normal_initrd_command (struct grub_arg_list *state __attribute__ ((unused)),
			    int argc, char **args)
{
  grub_rescue_cmd_initrd (argc, args);
  return grub_errno;
}

GRUB_MOD_INIT
{
  (void) mod; /* To stop warning.  */
  grub_register_command ("linux", grub_normal_linux_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "linux FILE [ARGS...]",
			 "Load a linux kernel.", 0);
  
  grub_register_command ("initrd", grub_normal_initrd_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "initrd FILE",
			 "Load an initrd.", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("linux");
  grub_unregister_command ("initrd");
}
