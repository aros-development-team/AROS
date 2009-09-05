/* grub-editenv.c - tool to edit environment block.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009 Free Software Foundation, Inc.
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
#include <grub/handler.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#define DEFAULT_ENVBLK_SIZE	1024

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

struct grub_handler_class grub_term_input_class;
struct grub_handler_class grub_term_output_class;

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
  list                      list the current variables\n\
  set [name=value ...]      set variables\n\
  unset [name ....]         delete variables\n\
\nOptions:\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n", PACKAGE_BUGREPORT);

  exit (status);
}

static void
create_envblk_file (const char *name)
{
  FILE *fp;
  char *buf;

  buf = malloc (DEFAULT_ENVBLK_SIZE);
  if (! buf)
    grub_util_error ("out of memory");

  fp = fopen (name, "wb");
  if (! fp)
    grub_util_error ("cannot open the file %s", name);

  memcpy (buf, GRUB_ENVBLK_SIGNATURE, sizeof (GRUB_ENVBLK_SIGNATURE) - 1);
  memset (buf + sizeof (GRUB_ENVBLK_SIGNATURE) - 1, '#',
          DEFAULT_ENVBLK_SIZE - sizeof (GRUB_ENVBLK_SIGNATURE) + 1);

  if (fwrite (buf, 1, DEFAULT_ENVBLK_SIZE, fp) != DEFAULT_ENVBLK_SIZE)
    grub_util_error ("cannot write to the file %s", name);

  fsync (fileno (fp));
  free (buf);
  fclose (fp);
}

static grub_envblk_t
open_envblk_file (const char *name)
{
  FILE *fp;
  char *buf;
  size_t size;
  grub_envblk_t envblk;

  fp = fopen (name, "rb");
  if (! fp)
    {
      /* Create the file implicitly.  */
      create_envblk_file (name);
      fp = fopen (name, "rb");
      if (! fp)
        grub_util_error ("cannot open the file %s", name);
    }

  if (fseek (fp, 0, SEEK_END) < 0)
    grub_util_error ("cannot seek the file %s", name);

  size = (size_t) ftell (fp);

  if (fseek (fp, 0, SEEK_SET) < 0)
    grub_util_error ("cannot seek the file %s", name);

  buf = malloc (size);
  if (! buf)
    grub_util_error ("out of memory");

  if (fread (buf, 1, size, fp) != size)
    grub_util_error ("cannot read the file %s", name);

  fclose (fp);

  envblk = grub_envblk_open (buf, size);
  if (! envblk)
    grub_util_error ("invalid environment block");

  return envblk;
}

static void
list_variables (const char *name)
{
  grub_envblk_t envblk;

  auto int print_var (const char *name, const char *value);
  int print_var (const char *name, const char *value)
    {
      printf ("%s=%s\n", name, value);
      return 0;
    }

  envblk = open_envblk_file (name);
  grub_envblk_iterate (envblk, print_var);
  grub_envblk_close (envblk);
}

static void
write_envblk (const char *name, grub_envblk_t envblk)
{
  FILE *fp;

  fp = fopen (name, "wb");
  if (! fp)
    grub_util_error ("cannot open the file %s", name);

  if (fwrite (grub_envblk_buffer (envblk), 1, grub_envblk_size (envblk), fp)
      != grub_envblk_size (envblk))
    grub_util_error ("cannot write to the file %s", name);

  fsync (fileno (fp));
  fclose (fp);
}

static void
set_variables (const char *name, int argc, char *argv[])
{
  grub_envblk_t envblk;

  envblk = open_envblk_file (name);
  while (argc)
    {
      char *p;

      p = strchr (argv[0], '=');
      if (! p)
        grub_util_error ("invalid parameter %s", argv[0]);

      *(p++) = 0;

      if (! grub_envblk_set (envblk, argv[0], p))
        grub_util_error ("environment block too small");

      argc--;
      argv++;
    }

  write_envblk (name, envblk);
  grub_envblk_close (envblk);
}

static void
unset_variables (const char *name, int argc, char *argv[])
{
  grub_envblk_t envblk;

  envblk = open_envblk_file (name);
  while (argc)
    {
      grub_envblk_delete (envblk, argv[0]);

      argc--;
      argv++;
    }

  write_envblk (name, envblk);
  grub_envblk_close (envblk);
}

int
main (int argc, char *argv[])
{
  char *filename;
  char *command;

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

  /* Obtain the filename.  */
  if (optind >= argc)
    {
      fprintf (stderr, "no filename specified\n");
      usage (1);
    }

  if (optind + 1 >= argc)
    {
      fprintf (stderr, "no command specified\n");
      usage (1);
    }

  filename = argv[optind];
  command = argv[optind + 1];

  if (strcmp (command, "create") == 0)
    create_envblk_file (filename);
  else if (strcmp (command, "list") == 0)
    list_variables (filename);
  else if (strcmp (command, "set") == 0)
    set_variables (filename, argc - optind - 2, argv + optind + 2);
  else if (strcmp (command, "unset") == 0)
    unset_variables (filename, argc - optind - 2, argv + optind + 2);
  else
    {
      fprintf (stderr, "unknown command %s\n", command);
      usage (1);
    }

  return 0;
}
