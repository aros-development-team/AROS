/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005  Free Software Foundation, Inc.
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_ENV_HEADER
#define GRUB_ENV_HEADER	1

#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/types.h>

struct grub_env_var;

typedef char *(*grub_env_read_hook_t) (struct grub_env_var *var,
				       const char *val);
typedef char *(*grub_env_write_hook_t) (struct grub_env_var *var,
					const char *val);

struct grub_env_var
{
  char *name;
  char *value;
  grub_env_read_hook_t read_hook;
  grub_env_write_hook_t write_hook;
  struct grub_env_var *next;
  struct grub_env_var **prevp;
  struct grub_env_var *sort_next;
  struct grub_env_var **sort_prevp;
};

grub_err_t EXPORT_FUNC(grub_env_set) (const char *var, const char *val);
char *EXPORT_FUNC(grub_env_get) (const char *name);
void EXPORT_FUNC(grub_env_unset) (const char *name);
void EXPORT_FUNC(grub_env_iterate) (int (* func) (struct grub_env_var *var));
grub_err_t EXPORT_FUNC(grub_register_variable_hook) (const char *var,
						     grub_env_read_hook_t read_hook,
						     grub_env_write_hook_t write_hook);

#endif /* ! GRUB_ENV_HEADER */
