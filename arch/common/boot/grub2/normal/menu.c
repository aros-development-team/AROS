/* menu.c - General supporting functionality for menus.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/env.h>
#include <grub/menu_viewer.h>
#include <grub/command.h>
#include <grub/parser.h>
#include <grub/auth.h>

/* Get a menu entry by its index in the entry list.  */
grub_menu_entry_t
grub_menu_get_entry (grub_menu_t menu, int no)
{
  grub_menu_entry_t e;

  for (e = menu->entry_list; e && no > 0; e = e->next, no--)
    ;

  return e;
}

/* Return the current timeout. If the variable "timeout" is not set or
   invalid, return -1.  */
int
grub_menu_get_timeout (void)
{
  char *val;
  int timeout;

  val = grub_env_get ("timeout");
  if (! val)
    return -1;

  grub_error_push ();

  timeout = (int) grub_strtoul (val, 0, 0);

  /* If the value is invalid, unset the variable.  */
  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_env_unset ("timeout");
      grub_errno = GRUB_ERR_NONE;
      timeout = -1;
    }

  grub_error_pop ();

  return timeout;
}

/* Set current timeout in the variable "timeout".  */
void
grub_menu_set_timeout (int timeout)
{
  /* Ignore TIMEOUT if it is zero, because it will be unset really soon.  */
  if (timeout > 0)
    {
      char buf[16];

      grub_sprintf (buf, "%d", timeout);
      grub_env_set ("timeout", buf);
    }
}

/* Get the first entry number from the value of the environment variable NAME,
   which is a space-separated list of non-negative integers.  The entry number
   which is returned is stripped from the value of NAME.  If no entry number
   can be found, -1 is returned.  */
static int
get_and_remove_first_entry_number (const char *name)
{
  char *val;
  char *tail;
  int entry;

  val = grub_env_get (name);
  if (! val)
    return -1;

  grub_error_push ();

  entry = (int) grub_strtoul (val, &tail, 0);

  if (grub_errno == GRUB_ERR_NONE)
    {
      /* Skip whitespace to find the next digit.  */
      while (*tail && grub_isspace (*tail))
	tail++;
      grub_env_set (name, tail);
    }
  else
    {
      grub_env_unset (name);
      grub_errno = GRUB_ERR_NONE;
      entry = -1;
    }

  grub_error_pop ();

  return entry;
}

/* Run a menu entry.  */
void
grub_menu_execute_entry(grub_menu_entry_t entry)
{
  grub_err_t err = GRUB_ERR_NONE;

  if (entry->restricted)
    err = grub_auth_check_authentication (entry->users);

  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  grub_parser_execute ((char *) entry->sourcecode);

  if (grub_errno == GRUB_ERR_NONE && grub_loader_is_loaded ())
    /* Implicit execution of boot, only if something is loaded.  */
    grub_command_execute ("boot", 0, 0);
}

/* Execute ENTRY from the menu MENU, falling back to entries specified
   in the environment variable "fallback" if it fails.  CALLBACK is a
   pointer to a struct of function pointers which are used to allow the
   caller provide feedback to the user.  */
void
grub_menu_execute_with_fallback (grub_menu_t menu,
				 grub_menu_entry_t entry,
				 grub_menu_execute_callback_t callback,
				 void *callback_data)
{
  int fallback_entry;

  callback->notify_booting (entry, callback_data);

  grub_menu_execute_entry (entry);

  /* Deal with fallback entries.  */
  while ((fallback_entry = get_and_remove_first_entry_number ("fallback"))
	 >= 0)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      entry = grub_menu_get_entry (menu, fallback_entry);
      callback->notify_fallback (entry, callback_data);
      grub_menu_execute_entry (entry);
      /* If the function call to execute the entry returns at all, then this is
	 taken to indicate a boot failure.  For menu entries that do something
	 other than actually boot an operating system, this could assume
	 incorrectly that something failed.  */
    }

  callback->notify_failure (callback_data);
}
