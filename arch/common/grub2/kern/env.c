/* env.c - Environment variables */
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

#include <grub/env.h>
#include <grub/misc.h>
#include <grub/mm.h>

/* The size of the hash table.  */
#define	HASHSZ	13

/* A hashtable for quick lookup of variables.  */
static struct grub_env_var *grub_env[HASHSZ];

/* The variables in a sorted list.  */
static struct grub_env_var *grub_env_sorted;

/* Return the hash representation of the string S.  */
static unsigned int grub_env_hashval (const char *s)
{
  unsigned int i = 0;

  /* XXX: This can be done much more effecient.  */
  while (*s)
    i += 5 * *(s++);

  return i % HASHSZ;
}

static struct grub_env_var *
grub_env_find (const char *name)
{
  struct grub_env_var *var;
  int idx = grub_env_hashval (name);

  for (var = grub_env[idx]; var; var = var->next)
    if (! grub_strcmp (var->name, name))
      return var;
  return 0;
}

grub_err_t
grub_env_set (const char *var, const char *val)
{
  int idx = grub_env_hashval (var);
  struct grub_env_var *env;
  struct grub_env_var *sort;
  struct grub_env_var **sortp;
  
  /* If the variable does already exist, just update the variable.  */
  env = grub_env_find (var);
  if (env)
    {
      char *old = env->value;

      if (env->write_hook)
	env->value = env->write_hook (env, val);
      else
	env->value = grub_strdup (val);
      
      if (! env->value)
	{
	  env->value = old;
	  return grub_errno;
	}

      grub_free (old);
      return 0;
    }

  /* The variable does not exist, create it.  */
  env = grub_malloc (sizeof (struct grub_env_var));
  if (! env)
    return grub_errno;
  
  grub_memset (env, 0, sizeof (struct grub_env_var));
  
  env->name = grub_strdup (var);
  if (! env->name)
    goto fail;
  
  env->value = grub_strdup (val);
  if (! env->name)
    goto fail;
  
  /* Insert it in the hashtable.  */
  env->prevp = &grub_env[idx];
  env->next = grub_env[idx];
  if (grub_env[idx])
    grub_env[idx]->prevp = &env->next;
  grub_env[idx] = env;
  
  /* Insert it in the sorted list.  */
  sortp = &grub_env_sorted;
  sort = grub_env_sorted;
  while (sort)
    {
      if (grub_strcmp (sort->name, var) > 0)
	break;
      
      sortp = &sort->sort_next;
      sort = sort->sort_next;
    }
  env->sort_prevp = sortp;
  env->sort_next = sort;
  if (sort)
    sort->sort_prevp = &env->sort_next;
  *sortp = env;

 fail:
  if (grub_errno)
    {
      grub_free (env->name);
      grub_free (env->value);
      grub_free (env);
    }
  
  return 0;
}

char *
grub_env_get (const char *name)
{
  struct grub_env_var *env;
  env = grub_env_find (name);
  if (! env)
    return 0;

  if (env->read_hook)
    return env->read_hook (env, env->value);

  return env->value;
}

void
grub_env_unset (const char *name)
{
  struct grub_env_var *env;
  env = grub_env_find (name);
  if (! env)
    return;

  /* XXX: It is not possible to unset variables with a read or write
     hook.  */
  if (env->read_hook || env->write_hook)
    return;

  *env->prevp = env->next;
  if (env->next)
    env->next->prevp = env->prevp;

  *env->sort_prevp = env->sort_next;
  if (env->sort_next)
    env->sort_next->sort_prevp = env->sort_prevp;

  grub_free (env->name);
  grub_free (env->value);
  grub_free (env);
  return;
}

void
grub_env_iterate (int (* func) (struct grub_env_var *var))
{
  struct grub_env_var *env;
  
  for (env = grub_env_sorted; env; env = env->sort_next)
    if (func (env))
      return;
}

grub_err_t
grub_register_variable_hook (const char *var,
			     grub_env_read_hook_t read_hook,
			     grub_env_write_hook_t write_hook)
{
  struct grub_env_var *env = grub_env_find (var);

  if (! env)
    {
      char *val = grub_strdup ("");

      if (! val)
	return grub_errno;
      
      if (grub_env_set (var, val) != GRUB_ERR_NONE)
	return grub_errno;
    }
  
  env = grub_env_find (var);
  /* XXX Insert an assertion?  */
  
  env->read_hook = read_hook;
  env->write_hook = write_hook;

  return GRUB_ERR_NONE;
}
