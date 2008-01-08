/* env.c - Environment variables */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2006,2007,2008  Free Software Foundation, Inc.
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

#include <grub/env.h>
#include <grub/misc.h>
#include <grub/mm.h>

/* The size of the hash table.  */
#define	HASHSZ	13

/* A hashtable for quick lookup of variables.  */
struct grub_env_context
{
  /* A hash table for variables.  */
  struct grub_env_var *vars[HASHSZ];
  
  /* One level deeper on the stack.  */
  struct grub_env_context *prev;
};

/* This is used for sorting only.  */
struct grub_env_sorted_var
{
  struct grub_env_var *var;
  struct grub_env_sorted_var *next;
};

/* The initial context.  */
static struct grub_env_context initial_context;

/* The current context.  */
static struct grub_env_context *current_context = &initial_context;

/* Return the hash representation of the string S.  */
static unsigned int
grub_env_hashval (const char *s)
{
  unsigned int i = 0;

  /* XXX: This can be done much more efficiently.  */
  while (*s)
    i += 5 * *(s++);

  return i % HASHSZ;
}

static struct grub_env_var *
grub_env_find (const char *name)
{
  struct grub_env_var *var;
  int idx = grub_env_hashval (name);

  /* Look for the variable in the current context.  */
  for (var = current_context->vars[idx]; var; var = var->next)
    if (grub_strcmp (var->name, name) == 0)
      return var;

  return 0;
}

grub_err_t
grub_env_context_open (void)
{
  struct grub_env_context *context;
  int i;

  context = grub_malloc (sizeof (*context));
  if (! context)
    return grub_errno;

  grub_memset (context, 0, sizeof (*context));
  context->prev = current_context;
  current_context = context;

  /* Copy exported variables.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct grub_env_var *var;
      
      for (var = context->prev->vars[i]; var; var = var->next)
	{
	  if (var->type == GRUB_ENV_VAR_GLOBAL)
	    {
	      if (grub_env_set (var->name, var->value) != GRUB_ERR_NONE)
		{
		  grub_env_context_close ();
		  return grub_errno;
		}
	      grub_register_variable_hook (var->name, var->read_hook, var->write_hook);
	    }
	}
    }
  
  return GRUB_ERR_NONE;
}

grub_err_t
grub_env_context_close (void)
{
  struct grub_env_context *context;
  int i;

  if (! current_context->prev)
    grub_fatal ("cannot close the initial context");
  
  /* Free the variables associated with this context.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct grub_env_var *p, *q;
      
      for (p = current_context->prev->vars[i]; p; p = q)
	{
	  q = p->next;
	  grub_free (p);
	}
    }

  /* Restore the previous context.  */
  context = current_context->prev;
  grub_free (current_context);
  current_context = context;

  return GRUB_ERR_NONE;
}

static void
grub_env_insert (struct grub_env_context *context,
		 struct grub_env_var *var)
{
  int idx = grub_env_hashval (var->name);

  /* Insert the variable into the hashtable.  */
  var->prevp = &context->vars[idx];;
  var->next = context->vars[idx];
  if (var->next)
    var->next->prevp = &var;
  context->vars[idx] = var;
}

static void
grub_env_remove (struct grub_env_var *var)
{
  /* Remove the entry from the variable table.  */
  *var->prevp = var->next;
  if (var->next)
    var->next->prevp = var->prevp;
}

grub_err_t
grub_env_export (const char *name)
{
  struct grub_env_var *var;

  var = grub_env_find (name);
  if (var)
    var->type = GRUB_ENV_VAR_GLOBAL;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_env_set (const char *name, const char *val)
{
  struct grub_env_var *var;

  /* If the variable does already exist, just update the variable.  */
  var = grub_env_find (name);
  if (var)
    {
      char *old = var->value;

      if (var->write_hook)
	var->value = var->write_hook (var, val);
      else
	var->value = grub_strdup (val);
      
      if (! var->value)
	{
	  var->value = old;
	  return grub_errno;
	}

      grub_free (old);
      return GRUB_ERR_NONE;
    }

  /* The variable does not exist, so create a new one.  */
  var = grub_malloc (sizeof (*var));
  if (! var)
    return grub_errno;
  
  grub_memset (var, 0, sizeof (*var));

  /* This is not necessary, because GRUB_ENV_VAR_LOCAL == 0. But leave
     this for readability.  */
  var->type = GRUB_ENV_VAR_LOCAL;
  
  var->name = grub_strdup (name);
  if (! var->name)
    goto fail;
  
  var->value = grub_strdup (val);
  if (! var->value)
    goto fail;

  grub_env_insert (current_context, var);

  return GRUB_ERR_NONE;

 fail:
  grub_free (var->name);
  grub_free (var->value);
  grub_free (var);

  return grub_errno;
}

char *
grub_env_get (const char *name)
{
  struct grub_env_var *var;
  
  var = grub_env_find (name);
  if (! var)
    return 0;

  if (var->read_hook)
    return var->read_hook (var, var->value);

  return var->value;
}

void
grub_env_unset (const char *name)
{
  struct grub_env_var *var;
  
  var = grub_env_find (name);
  if (! var)
    return;

  /* XXX: It is not possible to unset variables with a read or write
     hook.  */
  if (var->read_hook || var->write_hook)
    return;

  grub_env_remove (var);

  grub_free (var->name);
  if (var->type != GRUB_ENV_VAR_DATA)
    grub_free (var->value);
  grub_free (var);
}

void
grub_env_iterate (int (*func) (struct grub_env_var *var))
{
  struct grub_env_sorted_var *sorted_list = 0;
  struct grub_env_sorted_var *sorted_var;
  int i;
  
  /* Add variables associated with this context into a sorted list.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct grub_env_var *var;
      
      for (var = current_context->vars[i]; var; var = var->next)
	{
	  struct grub_env_sorted_var *p, **q;

	  /* Ignore data slots.  */
	  if (var->type == GRUB_ENV_VAR_DATA)
	    continue;
	  
	  sorted_var = grub_malloc (sizeof (*sorted_var));
	  if (! sorted_var)
	    goto fail;

	  sorted_var->var = var;

	  for (q = &sorted_list, p = *q; p; q = &((*q)->next), p = *q)
	    {
	      if (grub_strcmp (p->var->name, var->name) > 0)
		break;
	    }
	  
	  sorted_var->next = *q;
	  *q = sorted_var;
	}
    }

  /* Iterate FUNC on the sorted list.  */
  for (sorted_var = sorted_list; sorted_var; sorted_var = sorted_var->next)
    if (func (sorted_var->var))
      break;

 fail:

  /* Free the sorted list.  */
  for (sorted_var = sorted_list; sorted_var; )
    {
      struct grub_env_sorted_var *tmp = sorted_var->next;

      grub_free (sorted_var);
      sorted_var = tmp;
    }
}

grub_err_t
grub_register_variable_hook (const char *name,
			     grub_env_read_hook_t read_hook,
			     grub_env_write_hook_t write_hook)
{
  struct grub_env_var *var = grub_env_find (name);

  if (! var)
    {
      char *val = grub_strdup ("");

      if (! val)
	return grub_errno;
      
      if (grub_env_set (name, val) != GRUB_ERR_NONE)
	return grub_errno;
      
      var = grub_env_find (name);
      /* XXX Insert an assertion?  */
    }
  
  var->read_hook = read_hook;
  var->write_hook = write_hook;

  return GRUB_ERR_NONE;
}

static char *
mangle_data_slot_name (const char *name)
{
  char *mangled_name;

  mangled_name = grub_malloc (grub_strlen (name) + 2);
  if (! mangled_name)
    return 0;
  
  grub_sprintf (mangled_name, "\e%s", name);
  return mangled_name;
}

grub_err_t
grub_env_set_data_slot (const char *name, const void *ptr)
{
  char *mangled_name;
  struct grub_env_var *var;

  mangled_name = mangle_data_slot_name (name);
  if (! mangled_name)
    goto fail;

  /* If the variable does already exist, just update the variable.  */
  var = grub_env_find (mangled_name);
  if (var)
    {
      var->value = (char *) ptr;
      return GRUB_ERR_NONE;
    }

  /* The variable does not exist, so create a new one.  */
  var = grub_malloc (sizeof (*var));
  if (! var)
    goto fail;
  
  grub_memset (var, 0, sizeof (*var));

  var->type = GRUB_ENV_VAR_DATA;
  var->name = mangled_name;
  var->value = (char *) ptr;

  grub_env_insert (current_context, var);

  return GRUB_ERR_NONE;

 fail:

  grub_free (mangled_name);
  return grub_errno;
}

void *
grub_env_get_data_slot (const char *name)
{
  char *mangled_name;
  void *ptr = 0;
  
  mangled_name = mangle_data_slot_name (name);
  if (! mangled_name)
    goto fail;

  ptr = grub_env_get (mangled_name);
  grub_free (mangled_name);

 fail:
  
  return ptr;
}

void
grub_env_unset_data_slot (const char *name)
{
  char *mangled_name;
  
  mangled_name = mangle_data_slot_name (name);
  if (! mangled_name)
    return;

  grub_env_unset (mangled_name);
  grub_free (mangled_name);
}
