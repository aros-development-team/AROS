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
#include <getopt.h>

#include "progname.h"

static struct option options[] =
  {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr,
	     "Try ``%s --help'' for more information.\n", program_name);
  else
    printf ("\
Usage: %s [PATH]\n\
\n\
Checks GRUB script configuration file for syntax errors.\n\
\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print the script as it is being processed\n\
\n\
Report bugs to <%s>.\n\
", program_name,
	    PACKAGE_BUGREPORT);
  exit (status);
}

int
main (int argc, char *argv[])
{
  char *argument;
  char *input;
  int lineno = 0;
  FILE *file = 0;
  int verbose = 0;
  int found_input = 0;
  struct grub_script *script = NULL;

  auto grub_err_t get_config_line (char **line, int cont);
  grub_err_t get_config_line (char **line, int cont __attribute__ ((unused)))
  {
    int i;
    char *cmdline = 0;
    size_t len = 0;
    ssize_t read;

    read = getline(&cmdline, &len, (file ?: stdin));
    if (read == -1)
      {
	*line = 0;
	grub_errno = GRUB_ERR_READ_ERROR;

	if (cmdline)
	  free (cmdline);
	return grub_errno;
      }

    if (verbose)
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

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "hvV", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbose = 1;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  /* Obtain ARGUMENT.  */
  if (optind >= argc)
    {
      file = 0; /* read from stdin */
    }
  else if (optind + 1 != argc)
    {
      fprintf (stderr, "Unknown extra argument `%s'.\n", argv[optind + 1]);
      usage (1);
    }
  else
    {
      argument = argv[optind];
      file = fopen (argument, "r");
      if (! file)
	{
	  fprintf (stderr, "%s: %s: %s\n", program_name, argument, strerror(errno));
	  usage (1);
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
      fprintf (stderr, "error: line no: %u\n", lineno);
      return 1;
    }

  return 0;
}
