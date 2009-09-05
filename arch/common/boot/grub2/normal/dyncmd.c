/* dyncmd.c - support dynamic command */
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

#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/command.h>
#include <grub/normal.h>

static grub_err_t
grub_dyncmd_dispatcher (struct grub_command *cmd,
			int argc, char **args)
{
  char *modname = cmd->data;
  grub_dl_t mod;
  grub_err_t ret;

  mod = grub_dl_load (modname);
  if (mod)
    {
      char *name;

      grub_free (modname);
      grub_dl_ref (mod);

      name = (char *) cmd->name;
      grub_unregister_command (cmd);

      cmd = grub_command_find (name);
      if (cmd)
	ret = (cmd->func) (cmd, argc, args);
      else
	ret = grub_errno;

      grub_free (name);
    }
  else
    ret = grub_errno;

  return ret;
}

/* Read the file command.lst for auto-loading.  */
void
read_command_list (void)
{
  const char *prefix;
  static int first_time = 1;

  /* Make sure that this function does not get executed twice.  */
  if (! first_time)
    return;
  first_time = 0;

  prefix = grub_env_get ("prefix");
  if (prefix)
    {
      char *filename;

      filename = grub_malloc (grub_strlen (prefix) + sizeof ("/command.lst"));
      if (filename)
	{
	  grub_file_t file;

	  grub_sprintf (filename, "%s/command.lst", prefix);
	  file = grub_file_open (filename);
	  if (file)
	    {
	      char *buf = 0;
	      for (;; grub_free(buf))
		{
		  char *p, *name, *modname;
		  grub_command_t cmd;
		  int prio = 0;

		  buf = grub_file_getline (file);

		  if (! buf)
		    break;

		  name = buf;
		  if (*name == '*')
		    {
		      name++;
		      prio++;
		    }

		  if (! grub_isgraph (name[0]))
		    continue;

		  p = grub_strchr (name, ':');
		  if (! p)
		    continue;

		  *p = '\0';
		  while (*++p == ' ')
		    ;

		  if (! grub_isgraph (*p))
		    continue;

		  if (grub_dl_get (p))
		    continue;

		  name = grub_strdup (name);
		  if (! name)
		    continue;

		  modname = grub_strdup (p);
		  if (! modname)
		    {
		      grub_free (name);
		      continue;
		    }

		  cmd = grub_register_command_prio (name,
						    grub_dyncmd_dispatcher,
						    0, "not loaded", prio);
		  if (! cmd)
		    {
		      grub_free (name);
		      grub_free (modname);
		      continue;
		    }
		  cmd->flags |= GRUB_COMMAND_FLAG_DYNCMD;
		  cmd->data = modname;

		  /* Update the active flag.  */
		  grub_command_find (name);
		}

	      grub_file_close (file);
	    }

	  grub_free (filename);
	}
    }

  /* Ignore errors.  */
  grub_errno = GRUB_ERR_NONE;
}
