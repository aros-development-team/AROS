/* extcmd.c - support extended command */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/list.h>
#include <grub/misc.h>
#include <grub/extcmd.h>

static grub_err_t
grub_extcmd_dispatcher (struct grub_command *cmd,
			int argc, char **args)
{
  int new_argc;
  char **new_args;
  struct grub_arg_option *parser;
  struct grub_arg_list *state;
  int maxargs = 0;
  grub_err_t ret;
  grub_extcmd_t ext;

  ext = cmd->data;
  parser = (struct grub_arg_option *) ext->options;
  while (parser && (parser++)->doc)
    maxargs++;

  /* Set up the option state.  */
  state = grub_zalloc (sizeof (struct grub_arg_list) * maxargs);

  if (grub_arg_parse (ext, argc, args, state, &new_args, &new_argc))
    {
      ext->state = state;
      ret = (ext->func) (ext, new_argc, new_args);
      grub_free (new_args);
    }
  else
    ret = grub_errno;

  grub_free (state);

  return ret;
}

grub_extcmd_t
grub_register_extcmd (const char *name, grub_extcmd_func_t func,
		      unsigned flags, const char *summary,
		      const char *description,
		      const struct grub_arg_option *parser)
{
  grub_extcmd_t ext;
  grub_command_t cmd;

  ext = (grub_extcmd_t) grub_malloc (sizeof (*ext));
  if (! ext)
    return 0;

  cmd = grub_register_command_prio (name, grub_extcmd_dispatcher,
				    summary, description, 1);
  if (! cmd)
    {
      grub_free (ext);
      return 0;
    }

  cmd->flags = (flags | GRUB_COMMAND_FLAG_EXTCMD);
  cmd->data = ext;

  ext->cmd = cmd;
  ext->func = func;
  ext->options = parser;
  ext->data = 0;

  return ext;
}

void
grub_unregister_extcmd (grub_extcmd_t ext)
{
  grub_unregister_command (ext->cmd);
  grub_free (ext);
}
