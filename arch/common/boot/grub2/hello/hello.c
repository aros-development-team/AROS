/* hello.c - test module for dynamic loading */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007  Free Software Foundation, Inc.
 *  Copyright (C) 2003  NIIBE Yutaka <gniibe@m17n.org>
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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/extcmd.h>

static grub_err_t
grub_cmd_hello (struct grub_extcmd *cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  grub_printf ("Hello World\n");
  return 0;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(hello)
{
  cmd = grub_register_extcmd ("hello", grub_cmd_hello, GRUB_COMMAND_FLAG_BOTH,
			      "hello", "Say hello", 0);
}

GRUB_MOD_FINI(hello)
{
  grub_unregister_extcmd (cmd);
}
