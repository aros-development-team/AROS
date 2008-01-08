/* linux_normal.c - boot Linux */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2007  Free Software Foundation, Inc.
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
#include <grub/machine/loader.h>

static const struct grub_arg_option options[] =
  {
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
grub_cmd_linux (struct grub_arg_list *state  __attribute__ ((unused)),
		int argc, char **args)
{
  grub_rescue_cmd_linux (argc, args);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_initrd (struct grub_arg_list *state  __attribute__ ((unused)),
		 int argc, char **args)
{
  grub_rescue_cmd_initrd (argc, args);
  return GRUB_ERR_NONE;
}

GRUB_MOD_INIT(linux_normal)
{
  (void) mod;
  grub_register_command ("linux", grub_cmd_linux, GRUB_COMMAND_FLAG_BOTH,
			 "linux [KERNELARGS...]",
			 "Loads linux", options);
  grub_register_command ("initrd", grub_cmd_initrd, GRUB_COMMAND_FLAG_BOTH,
			 "initrd FILE",
			 "Loads initrd", options);
}

GRUB_MOD_FINI(linux_normal)
{
  grub_unregister_command ("linux");
  grub_unregister_command ("initrd");
}
