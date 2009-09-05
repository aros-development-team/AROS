/* menu_viewer.h - Interface to menu viewer implementations. */
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

#ifndef GRUB_MENU_VIEWER_HEADER
#define GRUB_MENU_VIEWER_HEADER 1

#include <grub/err.h>
#include <grub/symbol.h>
#include <grub/types.h>
#include <grub/menu.h>

struct grub_menu_viewer
{
  /* The menu viewer name.  */
  const char *name;

  grub_err_t (*show_menu) (grub_menu_t menu, int nested);

  struct grub_menu_viewer *next;
};
typedef struct grub_menu_viewer *grub_menu_viewer_t;

void grub_menu_viewer_register (grub_menu_viewer_t viewer);

grub_err_t grub_menu_viewer_show_menu (grub_menu_t menu, int nested);

#endif /* GRUB_MENU_VIEWER_HEADER */
