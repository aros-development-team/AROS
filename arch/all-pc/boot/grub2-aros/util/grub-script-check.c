/* grub-script-check.c - check grub script file for syntax errors */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010  Free Software Foundation, Inc.
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

#include <config.h>
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/i18n.h>
#include <grub/parser.h>
#include <grub/script_sh.h>

#define _GNU_SOURCE	1

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#include "progname.h"

struct arguments
{
  int verbose;
  char *filename;
};

static struct argp_option options[] = {
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  { 0, 0, 0, 0, 0, 0 }
};

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'v':
      arguments->verbose = 1;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num == 0)
	arguments->filename = xstrdup (arg);
      else
	{
	  /* Too many arguments. */
	  fprintf (stderr, _("Unknown extra argument `%s'."), arg);
	  fprintf (stderr, "\n");
	  argp_usage (state);
	}
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("[PATH]"),
  N_("Checks GRUB script configuration file for syntax errors."),
  NULL, NULL, NULL
};

int
main (int argc, char *argv[])
{
  char *input;
  int lineno = 0;
  FILE *file = 0;
  struct arguments arguments;
  int found_input = 0;
  struct grub_script *script = NULL;

  auto grub_err_t get_config_line (char **line, int cont);
  grub_err_t get_config_line (char **line, int cont __attribute__ ((unused)))
  {
    int i;
    char *cmdline = 0;
    size_t len = 0;
    ssize_t curread;

    curread = getline(&cmdline, &len, (file ?: stdin));
    if (curread == -1)
      {
	*line = 0;
	grub_errno = GRUB_ERR_READ_ERROR;

	if (cmdline)
	  free (cmdline);
	return grub_errno;
      }

    if (arguments.verbose)
      grub_printf("%s", cmdline);

    for (i = 0; cmdline[i] != '\0'; i++)
      {
	/* Replace tabs and carriage returns with spaces.  */
	if (cmdline[i] == '\t' || cmdline[i] == '\r')
	  cmdline[i] = ' ';

	/* Replace '\n' with '\0'.  */
	if (cmdline[i] == '\n')
	  cmdline[i] = '\0';
      }

    lineno++;
    *line = grub_strdup (cmdline);

    free (cmdline);
    return 0;
  }

  set_program_name (argv[0]);
  grub_util_init_nls ();

  memset (&arguments, 0, sizeof (struct arguments));

  /* Check for options.  */
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  /* Obtain ARGUMENT.  */
  if (!arguments.filename)
    {
      file = 0; /* read from stdin */
    }
  else
    {
      file = fopen (arguments.filename, "r");
      if (! file)
	{
          char *program = xstrdup(program_name);
	  fprintf (stderr, "%s: %s: %s\n", program_name, 
		   arguments.filename, strerror(errno));
          argp_help (&argp, stderr, ARGP_HELP_STD_USAGE, program);
          free(program);
          exit(1);
	}
    }

  do
    {
      input = 0;
      get_config_line(&input, 0);
      if (! input) 
	break;
      found_input = 1;

      script = grub_script_parse (input, get_config_line);
      if (script)
	{
	  grub_script_execute (script);
	  grub_script_free (script);
	}

      grub_free (input);
    } while (script != 0);

  if (file) fclose (file);

  if (found_input && script == 0)
    {
      fprintf (stderr, _("Syntax error at line %u\n"), lineno);
      return 1;
    }

  return 0;
}
