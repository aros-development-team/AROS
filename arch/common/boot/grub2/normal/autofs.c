/* autofs.c - support auto-loading from fs.lst */
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
#include <grub/dl.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/fs.h>
#include <grub/normal.h>

/* This is used to store the names of filesystem modules for auto-loading.  */
struct grub_fs_module_list
{
  char *name;
  struct grub_fs_module_list *next;
};
typedef struct grub_fs_module_list *grub_fs_module_list_t;

static grub_fs_module_list_t fs_module_list = 0;

/* The auto-loading hook for filesystems.  */
static int
autoload_fs_module (void)
{
  grub_fs_module_list_t p;

  while ((p = fs_module_list) != 0)
    {
      if (! grub_dl_get (p->name) && grub_dl_load (p->name))
	return 1;

      fs_module_list = p->next;
      grub_free (p->name);
      grub_free (p);
    }

  return 0;
}

/* Read the file fs.lst for auto-loading.  */
void
read_fs_list (void)
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

      filename = grub_malloc (grub_strlen (prefix) + sizeof ("/fs.lst"));
      if (filename)
	{
	  grub_file_t file;

	  grub_sprintf (filename, "%s/fs.lst", prefix);
	  file = grub_file_open (filename);
	  if (file)
	    {
	      while (1)
		{
		  char *buf;
		  char *p;
		  char *q;
		  grub_fs_module_list_t fs_mod;

		  buf = grub_file_getline (file);
		  if (! buf)
		    break;

		  p = buf;
		  q = buf + grub_strlen (buf) - 1;

		  /* Ignore space.  */
		  while (grub_isspace (*p))
		    p++;

		  while (p < q && grub_isspace (*q))
		    *q-- = '\0';

		  /* If the line is empty, skip it.  */
		  if (p >= q)
		    continue;

		  fs_mod = grub_malloc (sizeof (*fs_mod));
		  if (! fs_mod)
		    continue;

		  fs_mod->name = grub_strdup (p);
		  if (! fs_mod->name)
		    {
		      grub_free (fs_mod);
		      continue;
		    }

		  fs_mod->next = fs_module_list;
		  fs_module_list = fs_mod;
		}

	      grub_file_close (file);
	    }

	  grub_free (filename);
	}
    }

  /* Ignore errors.  */
  grub_errno = GRUB_ERR_NONE;

  /* Set the hook.  */
  grub_fs_autoload_hook = autoload_fs_module;
}
