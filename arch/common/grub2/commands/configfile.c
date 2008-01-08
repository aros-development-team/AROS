/* configfile.c - command to manually load config file  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007  Free Software Foundation, Inc.
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
#include <grub/arg.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/env.h>

static grub_err_t
grub_cmd_configfile (struct grub_arg_list *state __attribute__ ((unused)),
             int argc, char **args)

{
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  grub_cls ();
  grub_env_context_open ();
  grub_normal_execute (args[0], 1);
  grub_env_context_close ();

  return 0;
}

static grub_err_t
grub_cmd_source (struct grub_arg_list *state __attribute__ ((unused)),
		 int argc, char **args)

{
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  grub_normal_execute (args[0], 1);

  return 0;
}


GRUB_MOD_INIT(configfile)
{
  (void) mod;                   /* To stop warning. */
  grub_register_command ("configfile", grub_cmd_configfile,
                        GRUB_COMMAND_FLAG_BOTH, "configfile FILE",
                        "Load another config file.", 0);
  grub_register_command ("source", grub_cmd_source,
                        GRUB_COMMAND_FLAG_BOTH, "source FILE",
                        "Load another config file without changing context.",
			 0);
  grub_register_command (".", grub_cmd_source,
                        GRUB_COMMAND_FLAG_BOTH, ". FILE",
                        "Load another config file without changing context.",
			 0);
}

GRUB_MOD_FINI(configfile)
{
  grub_unregister_command ("configfile");
  grub_unregister_command ("source");
  grub_unregister_command (".");
}
