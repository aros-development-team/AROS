/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/dl.h>

static grub_command_t grub_command_list;

grub_command_t
grub_register_command (const char *name,
		       grub_err_t (*func) (struct grub_arg_list *state,
					   int argc, char **args),
		       unsigned flags,
		       const char *summary,
		       const char *description,
		       const struct grub_arg_option *options)
{
  grub_command_t cmd, *p;

  cmd = (grub_command_t) grub_malloc (sizeof (*cmd));
  if (! cmd)
    return 0;

  cmd->name = grub_strdup (name);
  if (! cmd->name)
    {
      grub_free (cmd);
      return 0;
    }
  
  cmd->func = func;
  cmd->flags = flags;
  cmd->summary = summary;
  cmd->description = description;
  cmd->options = options;
  cmd->module_name = 0;

  /* Keep the list sorted for simplicity.  */
  p = &grub_command_list;
  while (*p)
    {
      if (grub_strcmp ((*p)->name, name) >= 0)
	break;

      p = &((*p)->next);
    }

  if (*p && grub_strcmp ((*p)->name, name) == 0)
    {
      grub_command_t q;

      q = *p;
      if (q->flags & GRUB_COMMAND_FLAG_NOT_LOADED)
	{
	  q->func = cmd->func;
	  q->flags = cmd->flags;
	  q->summary = cmd->summary;
	  q->description = cmd->description;
	  q->options = cmd->options;
	  grub_free (cmd->name);
	  grub_free (cmd->module_name);
	  grub_free (cmd);
	  cmd = q;
	}
      else
	{
	  grub_free (cmd->name);
	  grub_free (cmd);
	  cmd = 0;
	}
    }
  else
    {
      cmd->next = *p;
      *p = cmd;
    }

  return cmd;
}

void
grub_unregister_command (const char *name)
{
  grub_command_t *p, q;

  for (p = &grub_command_list, q = *p; q; p = &(q->next), q = q->next)
    if (grub_strcmp (name, q->name) == 0)
      {
        *p = q->next;
	grub_free (q->name);
	grub_free (q->module_name);
        grub_free (q);
        break;
      }
}

grub_command_t
grub_command_find (char *cmdline)
{
  char *first_space;
  grub_command_t cmd;
  int count = 0;
  
  first_space = grub_strchr (cmdline, ' ');
  if (first_space)
    *first_space = '\0';

 again:
  
  for (cmd = grub_command_list; cmd; cmd = cmd->next)
    if (grub_strcmp (cmdline, cmd->name) == 0)
      break;

  if (! cmd)
    grub_error (GRUB_ERR_UNKNOWN_COMMAND, "unknown command `%s'", cmdline);
  else if (cmd->flags & GRUB_COMMAND_FLAG_NOT_LOADED)
    {
      /* Automatically load the command.  */
      if (count == 0)
	{
	  grub_dl_t mod;
	  char *module_name;

	  module_name = grub_strdup (cmd->module_name);
	  if (module_name)
	    {
	      mod = grub_dl_load (module_name);
	      if (mod)
		{
		  grub_dl_ref (mod);
		  count++;
  		  goto again;
		}

	      grub_free (module_name);
	    }
	}

      /* This module seems broken.  */
      grub_unregister_command (cmdline);
      grub_error (GRUB_ERR_UNKNOWN_COMMAND, "unknown command `%s'", cmdline);
      cmd = 0;
    }
  
  if (first_space)
    *first_space = ' ';

  return cmd;
}

int
grub_iterate_commands (int (*iterate) (grub_command_t))
{
  grub_command_t cmd;
  
  for (cmd = grub_command_list; cmd; cmd = cmd->next)
    if (iterate (cmd))
      return 1;
  
  return 0;
}

int
grub_command_execute (char *cmdline, int interactive)
{
  auto grub_err_t cmdline_get (char **s);
  grub_err_t cmdline_get (char **s)
    {
      *s = grub_malloc (GRUB_MAX_CMDLINE);
      *s[0] = '\0';
      return grub_cmdline_get (">", *s, GRUB_MAX_CMDLINE, 0, 1);
    }

  grub_command_t cmd;
  grub_err_t ret = 0;
  char *pager;
  int num;
  char **args;
  struct grub_arg_list *state;
  struct grub_arg_option *parser;
  int maxargs = 0;
  char **arglist;
  int numargs;

  if (grub_split_cmdline (cmdline, cmdline_get, &num, &args))
    return 0;
  
  /* In case of an assignment set the environment accordingly instead
     of calling a function.  */
  if (num == 0 && grub_strchr (args[0], '='))
    {
      char *val;

      if (! interactive)
	grub_printf ("%s\n", cmdline);
      
      val = grub_strchr (args[0], '=');
      val[0] = 0;
      grub_env_set (args[0], val + 1);
      val[0] = '=';
      return 0;
    }
  
  cmd = grub_command_find (args[0]);
  if (! cmd)
    return -1;

  if (! (cmd->flags & GRUB_COMMAND_FLAG_NO_ECHO) && ! interactive)
    grub_printf ("%s\n", cmdline);
  
  /* Enable the pager if the environment pager is set to 1.  */
  if (interactive)
    pager = grub_env_get ("pager");
  else
    pager = 0;
  if (pager && (! grub_strcmp (pager, "1")))
    grub_set_more (1);
  
  parser = (struct grub_arg_option *) cmd->options;
  while (parser && (parser++)->doc)
    maxargs++;

  state = grub_malloc (sizeof (struct grub_arg_list) * maxargs);
  grub_memset (state, 0, sizeof (struct grub_arg_list) * maxargs);
  if (! (cmd->flags & GRUB_COMMAND_FLAG_NO_ARG_PARSE))
    {
      if (grub_arg_parse (cmd, num, &args[1], state, &arglist, &numargs))
	ret = (cmd->func) (state, numargs, arglist);
    }
  else
    ret = (cmd->func) (state, num, &args[1]);
  
  grub_free (state);

  if (pager && (! grub_strcmp (pager, "1")))
    grub_set_more (0);
  
  grub_free (args);
  return ret;
}

static grub_err_t
rescue_command (struct grub_arg_list *state __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  grub_longjmp (grub_exit_env, 0);

  /* Never reach here.  */
  return 0;
}


static grub_err_t
set_command (struct grub_arg_list *state __attribute__ ((unused)),
	     int argc, char **args)
{
  char *var;
  char *val;

  auto int print_env (struct grub_env_var *env);
  int print_env (struct grub_env_var *env)
    {
      grub_printf ("%s=%s\n", env->name, env->value);
      return 0;
    }
  
  if (! argc)
    {
      grub_env_iterate (print_env);
      return 0;
    }
  
  var = args[0];
  val = grub_strchr (var, '=');
  if (! val)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "not an assignment");
      return grub_errno;
    }
  
  val[0] = 0;
  grub_env_set (var, val + 1);
  val[0] = '=';
  return 0;
}

static grub_err_t
unset_command (struct grub_arg_list *state __attribute__ ((unused)),
	       int argc, char **args)
{
  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       "no environment variable specified");

  grub_env_unset (args[0]);
  return 0;
}

static grub_err_t
insmod_command (struct grub_arg_list *state __attribute__ ((unused)),
		int argc, char **args)
{
  char *p;
  grub_dl_t mod;
  
  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");

  p = grub_strchr (args[0], '/');
  if (! p)
    mod = grub_dl_load (args[0]);
  else
    mod = grub_dl_load_file (args[0]);

  if (mod)
    grub_dl_ref (mod);

  return 0;
}

static grub_err_t
rmmod_command (struct grub_arg_list *state __attribute__ ((unused)),
		int argc, char **args)
{
  grub_dl_t mod;
  
  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");
  
  mod = grub_dl_get (args[0]);
  if (! mod)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such module");

  if (! grub_dl_unref (mod))
    grub_dl_unload (mod);

  return 0;
}

static grub_err_t
lsmod_command (struct grub_arg_list *state __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))
{
  auto int print_module (grub_dl_t mod);

  int print_module (grub_dl_t mod)
    {
      grub_dl_dep_t dep;
      
      grub_printf ("%s\t%d\t\t", mod->name, mod->ref_count);
      for (dep = mod->dep; dep; dep = dep->next)
	{
	  if (dep != mod->dep)
	    grub_putchar (',');

	  grub_printf ("%s", dep->mod->name);
	}
      grub_putchar ('\n');
      grub_refresh ();

      return 0;
    }

  grub_printf ("Name\tRef Count\tDependencies\n");
  grub_dl_iterate (print_module);
  return 0;
}

void
grub_command_init (void)
{
  /* This is a special command, because this never be called actually.  */
  grub_register_command ("title", 0, GRUB_COMMAND_FLAG_TITLE, 0, 0, 0);

  grub_register_command ("rescue", rescue_command, GRUB_COMMAND_FLAG_BOTH,
			 "rescue", "Go back to the rescue mode.", 0);

  grub_register_command ("set", set_command, GRUB_COMMAND_FLAG_BOTH,
			 "set [ENVVAR=VALUE]",
			 "Set an environment variable.", 0);

  grub_register_command ("unset", unset_command, GRUB_COMMAND_FLAG_BOTH,
			 "unset ENVVAR", "Remove an environment variable.", 0);

  grub_register_command ("insmod", insmod_command, GRUB_COMMAND_FLAG_BOTH,
			 "insmod MODULE",
			 "Insert a module. The argument can be a file or a module name.",
			 0);

  grub_register_command ("rmmod", rmmod_command, GRUB_COMMAND_FLAG_BOTH,
			 "rmmod MODULE", "Remove a module.", 0);

  grub_register_command ("lsmod", lsmod_command, GRUB_COMMAND_FLAG_BOTH,
			 "lsmod", "Show loaded modules.", 0);
}
