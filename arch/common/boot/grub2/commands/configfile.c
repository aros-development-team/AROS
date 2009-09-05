/* configfile.c - command to manually load config file  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2009  Free Software Foundation, Inc.
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
#include <grub/term.h>
#include <grub/env.h>
#include <grub/normal.h>
#include <grub/command.h>

static grub_err_t
grub_cmd_source (grub_command_t cmd, int argc, char **args)
{
  int new_env;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  new_env = (cmd->name[0] == 'c');

  if (new_env)
    {
      grub_cls ();
      grub_env_context_open (1);
    }

  grub_normal_execute (args[0], 1, ! new_env);

  if (new_env)
    grub_env_context_close ();

  return 0;
}

static grub_command_t cmd_configfile, cmd_source, cmd_dot;

GRUB_MOD_INIT(configfile)
{
  cmd_configfile =
    grub_register_command ("configfile", grub_cmd_source,
			   "configfile FILE", "Load another config file.");
  cmd_source =
    grub_register_command ("source", grub_cmd_source,
			   "source FILE",
			   "Load another config file without changing context."
			   );
  cmd_dot =
    grub_register_command (".", grub_cmd_source,
			   ". FILE",
			   "Load another config file without changing context."
			   );
}

GRUB_MOD_FINI(configfile)
{
  grub_unregister_command (cmd_configfile);
  grub_unregister_command (cmd_source);
  grub_unregister_command (cmd_dot);
}
