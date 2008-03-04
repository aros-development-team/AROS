/* grub-fstest.c - debug tool for filesystem driver */
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
#include <grub/misc.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/fs.h>
#include <grub/env.h>
#include <grub/term.h>
#include <grub/mm.h>
#include <grub/normal.h>
#include <grub/hexdump.h>

#include <grub_fstest_init.h>

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

int
grub_getkey (void)
{
  return -1;
}

grub_term_t
grub_term_get_current (void)
{
  return 0;
}

void
grub_refresh (void)
{
}

static struct grub_command cmd_loopback;
static struct grub_command cmd_blocklist;
static struct grub_command cmd_ls;

grub_command_t
grub_register_command (const char *name,
		       grub_err_t (*func) (struct grub_arg_list * state,
					   int argc, char **args),
		       unsigned flags,
		       const char *summary __attribute__ ((unused)),
		       const char *description __attribute__ ((unused)),
		       const struct grub_arg_option *options)
{
  grub_command_t cmd = 0;

  if (!grub_strcmp (name, "loopback"))
    cmd = &cmd_loopback;
  else if (!grub_strcmp (name, "blocklist"))
    cmd = &cmd_blocklist;
  else if (!grub_strcmp (name, "ls"))
    cmd = &cmd_ls;

  if (cmd)
    {
      cmd->func = func;
      cmd->flags = flags;
      cmd->options = options;
    }
  return NULL;
}

grub_err_t
execute_command (grub_command_t cmd, int n, char **args)
{
  int maxargs = 0;
  grub_err_t ret = 0;
  struct grub_arg_list *state;
  struct grub_arg_option *parser;
  char **parsed_arglist;
  int numargs;

  /* Count the amount of options the command has.  */
  parser = (struct grub_arg_option *) cmd->options;
  while (parser && (parser++)->doc)
    maxargs++;

  /* Set up the option state.  */
  state = grub_malloc (sizeof (struct grub_arg_list) * maxargs);
  grub_memset (state, 0, sizeof (struct grub_arg_list) * maxargs);

  /* Start the command.  */
  if (!(cmd->flags & GRUB_COMMAND_FLAG_NO_ARG_PARSE))
    {
      if (grub_arg_parse (cmd, n, args, state, &parsed_arglist, &numargs))
	ret = (cmd->func) (state, numargs, parsed_arglist);
    }
  else
    ret = (cmd->func) (state, n, args);

  grub_free (state);

  return ret;
}

void
grub_unregister_command (const char *name __attribute__ ((unused)))
{
}

#define CMD_LS          1
#define CMD_CP          2
#define CMD_CMP         3
#define CMD_HEX         4
#define CMD_BLOCKLIST   5

#define BUF_SIZE  32256

static grub_off_t skip, leng;
static char *part;

static void
read_file (char *pathname, int (*hook) (grub_off_t ofs, char *buf, int len))
{
  static char buf[BUF_SIZE];
  grub_file_t file;
  grub_off_t ofs, len;

  file = grub_file_open (pathname);
  if (!file)
    {
      grub_util_error ("cannot open file %s.\n", pathname);
      return;
    }

  if (skip > file->size)
    {
      grub_util_error ("invalid skip value %d.\n");
      return;
    }

  ofs = skip;
  len = file->size - skip;
  if ((leng) && (leng < len))
    len = leng;

  file->offset = skip;

  while (len)
    {
      grub_ssize_t sz;

      sz = grub_file_read (file, buf, (len > BUF_SIZE) ? BUF_SIZE : len);
      if (sz < 0)
	{
	  grub_util_error ("read error at offset %llu.\n", ofs);
	  break;
	}

      if ((sz == 0) || (hook (ofs, buf, sz)))
	break;

      ofs += sz;
      len -= sz;
    }

  grub_file_close (file);
}

static void
cmd_cp (char *src, char *dest)
{
  FILE *ff;

  auto int cp_hook (grub_off_t ofs, char *buf, int len);
  int cp_hook (grub_off_t ofs, char *buf, int len)
  {
    (void) ofs;

    if ((int) fwrite (buf, 1, len, ff) != len)
      {
	grub_util_error ("write error.\n");
	return 1;
      }

    return 0;
  }

  ff = fopen (dest, "w");
  if (ff == NULL)
    {
      grub_util_error ("open error.\n");
      return;
    }
  read_file (src, cp_hook);
  fclose (ff);
}

static void
cmd_cmp (char *src, char *dest)
{
  FILE *ff;
  static char buf_1[BUF_SIZE];

  auto int cmp_hook (grub_off_t ofs, char *buf, int len);
  int cmp_hook (grub_off_t ofs, char *buf, int len)
  {
    if ((int) fread (buf_1, 1, len, ff) != len)
      {
	grub_util_error ("read error at offset %llu.\n", ofs);
	return 1;
      }

    if (grub_memcmp (buf, buf_1, len))
      {
	int i;

	for (i = 0; i < len; i++, ofs++)
	  if (buf_1[i] != buf[i])
	    {
	      grub_util_error ("compare fail at offset %llu.\n", ofs);
	      return 1;
	    }
      }
    return 0;
  }

  ff = fopen (dest, "r");
  if (ff == NULL)
    {
      grub_util_error ("open error.\n");
      return;
    }

  if ((skip) && (fseek (ff, skip, SEEK_SET)))
    grub_util_error ("fseek error.\n");

  read_file (src, cmp_hook);
  fclose (ff);
}

static void
cmd_hex (char *pathname)
{
  auto int hex_hook (grub_off_t ofs, char *buf, int len);
  int hex_hook (grub_off_t ofs, char *buf, int len)
  {
    hexdump (ofs, buf, len);
    return 0;
  }

  read_file (pathname, hex_hook);
}

static void
fstest (char *image_path, int cmd, int n, char **args)
{
  char host_file[7 + grub_strlen (image_path) + 1];
  char device_name[(part) ? (6 + grub_strlen (part)) : 5];
  char *argv[3] = { "-p", "loop", host_file };


  grub_sprintf (host_file, "(host)/%s", image_path);

  if (execute_command (&cmd_loopback, 3, argv))
    {
      grub_util_error ("loopback command fails.\n");
      goto fail;
    }

  if (part)
    grub_sprintf (device_name, "loop,%s", part);
  else
    grub_strcpy (device_name, "loop");

  grub_env_set ("root", device_name);

  switch (cmd)
    {
    case CMD_LS:
      execute_command (&cmd_ls, n, args);
      break;
    case CMD_CP:
      cmd_cp (args[0], args[1]);
      break;
    case CMD_CMP:
      cmd_cmp (args[0], args[1]);
      break;
    case CMD_HEX:
      cmd_hex (args[0]);
      break;
    case CMD_BLOCKLIST:
      execute_command (&cmd_blocklist, n, args);
      grub_printf ("\n");
    }

fail:

  argv[0] = "-d";

  execute_command (&cmd_loopback, 2, argv);
}

static struct option options[] = {
  {"part", required_argument, 0, 'p'},
  {"skip", required_argument, 0, 's'},
  {"length", required_argument, 0, 'n'},
  {"debug", required_argument, 0, 'd'},
  {"raw", no_argument, 0, 'r'},
  {"long", no_argument, 0, 'l'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"verbose", no_argument, 0, 'v'},

  {0, 0, 0, 0}
};

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``grub-fstest --help'' for more information.\n");
  else
    printf ("\
Usage: grub-fstest [OPTION]... IMAGE_PATH COMMANDS\n\
\n\
Debug tool for filesystem driver.\n\
\nCommands:\n\
  ls PATH                   list files in PATH\n\
  cp SRC DEST               copy file to local system\n\
  cmp SRC DEST              compare files\n\
  hex FILE                  hex dump FILE\n\
  blocklist FILE            display blocklist of FILE\n\
\nOptions:\n\
  -p, --part=NUM            select partition NUM\n\
  -s, --skip=N              skip N bytes from output file\n\
  -n, --length=N            handle N bytes in output file\n\
  -d, --debug=S             Set debug environment variable\n\
  -r, --raw                 disable auto decompression\n\
  -l, --long                show long directory list\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n", PACKAGE_BUGREPORT);

  exit (status);
}

int
main (int argc, char *argv[])
{
  char *image_path, *debug_str = 0;
  int cmd, is_raw = 0, is_long = 0;

  progname = "grub-fstest";

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "p:s:n:d:rlhVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'p':
	    part = optarg;
	    break;

	  case 's':
	    skip = grub_strtoul (optarg, NULL, 0);
	    break;

	  case 'n':
	    leng = grub_strtoul (optarg, NULL, 0);
	    break;

          case 'd':
            debug_str = optarg;
            break;

	  case 'r':
	    is_raw = 1;
	    break;

	  case 'l':
	    is_long = 1;
	    break;

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
      fprintf (stderr, "No path is specified.\n");
      usage (1);
    }

  image_path = argv[optind];

  if (*image_path != '/')
    {
      fprintf (stderr, "Must use absolute path.\n");
      usage (1);
    }

  optind++;

  cmd = 0;
  if (optind < argc)
    {
      int nparm = 1;

      if (!grub_strcmp (argv[optind], "ls"))
	{
	  cmd = CMD_LS;
	  if (is_long)
	    argv[optind--] = "-l";
	  else
	    nparm = 0;
	}
      else if (!grub_strcmp (argv[optind], "cp"))
	{
	  cmd = CMD_CP;
	  nparm = 2;
	}
      else if (!grub_strcmp (argv[optind], "cmp"))
	{
	  cmd = CMD_CMP;
	  nparm = 2;
	}
      else if (!grub_strcmp (argv[optind], "hex"))
	{
	  cmd = CMD_HEX;
	}
      else if (!grub_strcmp (argv[optind], "blocklist"))
	{
	  cmd = CMD_BLOCKLIST;
	}
      else
	{
	  fprintf (stderr, "Invalid command %s.\n", argv[optind]);
	  usage (1);
	}

      if (optind + 1 + nparm > argc)
	{
	  fprintf (stderr, "Invalid parameter for command %s.\n",
		   argv[optind]);
	  usage (1);
	}

      optind++;
    }
  else
    {
      fprintf (stderr, "No command is specified.\n");
      usage (1);
    }

  grub_hostfs_init ();

  /* Initialize all modules. */
  grub_init_all ();

  if (is_raw)
    grub_env_set ("filehook", "0");

  if (debug_str)
    grub_env_set ("debug", debug_str);

  /* Do it.  */
  fstest (image_path + 1, cmd, argc - optind, argv + optind);

  /* Free resources.  */
  grub_fini_all ();

  grub_hostfs_fini ();

  return 0;
}
