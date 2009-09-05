/* handler.c - support handler loading */
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
#include <grub/err.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/command.h>
#include <grub/handler.h>
#include <grub/normal.h>

struct grub_handler_list
{
  struct grub_handler_list *next;
  char *name;
  grub_command_t cmd;
};

static grub_list_t handler_list;

static grub_err_t
grub_handler_cmd (struct grub_command *cmd,
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  char *p;
  grub_handler_class_t class;
  grub_handler_t handler;

  p = grub_strchr (cmd->name, '.');
  if (! p)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid command name");

  if (cmd->data)
    {
      if (! grub_dl_get (cmd->data))
	{
	  grub_dl_t mod;

	  mod = grub_dl_load (cmd->data);
	  if (mod)
	    grub_dl_ref (mod);
	  else
	    return grub_errno;
	}
      grub_free (cmd->data);
      cmd->data = 0;
    }

  *p = 0;
  class = grub_named_list_find (GRUB_AS_NAMED_LIST (grub_handler_class_list),
				cmd->name);
  *p = '.';

  if (! class)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "class not found");


  handler = grub_named_list_find (GRUB_AS_NAMED_LIST (class->handler_list),
				  p + 1);
  if (! handler)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "handler not found");

  grub_handler_set_current (class, handler);

  return 0;
}

static void
insert_handler (char *name, char *module)
{
  struct grub_handler_list *item;
  char *data;

  if (grub_command_find (name))
    return;

  item = grub_malloc (sizeof (*item));
  if (! item)
    return;

  item->name = grub_strdup (name);
  if (! item->name)
    {
      grub_free (item);
      return;
    }

  if (module)
    {
      data = grub_strdup (module);
      if (! data)
	{
	  grub_free (item->name);
	  grub_free (item);
	  return;
	}
    }
  else
    data = 0;

  item->cmd = grub_register_command (item->name, grub_handler_cmd, 0,
				     "Set active handler");
  if (! item->cmd)
    {
      grub_free (data);
      grub_free (item->name);
      grub_free (item);
      return;
    }

  item->cmd->data = data;
  grub_list_push (&handler_list, GRUB_AS_LIST (item));
}

/* Read the file handler.lst for auto-loading.  */
void
read_handler_list (void)
{
  const char *prefix;
  static int first_time = 1;
  const char *class_name;

  auto int iterate_handler (grub_handler_t handler);
  int iterate_handler (grub_handler_t handler)
    {
      char name[grub_strlen (class_name) + grub_strlen (handler->name) + 2];

      grub_strcpy (name, class_name);
      grub_strcat (name, ".");
      grub_strcat (name, handler->name);

      insert_handler (name, 0);

      return 0;
    }

  auto int iterate_class (grub_handler_class_t class);
  int iterate_class (grub_handler_class_t class)
    {
      class_name = class->name;
      grub_list_iterate (GRUB_AS_LIST (class->handler_list),
			 (grub_list_hook_t) iterate_handler);

      return 0;
    }

  /* Make sure that this function does not get executed twice.  */
  if (! first_time)
    return;
  first_time = 0;

  prefix = grub_env_get ("prefix");
  if (prefix)
    {
      char *filename;

      filename = grub_malloc (grub_strlen (prefix) + sizeof ("/handler.lst"));
      if (filename)
	{
	  grub_file_t file;

	  grub_sprintf (filename, "%s/handler.lst", prefix);
	  file = grub_file_open (filename);
	  if (file)
	    {
	      char *buf = 0;
	      for (;; grub_free(buf))
		{
		  char *p;

		  buf = grub_file_getline (file);

		  if (! buf)
		    break;

		  if (! grub_isgraph (buf[0]))
		    continue;

		  p = grub_strchr (buf, ':');
		  if (! p)
		    continue;

		  *p = '\0';
		  while (*++p == ' ')
		    ;

		  insert_handler (buf, p);
		}
	      grub_file_close (file);
	    }
	  grub_free (filename);
	}
    }

  grub_list_iterate (GRUB_AS_LIST (grub_handler_class_list),
		     (grub_list_hook_t) iterate_class);

  /* Ignore errors.  */
  grub_errno = GRUB_ERR_NONE;
}

void
free_handler_list (void)
{
  struct grub_handler_list *item;

  while ((item = grub_list_pop (&handler_list)) != 0)
    {
      grub_free (item->cmd->data);
      grub_unregister_command (item->cmd);
      grub_free (item->name);
      grub_free (item);
    }
}
