/* search.c - search devices based on a file or a filesystem label */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/device.h>
#include <grub/file.h>
#include <grub/env.h>
#include <grub/extcmd.h>

static const struct grub_arg_option options[] =
  {
    {"file",		'f', 0, "search devices by a file", 0, 0},
    {"label",		'l', 0, "search devices by a filesystem label", 0, 0},
    {"fs-uuid",		'u', 0, "search devices by a filesystem UUID", 0, 0},
    {"set",		's', GRUB_ARG_OPTION_OPTIONAL, "set a variable to the first device found", "VAR", ARG_TYPE_STRING},
    {"no-floppy",	'n', 0, "do not probe any floppy drive", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

enum options
  {
    SEARCH_FILE,
    SEARCH_LABEL,
    SEARCH_FS_UUID,
    SEARCH_SET,
    SEARCH_NO_FLOPPY,
 };

static void
search_fs (const char *key, const char *var, int no_floppy, enum options type)
{
  int count = 0;
  char *buf = NULL;
  grub_fs_autoload_hook_t saved_autoload;

  auto int iterate_device (const char *name);
  int iterate_device (const char *name)
  {
    int found = 0;

    /* Skip floppy drives when requested.  */
    if (no_floppy &&
	name[0] == 'f' && name[1] == 'd' && name[2] >= '0' && name[2] <= '9')
      return 0;

    if (type == SEARCH_FILE)
      {
	grub_size_t len;
	char *p;
	grub_file_t file;

	len = grub_strlen (name) + 2 + grub_strlen (key) + 1;
	p = grub_realloc (buf, len);
	if (! p)
	  return 1;

	buf = p;
	grub_sprintf (buf, "(%s)%s", name, key);

	file = grub_file_open (buf);
	if (file)
	  {
	    found = 1;
	    grub_file_close (file);
	  }
      }
    else
      {
	/* type is SEARCH_FS_UUID or SEARCH_LABEL */
	grub_device_t dev;
	grub_fs_t fs;
	int (*compare_fn) (const char *, const char *);
	char *quid;

	dev = grub_device_open (name);
	if (dev)
	  {
	    fs = grub_fs_probe (dev);
	    compare_fn =
	      (type == SEARCH_FS_UUID) ? grub_strcasecmp : grub_strcmp;

	    if (fs && ((type == SEARCH_FS_UUID) ? fs->uuid : fs->label))
	      {
		if (type == SEARCH_FS_UUID)
		  fs->uuid (dev, &quid);
		else
		  fs->label (dev, &quid);

		if (grub_errno == GRUB_ERR_NONE && quid)
		  {
		    if (compare_fn (quid, key) == 0)
		      found = 1;

		    grub_free (quid);
		  }
	      }

	    grub_device_close (dev);
	  }
      }

    if (found)
      {
	count++;
	if (var)
	  grub_env_set (var, name);
	else
	  grub_printf (" %s", name);
      }

    grub_errno = GRUB_ERR_NONE;
    return (found && var);
  }

  /* First try without autoloading if we're setting variable. */
  if (var)
    {
      saved_autoload = grub_fs_autoload_hook;
      grub_fs_autoload_hook = 0;
      grub_device_iterate (iterate_device);

      /* Restore autoload hook.  */
      grub_fs_autoload_hook = saved_autoload;

      /* Retry with autoload if nothing found.  */
      if (grub_errno == GRUB_ERR_NONE && count == 0)
	grub_device_iterate (iterate_device);
    }
  else
    grub_device_iterate (iterate_device);

  grub_free (buf);

  if (grub_errno == GRUB_ERR_NONE && count == 0)
    grub_error (GRUB_ERR_FILE_NOT_FOUND, "no such device: %s", key);
}

static grub_err_t
grub_cmd_search (grub_extcmd_t cmd, int argc, char **args)
{
  struct grub_arg_list *state = cmd->state;
  const char *var = 0;

  if (argc == 0)
    return grub_error (GRUB_ERR_INVALID_COMMAND, "no argument specified");

  if (state[SEARCH_SET].set)
    var = state[SEARCH_SET].arg ? state[SEARCH_SET].arg : "root";

  if (state[SEARCH_LABEL].set)
    search_fs (args[0], var, state[SEARCH_NO_FLOPPY].set, SEARCH_LABEL);
  else if (state[SEARCH_FS_UUID].set)
    search_fs (args[0], var, state[SEARCH_NO_FLOPPY].set, SEARCH_FS_UUID);
  else if (state[SEARCH_FILE].set)
    search_fs (args[0], var, state[SEARCH_NO_FLOPPY].set, SEARCH_FILE);
  else
    return grub_error (GRUB_ERR_INVALID_COMMAND, "unspecified search type");

  return grub_errno;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(search)
{
  cmd =
    grub_register_extcmd ("search", grub_cmd_search,
			  GRUB_COMMAND_FLAG_BOTH,
			  "search [-f|-l|-u|-s|-n] NAME",
			  "Search devices by file, filesystem label or filesystem UUID."
			  " If --set is specified, the first device found is"
			  " set to a variable. If no variable name is"
			  " specified, \"root\" is used.",
			  options);
}

GRUB_MOD_FINI(search)
{
  grub_unregister_extcmd (cmd);
}
