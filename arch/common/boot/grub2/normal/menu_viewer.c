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
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/menu_viewer.h>
#include <grub/menu.h>
#include <grub/auth.h>

/* The list of menu viewers.  */
static grub_menu_viewer_t menu_viewer_list;

void
grub_menu_viewer_register (grub_menu_viewer_t viewer)
{
  viewer->next = menu_viewer_list;
  menu_viewer_list = viewer;
}

static grub_menu_viewer_t get_current_menu_viewer (void)
{
  const char *selected_name = grub_env_get ("menuviewer");

  /* If none selected, pick the last registered one. */
  if (selected_name == 0)
    return menu_viewer_list;

  grub_menu_viewer_t cur;
  for (cur = menu_viewer_list; cur; cur = cur->next)
    {
      if (grub_strcmp (cur->name, selected_name) == 0)
        return cur;
    }

  /* Fall back to the first entry (or null).  */
  return menu_viewer_list;
}

grub_err_t
grub_menu_viewer_show_menu (grub_menu_t menu, int nested)
{
  grub_menu_viewer_t cur = get_current_menu_viewer ();
  grub_err_t err1, err2;
  if (!cur)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "No menu viewer available.");

  while (1)
    {
      err1 = cur->show_menu (menu, nested);
      grub_print_error ();

      err2 = grub_auth_check_authentication (NULL);
      if (err2)
	{
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}

      break;
    }

  return err1;
}

