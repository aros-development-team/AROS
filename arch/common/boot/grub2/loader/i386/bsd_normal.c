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

#include <grub/loader.h>
#include <grub/machine/loader.h>
#include <grub/err.h>
#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/aout.h>

static grub_err_t
grub_normal_freebsd_command (struct grub_arg_list *state
			     __attribute__ ((unused)), int argc, char **args)
{
  grub_rescue_cmd_freebsd (argc, args);
  return grub_errno;
}

static grub_err_t
grub_normal_openbsd_command (struct grub_arg_list *state
			     __attribute__ ((unused)), int argc, char **args)
{
  grub_rescue_cmd_openbsd (argc, args);
  return grub_errno;
}

static grub_err_t
grub_normal_netbsd_command (struct grub_arg_list *state
			    __attribute__ ((unused)), int argc, char **args)
{
  grub_rescue_cmd_netbsd (argc, args);
  return grub_errno;
}

static grub_err_t
grub_normal_freebsd_loadenv_command (struct grub_arg_list *state
				     __attribute__ ((unused)), int argc,
				     char **args)
{
  grub_rescue_cmd_freebsd_loadenv (argc, args);
  return grub_errno;
}

static grub_err_t
grub_normal_freebsd_module_command (struct grub_arg_list *state
				    __attribute__ ((unused)), int argc,
				    char **args)
{
  grub_rescue_cmd_freebsd_module (argc, args);
  return grub_errno;
}

GRUB_MOD_INIT (bsd_normal)
{
  (void) mod;			/* To stop warning.  */
  grub_register_command ("freebsd", grub_normal_freebsd_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "freebsd FILE [OPTS] [ARGS...]",
			 "Load freebsd kernel.", 0);
  grub_register_command ("openbsd", grub_normal_openbsd_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "openbsd FILE [OPTS]", "Load openbsd kernel.", 0);
  grub_register_command ("netbsd", grub_normal_netbsd_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "netbsd FILE [OPTS]", "Load netbsd kernel.", 0);

  grub_register_command ("freebsd_loadenv",
			 grub_normal_freebsd_loadenv_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "freebsd_loadenv FILE", "Load freebsd env.", 0);
  grub_register_command ("freebsd_module",
			 grub_normal_freebsd_module_command,
			 GRUB_COMMAND_FLAG_BOTH,
			 "freebsd_module [FILE [type=module_type] [ARGS...]]",
			 "Load freebsd module.", 0);
}

GRUB_MOD_FINI (bsd_normal)
{
  grub_unregister_command ("freebsd");
  grub_unregister_command ("openbsd");
  grub_unregister_command ("netbsd");

  grub_unregister_command ("freebsd_loadenv");
  grub_unregister_command ("freebsd_module");
}
