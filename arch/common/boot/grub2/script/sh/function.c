/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2009  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/script_sh.h>
#include <grub/parser.h>
#include <grub/mm.h>

static grub_script_function_t grub_script_function_list;

grub_script_function_t
grub_script_function_create (struct grub_script_arg *functionname_arg,
			     struct grub_script *cmd)
{
  grub_script_function_t func;
  grub_script_function_t *p;

  func = (grub_script_function_t) grub_malloc (sizeof (*func));
  if (! func)
    return 0;

  func->name = grub_script_execute_argument_to_string (functionname_arg);
  if (! func->name)
    {
      grub_free (func);
      return 0;
    }

  func->func = cmd;

  /* Keep the list sorted for simplicity.  */
  p = &grub_script_function_list;
  while (*p)
    {
      if (grub_strcmp ((*p)->name, func->name) >= 0)
	break;

      p = &((*p)->next);
    }

  /* If the function already exists, overwrite the old function.  */
  if (*p && grub_strcmp ((*p)->name, func->name) == 0)
    {
      grub_script_function_t q;

      q = *p;
      grub_script_free (q->func);
      q->func = cmd;
      grub_free (func);
      func = q;
    }
  else
    {
      func->next = *p;
      *p = func;
    }

  return func;
}

void
grub_script_function_remove (const char *name)
{
  grub_script_function_t *p, q;

  for (p = &grub_script_function_list, q = *p; q; p = &(q->next), q = q->next)
    if (grub_strcmp (name, q->name) == 0)
      {
        *p = q->next;
	grub_free (q->name);
	grub_script_free (q->func);
        grub_free (q);
        break;
      }
}

grub_script_function_t
grub_script_function_find (char *functionname)
{
  grub_script_function_t func;

  for (func = grub_script_function_list; func; func = func->next)
    if (grub_strcmp (functionname, func->name) == 0)
      break;

  if (! func)
    grub_error (GRUB_ERR_UNKNOWN_COMMAND, "unknown command `%.20s'", functionname);

  return func;
}

int
grub_script_function_iterate (int (*iterate) (grub_script_function_t))
{
  grub_script_function_t func;

  for (func = grub_script_function_list; func; func = func->next)
    if (iterate (func))
      return 1;

  return 0;
}

int
grub_script_function_call (grub_script_function_t func,
			   int argc __attribute__((unused)),
			   char **args __attribute__((unused)))
{
  /* XXX: Arguments are not supported yet.  */
  return grub_script_execute (func->func);
}
