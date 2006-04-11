/* search.c - search devices based on a file or a filesystem label */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/normal.h>
#include <grub/arg.h>
#include <grub/device.h>
#include <grub/file.h>
#include <grub/env.h>

static const struct grub_arg_option options[] =
  {
    {"file", 'f', 0, "search devices by a file (default)", 0, 0},
    {"label", 'l', 0, "search devices by a filesystem label", 0, 0},
    {"set", 's', GRUB_ARG_OPTION_OPTIONAL, "set a variable to the first device found", "VAR", ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static void
search_label (const char *key, const char *var)
{
  int count = 0;
  auto int iterate_device (const char *name);

  int iterate_device (const char *name)
    {
      grub_device_t dev;
      
      dev = grub_device_open (name);
      if (dev)
	{
	  grub_fs_t fs;
	  
	  fs = grub_fs_probe (dev);
	  if (fs && fs->label)
	    {
	      char *label;
	      
	      (fs->label) (dev, &label);
	      if (grub_errno == GRUB_ERR_NONE && label)
		{
		  if (grub_strcmp (label, key) == 0)
		    {
		      /* Found!  */
		      grub_printf (" %s", name);
		      if (count++ == 0 && var)
			grub_env_set (var, name);
		    }
		  
		  grub_free (label);
		}
	    }
	  
	  grub_device_close (dev);
	}

      grub_errno = GRUB_ERR_NONE;
      return 0;
    }
  
  grub_device_iterate (iterate_device);
  
  if (count == 0)
    grub_error (GRUB_ERR_FILE_NOT_FOUND, "no such device: %s", key);
}

static void
search_file (const char *key, const char *var)
{
  int count = 0;
  char *buf = 0;
  auto int iterate_device (const char *name);

  int iterate_device (const char *name)
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
	  /* Found!  */
	  grub_printf (" %s", name);
	  if (count++ == 0 && var)
	    grub_env_set (var, name);

	  grub_file_close (file);
	}
      
      grub_errno = GRUB_ERR_NONE;
      return 0;
    }
  
  grub_device_iterate (iterate_device);
  
  grub_free (buf);
  
  if (grub_errno == GRUB_ERR_NONE && count == 0)
    grub_error (GRUB_ERR_FILE_NOT_FOUND, "no such device");
}

static grub_err_t
grub_cmd_search (struct grub_arg_list *state, int argc, char **args)
{
  const char *var = 0;
  
  if (argc == 0)
    return grub_error (GRUB_ERR_INVALID_COMMAND, "no argument specified");

  if (state[2].set)
    var = state[2].arg ? : "root";
  
  if (state[1].set)
    search_label (args[0], var);
  else
    search_file (args[0], var);

  return grub_errno;
}

#ifdef GRUB_UTIL
void
grub_search_init (void)
{
  grub_register_command ("search", grub_cmd_search, GRUB_COMMAND_FLAG_BOTH,
			 "search [-f|-l|-s] NAME",
			 "Search devices by a file or a filesystem label."
			 " If --set is specified, the first device found is"
			 " set to a variable. If no variable name is"
			 " specified, \"root\" is used.",
			 options);
}

void
grub_search_fini (void)
{
  grub_unregister_command ("search");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("search", grub_cmd_search, GRUB_COMMAND_FLAG_BOTH,
			 "search [-f|-l|-s] NAME",
			 "Search devices by a file or a filesystem label."
			 " If --set is specified, the first device found is"
			 " set to a variable. If no variable name is"
			 " specified, \"root\" is used.",
			 options);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("search");
}
#endif /* ! GRUB_UTIL */
