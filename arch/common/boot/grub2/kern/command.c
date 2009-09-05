/* command.c - support basic command */
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
#include <grub/command.h>

grub_command_t grub_command_list;

grub_command_t
grub_register_command_prio (const char *name,
			    grub_command_func_t func,
			    const char *summary,
			    const char *description,
			    int prio)
{
  grub_command_t cmd;

  cmd = (grub_command_t) grub_malloc (sizeof (*cmd));
  if (! cmd)
    return 0;

  cmd->name = name;
  cmd->func = func;
  cmd->summary = (summary) ? summary : name;
  cmd->description = description;

  cmd->flags = GRUB_COMMAND_FLAG_BOTH;
  cmd->prio = prio;
  cmd->data = 0;

  grub_prio_list_insert (GRUB_AS_PRIO_LIST_P (&grub_command_list),
			 GRUB_AS_PRIO_LIST (cmd));

  return cmd;
}

void
grub_unregister_command (grub_command_t cmd)
{
  grub_prio_list_remove (GRUB_AS_PRIO_LIST_P (&grub_command_list),
			 GRUB_AS_PRIO_LIST (cmd));
  grub_free (cmd);
}
