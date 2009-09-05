/* handler.h - header for grub handler */
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

#ifndef GRUB_HANDLER_HEADER
#define GRUB_HANDLER_HEADER 1

#include <grub/list.h>
#include <grub/err.h>

struct grub_handler
{
  struct grub_handler *next;
  const char *name;
  grub_err_t (*init) (void);
  grub_err_t (*fini) (void);
};
typedef struct grub_handler *grub_handler_t;

struct grub_handler_class
{
  struct grub_handler_class *next;
  const char *name;
  grub_handler_t handler_list;
  grub_handler_t cur_handler;
};
typedef struct grub_handler_class *grub_handler_class_t;

extern grub_handler_class_t EXPORT_VAR(grub_handler_class_list);

void EXPORT_FUNC(grub_handler_register) (grub_handler_class_t class,
					 grub_handler_t handler);
void EXPORT_FUNC(grub_handler_unregister) (grub_handler_class_t class,
					   grub_handler_t handler);
grub_err_t EXPORT_FUNC(grub_handler_set_current) (grub_handler_class_t class,
						  grub_handler_t handler);

#define GRUB_AS_HANDLER(ptr) \
  ((GRUB_FIELD_MATCH (ptr, grub_handler_t, next) && \
    GRUB_FIELD_MATCH (ptr, grub_handler_t, name) && \
    GRUB_FIELD_MATCH (ptr, grub_handler_t, init) && \
    GRUB_FIELD_MATCH (ptr, grub_handler_t, fini)) ? \
   (grub_handler_t) ptr : grub_assert_fail ())

#endif /* ! GRUB_HANDLER_HEADER */
