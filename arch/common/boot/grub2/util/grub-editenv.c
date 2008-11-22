/* grub-editenv.c - tool to edit environment block.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <grub/util/misc.h>
#include <grub/lib/envblk.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

void
grub_putchar (int c)
{
  putchar (c);
}

void
grub_refresh (void)
{
  fflush (stdout);
}

void *
grub_term_get_current_input (void)
{
  return 0;
}

void *
grub_term_get_current_output (void)
{
  return 0;
}

int
grub_getkey (void)
{
  return 0;
}

char *
grub_env_get (const char *name __attribute__ ((unused)))
{
  return NULL;
}

static struct option options[] = {
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"verbose", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

char buffer[GRUB_ENVBLK_MAXLEN];
grub_envblk_t envblk;

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``grub-editenv --help'' for more information.\n");
  else
    printf ("\
Usage: grub-editenv [OPTIONS] FILENAME COMMAND\n\
\n\
Tool to edit environment block.\n\
\nCommands:\n\
  create                    create a blank environment block file\n\
  info                      show information about the environment block\n\
  list                      list the current variables\n\
  set [name=value] ...      change/delete variables\n\
  clear                     delete all variables\n\
\nOptions:\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n", PACKAGE_BUGREPORT);

  exit (status);
}

int
create_envblk_file (char *name)
{
  FILE *f;
  grub_envblk_t p;

  f = fopen (name, "wb");
  if (! f)
    return 1;

  /* Just in case OS don't save 0s.  */
  memset (buffer, -1, sizeof (buffer));

  p = (grub_envblk_t) &buffer[0];
  p->signature = GRUB_ENVBLK_SIGNATURE;
  p->length = sizeof (buffer) - sizeof (struct grub_envblk);
  p->data[0] = p->data[1] = 0;

  fwrite (buffer, sizeof (buffer), 1, f);

  fclose (f);
  return 0;
}

FILE *
open_envblk_file (char *name)
{
  FILE *f;

  f = fopen (name, "r+b");
  if (! f)
    grub_util_error ("Can\'t open file %s", name);

  if (fread (buffer, 1, sizeof (buffer), f) != sizeof (buffer))
    grub_util_error ("The envblk file is too short");

  envblk = grub_envblk_find (buffer);
  if (! envblk)
    grub_util_error ("Can\'t find environment block");

  return f;
}

static void
cmd_info (void)
{
  printf ("Envblk offset: %ld\n", (long) (envblk->data - buffer));
  printf ("Envblk length: %d\n", envblk->length);
}

static void
cmd_list (void)
{
  auto int hook (char *name, char *value);
  int hook (char *name, char *value)
    {
      printf ("%s=%s\n", name, value);
      return 0;
    }

  grub_envblk_iterate (envblk, hook);
}

static void
cmd_set (int argc, char *argv[])
{
  while (argc)
    {
      char *p;

      p = strchr (argv[0], '=');
      if (! p)
        grub_util_error ("Invalid parameter");

      *(p++) = 0;

      if (*p)
        {
          if (grub_envblk_insert (envblk, argv[0], p))
            grub_util_error ("Environment block too small");
        }
      else
        grub_envblk_delete (envblk, argv[0]);

      argc--;
      argv++;
    }
}

static void
cmd_clear (void)
{
  envblk->data[0] = envblk->data[1] = 0;
}

int
main (int argc, char *argv[])
{
  FILE *f;

  progname = "grub-editenv";

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", progname, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  /* Obtain PATH.  */
  if (optind >= argc)
    {
      fprintf (stderr, "Filename not specified.\n");
      usage (1);
    }

  if (optind + 1 >= argc)
    {
      fprintf (stderr, "Command not specified.\n");
      usage (1);
    }

  if (! strcmp (argv[optind + 1], "create"))
    return create_envblk_file (argv[optind]);

  f = open_envblk_file (argv[optind]);

  optind++;
  if (! strcmp (argv[optind], "info"))
    cmd_info ();
  else if (! strcmp (argv[optind], "list"))
    cmd_list ();
  else
    {
      if (! strcmp (argv[optind], "set"))
        cmd_set (argc - optind - 1, argv + optind + 1);
      else if (! strcmp (argv[optind], "clear"))
        cmd_clear ();

      fseek (f, 0, SEEK_SET);
      fwrite (buffer, sizeof (buffer), 1, f);
    }
  fclose (f);

  return 0;
}
