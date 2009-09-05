/* handler.c - grub handler function */
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

#include <grub/handler.h>

grub_handler_class_t grub_handler_class_list;

void
grub_handler_register (grub_handler_class_t class, grub_handler_t handler)
{
  int first_handler = (class->handler_list == 0);

  grub_list_push (GRUB_AS_LIST_P (&class->handler_list),
		  GRUB_AS_LIST (handler));

  if (first_handler)
    {
      grub_list_push (GRUB_AS_LIST_P (&grub_handler_class_list),
		      GRUB_AS_LIST (class));
      grub_handler_set_current (class, handler);
    }
}

void
grub_handler_unregister (grub_handler_class_t class, grub_handler_t handler)
{
  grub_list_remove (GRUB_AS_LIST_P (&class->handler_list),
		    GRUB_AS_LIST (handler));

  if (class->handler_list == 0)
    grub_list_remove (GRUB_AS_LIST_P (&grub_handler_class_list),
		      GRUB_AS_LIST (class));
}

grub_err_t
grub_handler_set_current (grub_handler_class_t class, grub_handler_t handler)
{
  if (class->cur_handler && class->cur_handler->fini)
    if ((class->cur_handler->fini) () != GRUB_ERR_NONE)
      return grub_errno;

  if (handler->init)
    if ((handler->init) () != GRUB_ERR_NONE)
      return grub_errno;

  class->cur_handler = handler;
  return GRUB_ERR_NONE;
}
