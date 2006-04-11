/* arg.c - argument parser */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005  Free Software Foundation, Inc.
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

#include <grub/arg.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/normal.h>
#include <grub/term.h>

/* Built-in parser for default options.  */
#define SHORT_ARG_HELP	-100
#define SHORT_ARG_USAGE	-101

static const struct grub_arg_option help_options[] =
  {
    {"help", SHORT_ARG_HELP, 0,
     "display this help and exit", 0, ARG_TYPE_NONE},
    {"usage", SHORT_ARG_USAGE, 0,
     "display the usage of this command and exit", 0, ARG_TYPE_NONE},
    {0, 0, 0, 0, 0, 0}
  };

static struct grub_arg_option *
find_short (const struct grub_arg_option *options, char c)
{
  struct grub_arg_option *found = 0;
  auto struct grub_arg_option *fnd_short (const struct grub_arg_option *opt);

  struct grub_arg_option *fnd_short (const struct grub_arg_option *opt)
    {
      while (opt->doc)
	{
	  if (opt->shortarg == c)
	    return (struct grub_arg_option *) opt;
	  opt++;
	}
      return 0;
    }

  if (options)
    found = fnd_short (options);
  
  if (! found)
    {
      switch (c)
	{
	case 'h':
	  found = (struct grub_arg_option *) help_options;
	  break;

	case 'u':
	  found = (struct grub_arg_option *) (help_options + 1);
	  break;

	default:
	  break;
	}
    }
    
  return found;
}

static char *
find_long_option (char *s)
{
  char *argpos = grub_strchr (s, '=');

  if (argpos)
    {
      *argpos = '\0';
      return ++argpos;
    }
  return 0;
}

static struct grub_arg_option *
find_long (const struct grub_arg_option *options, char *s)
{
  struct grub_arg_option *found = 0;
  auto struct grub_arg_option *fnd_long (const struct grub_arg_option *opt);

  struct grub_arg_option *fnd_long (const struct grub_arg_option *opt)
    {
      while (opt->doc)
	{
	  if (opt->longarg && ! grub_strcmp (opt->longarg, s))
	    return (struct grub_arg_option *) opt;
	  opt++;
	}
      return 0;
    }

  if (options)
    found = fnd_long (options);
  
  if (! found)
    found = fnd_long (help_options);
    
  return found;
}

static void
show_usage (grub_command_t cmd)
{
  grub_printf ("Usage: %s\n", cmd->summary);
}

void
grub_arg_show_help (grub_command_t cmd)
{
  auto void showargs (const struct grub_arg_option *opt);
  int h_is_used = 0;
  int u_is_used = 0;
  
  auto void showargs (const struct grub_arg_option *opt)
    {
      for (; opt->doc; opt++)
	{
	  int spacing = 20;
	  
	  if (opt->shortarg && grub_isgraph (opt->shortarg))
	    grub_printf ("-%c%c ", opt->shortarg, opt->longarg ? ',':' ');
	  else if (opt->shortarg == SHORT_ARG_HELP && ! h_is_used)
	    grub_printf ("-h, ");
	  else if (opt->shortarg == SHORT_ARG_USAGE && ! u_is_used)
	    grub_printf ("-u, ");
	  else
	    grub_printf ("    ");
	  
	  if (opt->longarg)
	    {
	      grub_printf ("--%s", opt->longarg);
	      spacing -= grub_strlen (opt->longarg);
	      
	      if (opt->arg)
		{
		  grub_printf ("=%s", opt->arg);
		  spacing -= grub_strlen (opt->arg) + 1;
		}
	    }

	  while (spacing-- > 0)
	    grub_putchar (' ');

	  grub_printf ("%s\n", opt->doc);

	  switch (opt->shortarg)
	    {
	    case 'h':
	      h_is_used = 1;
	      break;

	    case 'u':
	      u_is_used = 1;
	      break;

	    default:
	      break;
	    }
	}
    }  

  show_usage (cmd);
  grub_printf ("%s\n\n", cmd->description);
  if (cmd->options)
    showargs (cmd->options);
  showargs (help_options);
#if 0
  grub_printf ("\nReport bugs to <%s>.\n", PACKAGE_BUGREPORT);
#endif
}


static int
parse_option (grub_command_t cmd, int key, char *arg, struct grub_arg_list *usr)
{
  switch (key)
    {
    case SHORT_ARG_HELP:
      grub_arg_show_help (cmd);
      return -1;
      
    case SHORT_ARG_USAGE:
      show_usage (cmd);
      return -1;

    default:
      {
	int found = -1;
	int i = 0;
	const struct grub_arg_option *opt = cmd->options;

	while (opt->doc)
	  {
	    if (opt->shortarg && key == opt->shortarg)
	      {
		found = i;
		break;
	      }
	    opt++;
	    i++;
	  }
	
	if (found == -1)
	  return -1;

	usr[found].set = 1;
	usr[found].arg = arg;
      }
    }
  
  return 0;
}

int
grub_arg_parse (grub_command_t cmd, int argc, char **argv,
		struct grub_arg_list *usr, char ***args, int *argnum)
{
  int curarg;
  char *longarg = 0;
  int complete = 0;
  char **argl = 0;
  int num = 0;
  auto grub_err_t add_arg (char *s);

  grub_err_t add_arg (char *s)
    {
      argl = grub_realloc (argl, (++num) * sizeof (char *));
      if (! argl)
	return grub_errno;
      argl[num - 1] = s;
      return 0;
    }


  for (curarg = 0; curarg < argc; curarg++)
    {
      char *arg = argv[curarg];
      struct grub_arg_option *opt;
      char *option = 0;

      /* No option is used.  */
      if (arg[0] != '-' || grub_strlen (arg) == 1)
	{
	  if (add_arg (arg) != 0)
	    goto fail;
  
	  continue;
	}

      /* One or more short options.  */
      if (arg[1] != '-')
	{
	  char *curshort = arg + 1;

	  while (1)
	    {
	      opt = find_short (cmd->options, *curshort);
	      if (! opt)
		{
		  grub_error (GRUB_ERR_BAD_ARGUMENT,
			      "Unknown argument `-%c'\n", *curshort);
		  goto fail;
		}
	      
	      curshort++;

	      /* Parse all arguments here except the last one because
		 it can have an argument value.  */
	      if (*curshort)
		{
		  if (parse_option (cmd, opt->shortarg, 0, usr) || grub_errno)
		    goto fail;
		}
	      else
		{
		  if (opt->type != ARG_TYPE_NONE)
		    {
		      if (curarg + 1 < argc)
			{
			  char *nextarg = argv[curarg + 1];
			  if (!(opt->flags & GRUB_ARG_OPTION_OPTIONAL) 
			      || (grub_strlen (nextarg) < 2 || nextarg[0] != '-'))
			    option = argv[++curarg];
			}
		    }
		  break;
		}
	    }
	  
	}
      else /* The argument starts with "--".  */
	{
	  /* If the argument "--" is used just pass the other
	     arguments.  */
	  if (grub_strlen (arg) == 2)
	    {
	      for (curarg++; curarg < argc; curarg++)
		if (add_arg (arg) != 0)
		  goto fail;
	      break;
	    }

	  longarg = (char *) grub_strdup (arg);
	  if (! longarg)
	    goto fail;

	  option = find_long_option (longarg);
	  arg = longarg;

	  opt = find_long (cmd->options, arg + 2);
	  if (! opt)
	    {
	      grub_error (GRUB_ERR_BAD_ARGUMENT, "Unknown argument `%s'\n", arg);
	      goto fail;
	    }
	}

      if (! (opt->type == ARG_TYPE_NONE 
	     || (! option && (opt->flags & GRUB_ARG_OPTION_OPTIONAL))))
	{
	  if (! option)
	    {
	      grub_error (GRUB_ERR_BAD_ARGUMENT, 
			  "Missing mandatory option for `%s'\n", opt->longarg);
	      goto fail;
	    }
	  
	  switch (opt->type)
	    {
	    case ARG_TYPE_NONE:
	      /* This will never happen.  */
	      break;
	      
	    case ARG_TYPE_STRING:
		  /* No need to do anything.  */
	      break;
	      
	    case ARG_TYPE_INT:
	      {
		char *tail;
		
		grub_strtoul (option, &tail, 0);
		if (tail == 0 || tail == option || *tail != '\0' || grub_errno)
		  {
		    grub_error (GRUB_ERR_BAD_ARGUMENT, 
				"The argument `%s' requires an integer.", 
				arg);

		    goto fail;
		  }
		break;
	      }
	      
	    case ARG_TYPE_DEVICE:
	    case ARG_TYPE_DIR:
	    case ARG_TYPE_FILE:
	    case ARG_TYPE_PATHNAME:
	      /* XXX: Not implemented.  */
	      break;
	    }
	  if (parse_option (cmd, opt->shortarg, option, usr) || grub_errno)
	    goto fail;
	}
      else
	{
	  if (option)
	    {
	      grub_error (GRUB_ERR_BAD_ARGUMENT, 
			  "A value was assigned to the argument `%s' while it "
			  "doesn't require an argument\n", arg);
	      goto fail;
	    }

	  if (parse_option (cmd, opt->shortarg, 0, usr) || grub_errno)
	    goto fail;
	}
      grub_free (longarg);
      longarg = 0;
    }      

  complete = 1;

  *args = argl;
  *argnum = num;

 fail:
  grub_free (longarg);
 
  return complete;
}
