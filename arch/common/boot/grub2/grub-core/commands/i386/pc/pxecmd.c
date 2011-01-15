/* pxe.c - command to control the pxe driver  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/machine/pxe.h>
#include <grub/command.h>
#include <grub/i18n.h>

static grub_err_t
grub_cmd_pxe_unload (grub_command_t cmd __attribute__ ((unused)),
		     int argc __attribute__ ((unused)),
		     char **args __attribute__ ((unused)))
{
  if (! grub_pxe_pxenv)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "no pxe environment");

  grub_pxe_unload ();

  return 0;
}

static grub_command_t cmd;

GRUB_MOD_INIT(pxecmd)
{
  cmd = grub_register_command ("pxe_unload", grub_cmd_pxe_unload,
			       0,
			       N_("Unload PXE environment."));
}

GRUB_MOD_FINI(pxecmd)
{
  grub_unregister_command (cmd);
}
