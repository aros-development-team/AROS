/*
 *  Copyright (C) 2008,2010  Free Software Foundation, Inc.
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
#include <stdio.h>
#include <stdlib.h>

#define _GNU_SOURCE	1
#include <getopt.h>

#include "progname.h"

static struct option options[] =
  {
    {"help", no_argument, 0, 'h' },
    {"version", no_argument, 0, 'V' },
    {0, 0, 0, 0 }
  };

static void __attribute__ ((noreturn))
usage (int status)
{
  if (status)
    fprintf (stderr,
	     "Try ``%s --help'' for more information.\n", program_name);
  else
    printf ("\
Usage: %s [OPTIONS] SYMBOL-NAME\n\
\n\
Convert a binary file to a C header.\n\
\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
\n\
Report bugs to <%s>.\n\
", program_name, PACKAGE_BUGREPORT);

  exit (status);
}

int
main (int argc, char *argv[])
{
  int b, i;
  char *sym;

  set_program_name (argv[0]);

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "snm:r:hVv", options, 0);

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

	  default:
	    usage (1);
	    break;
	  }
    }

  if (optind >= argc)
    usage (1);
  
  if (optind + 1 != argc)
    usage (1);

  sym = argv[optind];

  b = getchar ();
  if (b == EOF)
    goto abort;

  printf ("/* THIS CHUNK OF BYTES IS AUTOMATICALLY GENERATED */\n"
	  "unsigned char %s[] =\n{\n", sym);

  while (1)
    {
      printf ("0x%02x", b);

      b = getchar ();
      if (b == EOF)
	goto end;

      for (i = 0; i < 16 - 1; i++)
	{
	  printf (", 0x%02x", b);

	  b = getchar ();
	  if (b == EOF)
	    goto end;
	}

      printf (",\n");
    }

end:
  printf ("\n};\n");

abort:
  exit (0);
}
