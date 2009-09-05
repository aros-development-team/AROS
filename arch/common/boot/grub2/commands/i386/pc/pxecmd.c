/* pxe.c - command to control the pxe driver  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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
#include <grub/extcmd.h>

static const struct grub_arg_option options[] =
{
    {"info", 'i', 0, "show PXE information.", 0, 0},
    {"bsize", 'b', 0, "set PXE block size", 0, ARG_TYPE_INT},
    {"unload", 'u', 0, "unload PXE stack.", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static void
print_ip (grub_uint32_t ip)
{
  int i;

  for (i = 0; i < 3; i++)
    {
      grub_printf ("%d.", ip & 0xFF);
      ip >>= 8;
    }
  grub_printf ("%d", ip);
}

static grub_err_t
grub_cmd_pxe (grub_extcmd_t cmd, int argc __attribute__ ((unused)),
	      char **args __attribute__ ((unused)))
{
  struct grub_arg_list *state = cmd->state;

  if (! grub_pxe_pxenv)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "no pxe environment");

  if (state[1].set)
    {
      int size;

      size = grub_strtoul (state[1].arg, 0, 0);
      if (size < GRUB_PXE_MIN_BLKSIZE)
        size = GRUB_PXE_MIN_BLKSIZE;
      else if (size > GRUB_PXE_MAX_BLKSIZE)
        size = GRUB_PXE_MAX_BLKSIZE;

      grub_pxe_blksize = size;
    }

  if (state[0].set)
    {
      grub_printf ("blksize : %d\n", grub_pxe_blksize);
      grub_printf ("client ip  : ");
      print_ip (grub_pxe_your_ip);
      grub_printf ("\nserver ip  : ");
      print_ip (grub_pxe_server_ip);
      grub_printf ("\ngateway ip : ");
      print_ip (grub_pxe_gateway_ip);
      grub_printf ("\n");
    }

  if (state[2].set)
    grub_pxe_unload ();

  return 0;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(pxecmd)
{
  cmd = grub_register_extcmd ("pxe", grub_cmd_pxe, GRUB_COMMAND_FLAG_BOTH,
			      "pxe [-i|-b|-u]",
			      "Command to control the PXE device.", options);
}

GRUB_MOD_FINI(pxecmd)
{
  grub_unregister_extcmd (cmd);
}
