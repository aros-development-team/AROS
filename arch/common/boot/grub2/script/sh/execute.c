/* execute.c -- Execute a GRUB script.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/script_sh.h>
#include <grub/command.h>
#include <grub/menu.h>
#include <grub/lib/arg.h>
#include <grub/normal.h>

static grub_err_t
grub_script_execute_cmd (struct grub_script_cmd *cmd)
{
  if (cmd == 0)
    return 0;

  return cmd->exec (cmd);
}

/* Parse ARG and return the textual representation.  Add strings are
   concatenated and all values of the variables are filled in.  */
char *
grub_script_execute_argument_to_string (struct grub_script_arg *arg)
{
  int size = 0;
  char *val;
  char *chararg;
  struct grub_script_arg *argi;

  /* First determine the size of the argument.  */
  for (argi = arg; argi; argi = argi->next)
    {
      if (argi->type == 1)
	{
	  val = grub_env_get (argi->str);
	  if (val)
	    size += grub_strlen (val);
	}
      else
	size += grub_strlen (argi->str);
    }

  /* Create the argument.  */
  chararg = grub_malloc (size + 1);
  if (! chararg)
    return 0;

  *chararg = '\0';
  /* First determine the size of the argument.  */
  for (argi = arg; argi; argi = argi->next)
    {
      if (argi->type == 1)
	{
	  val = grub_env_get (argi->str);
	  if (val)
	    grub_strcat (chararg, val);
	}
      else
	grub_strcat (chararg, argi->str);
    }

  return chararg;
}

/* Execute a single command line.  */
grub_err_t
grub_script_execute_cmdline (struct grub_script_cmd *cmd)
{
  struct grub_script_cmdline *cmdline = (struct grub_script_cmdline *) cmd;
  struct grub_script_arglist *arglist;
  char **args = 0;
  int i = 0;
  grub_command_t grubcmd;
  grub_err_t ret = 0;
  int argcount = 0;
  grub_script_function_t func = 0;
  char errnobuf[6];
  char *cmdname;

  /* Lookup the command.  */
  cmdname = grub_script_execute_argument_to_string (cmdline->arglist->arg);
  grubcmd = grub_command_find (cmdname);
  if (! grubcmd)
    {
      /* Ignore errors.  */
      grub_errno = GRUB_ERR_NONE;

      /* It's not a GRUB command, try all functions.  */
      func = grub_script_function_find (cmdname);
      if (! func)
	{
	  /* As a last resort, try if it is an assignment.  */
	  char *assign = grub_strdup (cmdname);
	  char *eq = grub_strchr (assign, '=');

	  if (eq)
	    {
	      /* Create two strings and set the variable.  */
	      *eq = '\0';
	      eq++;
	      grub_env_set (assign, eq);

	      /* This was set because the command was not found.  */
	      grub_errno = GRUB_ERR_NONE;
	    }
	  grub_free (assign);
	  return 0;
	}
    }
  grub_free (cmdname);

  if (cmdline->arglist->next)
    {
      argcount = cmdline->arglist->argcount - 1;

      /* Create argv from the arguments.  */
      args = grub_malloc (sizeof (char *) * argcount);
      for (arglist = cmdline->arglist->next; arglist; arglist = arglist->next)
	{
	  char *str;
	  str = grub_script_execute_argument_to_string (arglist->arg);
	  args[i++] = str;
	}
    }

  /* Execute the GRUB command or function.  */
  if (grubcmd)
    ret = (grubcmd->func) (grubcmd, argcount, args);
  else
    ret = grub_script_function_call (func, argcount, args);

  /* Free arguments.  */
  for (i = 0; i < argcount; i++)
    grub_free (args[i]);
  grub_free (args);

  grub_sprintf (errnobuf, "%d", ret);
  grub_env_set ("?", errnobuf);

  return ret;
}

/* Execute a block of one or more commands.  */
grub_err_t
grub_script_execute_cmdblock (struct grub_script_cmd *cmd)
{
  struct grub_script_cmdblock *cmdblock = (struct grub_script_cmdblock *) cmd;

  /* Loop over every command and execute it.  */
  for (cmd = cmdblock->cmdlist; cmd; cmd = cmd->next)
    grub_script_execute_cmd (cmd);

  return 0;
}

/* Execute an if statement.  */
grub_err_t
grub_script_execute_cmdif (struct grub_script_cmd *cmd)
{
  struct grub_script_cmdif *cmdif = (struct grub_script_cmdif *) cmd;
  char *result;

  /* Check if the commands results in a true or a false.  The value is
     read from the env variable `?'.  */
  grub_script_execute_cmd (cmdif->exec_to_evaluate);
  result = grub_env_get ("?");

  grub_errno = GRUB_ERR_NONE;

  /* Execute the `if' or the `else' part depending on the value of
     `?'.  */
  if (result && ! grub_strcmp (result, "0"))
    return grub_script_execute_cmd (cmdif->exec_on_true);
  else
    return grub_script_execute_cmd (cmdif->exec_on_false);
}

/* Execute the menu entry generate statement.  */
grub_err_t
grub_script_execute_menuentry (struct grub_script_cmd *cmd)
{
  struct grub_script_cmd_menuentry *cmd_menuentry;
  struct grub_script_arglist *arglist;
  char **args = 0;
  int argcount = 0;
  int i = 0;

  cmd_menuentry = (struct grub_script_cmd_menuentry *) cmd;

  if (cmd_menuentry->arglist)
    {
      argcount = cmd_menuentry->arglist->argcount;

      /* Create argv from the arguments.  */
      args = grub_malloc (sizeof (char *) * argcount);

      if (! args)
	{
	  return grub_errno;
	}

      for (arglist = cmd_menuentry->arglist; arglist; arglist = arglist->next)
	{
	  char *str;
	  str = grub_script_execute_argument_to_string (arglist->arg);
	  args[i++] = str;
	}
    }

  grub_normal_add_menu_entry (argcount, (const char **) args,
			      cmd_menuentry->sourcecode);

  /* Free arguments.  */
  for (i = 0; i < argcount; i++)
    grub_free (args[i]);
  grub_free (args);

  return grub_errno;
}



/* Execute any GRUB pre-parsed command or script.  */
grub_err_t
grub_script_execute (struct grub_script *script)
{
  if (script == 0)
    return 0;

  return grub_script_execute_cmd (script->cmd);
}

