/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/normal.h>
#include <grub/mm.h>

static struct grub_context context =
  {
    .menu_list = 0
  };

/* Return a pointer to the context.  */
grub_context_t
grub_context_get (void)
{
  return &context;
}

/* Return the current menu.  */
grub_menu_t
grub_context_get_current_menu (void)
{
  if (context.menu_list)
    return context.menu_list->menu;

  return 0;
}

/* Push a new menu. Return this menu. If any error occurs, return NULL.  */
grub_menu_t
grub_context_push_menu (grub_menu_t menu)
{
  grub_menu_list_t menu_list;

  menu_list = grub_malloc (sizeof (*menu_list));
  if (! menu_list)
    return 0;

  menu_list->menu = menu;
  menu_list->next = context.menu_list;
  context.menu_list = menu_list;

  return menu;
}

/* Pop a menu.  */
void
grub_context_pop_menu (void)
{
  grub_menu_list_t menu_list;

  menu_list = context.menu_list;
  if (menu_list)
    {
      context.menu_list = menu_list->next;
      grub_free (menu_list);
    }
}

