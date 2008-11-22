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
#include <grub/raid.h>
#include <grub/lib/hexdump.h>
#include <grub/lib/crc.h>

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

grub_term_input_t
grub_term_get_current_input (void)
{
  return 0;
}

grub_term_output_t
grub_term_get_current_output (void)
{
  return 0;
}

void
grub_refresh (void)
{
  fflush (stdout);
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

static grub_err_t
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
#define CMD_CRC         6
#define CMD_BLOCKLIST   7

#define BUF_SIZE  32256

static grub_off_t skip, leng;

static void
read_file (char *pathname, int (*hook) (grub_off_t ofs, char *buf, int len))
{
  static char buf[BUF_SIZE];
  grub_file_t file;
  grub_off_t ofs, len;

  if ((pathname[0] == '-') && (pathname[1] == 0))
    {
      grub_device_t dev;

      dev = grub_device_open (0);
      if ((! dev) || (! dev->disk))
        grub_util_error ("Can\'t open device.");

      grub_util_info ("total sectors : %lld.",
                      (unsigned long long) dev->disk->total_sectors);

      if (! leng)
        leng = (dev->disk->total_sectors << GRUB_DISK_SECTOR_BITS) - skip;

      while (leng)
        {
          grub_size_t len;

          len = (leng > BUF_SIZE) ? BUF_SIZE : leng;

          if (grub_disk_read (dev->disk, 0, skip, len, buf))
            grub_util_error ("Disk read fails at offset %lld, length %d.",
                             skip, len);

          if (hook (skip, buf, len))
            break;

          skip += len;
          leng -= len;
        }

      grub_device_close (dev);
      return;
    }

  file = grub_file_open (pathname);
  if (!file)
    {
      grub_util_error ("cannot open file %s.", pathname);
      return;
    }

  grub_util_info ("file size : %lld.", (unsigned long long) file->size);

  if (skip > file->size)
    {
      grub_util_error ("invalid skip value %d.");
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
	  grub_util_error ("read error at offset %llu.", ofs);
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
	grub_util_error ("write error.");
	return 1;
      }

    return 0;
  }

  ff = fopen (dest, "wb");
  if (ff == NULL)
    {
      grub_util_error ("open error.");
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
	grub_util_error ("read error at offset %llu.", ofs);
	return 1;
      }

    if (grub_memcmp (buf, buf_1, len))
      {
	int i;

	for (i = 0; i < len; i++, ofs++)
	  if (buf_1[i] != buf[i])
	    {
	      grub_util_error ("compare fail at offset %llu.", ofs);
	      return 1;
	    }
      }
    return 0;
  }

  ff = fopen (dest, "rb");
  if (ff == NULL)
    {
      grub_util_error ("open error.");
      return;
    }

  if ((skip) && (fseeko (ff, skip, SEEK_SET)))
    grub_util_error ("seek error.");

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
cmd_crc (char *pathname)
{
  grub_uint32_t crc = 0;

  auto int crc_hook (grub_off_t ofs, char *buf, int len);
  int crc_hook (grub_off_t ofs, char *buf, int len)
  {
    (void) ofs;

    crc = grub_getcrc32 (crc, buf, len);
    return 0;
  }

  read_file (pathname, crc_hook);
  printf ("%08x\n", crc);
}

static void
fstest (char **images, int num_disks, int cmd, int n, char **args)
{
  char host_file[128];
  char loop_name[8];
  char *argv[3] = { "-p", loop_name, host_file};
  int i;

  for (i = 0; i < num_disks; i++)
    {
      if (grub_strlen (images[i]) + 7 > sizeof (host_file))
        grub_util_error ("Pathname %s too long.", images[i]);

      grub_sprintf (loop_name, "loop%d", i);
      grub_sprintf (host_file, "(host)%s", images[i]);

      if (execute_command (&cmd_loopback, 3, argv))
        grub_util_error ("loopback command fails.");
    }

  grub_raid_rescan ();
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
    case CMD_CRC:
      cmd_crc (args[0]);
      break;
    case CMD_BLOCKLIST:
      execute_command (&cmd_blocklist, n, args);
      grub_printf ("\n");
    }

  argv[0] = "-d";

  for (i = 0; i < num_disks; i++)
    {
      grub_sprintf (loop_name, "loop%d", i);
      execute_command (&cmd_loopback, 2, argv);
    }
}

static struct option options[] = {
  {"root", required_argument, 0, 'r'},
  {"skip", required_argument, 0, 's'},
  {"length", required_argument, 0, 'n'},
  {"diskcount", required_argument, 0, 'c'},
  {"debug", required_argument, 0, 'd'},
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
  cp FILE LOCAL             copy FILE to local file LOCAL\n\
  cmp FILE LOCAL            compare FILE with local file LOCAL\n\
  hex FILE                  Hex dump FILE\n\
  crc FILE                  Get crc32 checksum of FILE\n\
  blocklist FILE            display blocklist of FILE\n\
\nOptions:\n\
  -r, --root=DEVICE_NAME    set root device\n\
  -s, --skip=N              skip N bytes from output file\n\
  -n, --length=N            handle N bytes in output file\n\
  -c, --diskcount=N         N input files\n\
  -d, --debug=S             Set debug environment variable\n\
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
  char *debug_str = 0, *root = 0, *default_root, *alloc_root;
  int i, cmd, num_opts, image_index, num_disks = 1;

  progname = "grub-fstest";

  /* Find the first non option entry.  */
  for (num_opts = 1; num_opts < argc; num_opts++)
    if (argv[num_opts][0] == '-')
      {
        if ((argv[num_opts][2] == 0) && (num_opts < argc - 1) &&
            ((argv[num_opts][1] == 'r') ||
             (argv[num_opts][1] == 's') ||
             (argv[num_opts][1] == 'n') ||
             (argv[num_opts][1] == 'c') ||
             (argv[num_opts][1] == 'd')))
            num_opts++;
      }
    else
      break;

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (num_opts, argv, "r:s:n:c:d:hVv", options, 0);
      char *p;

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'r':
	    root = optarg;
	    break;

	  case 's':
	    skip = grub_strtoul (optarg, &p, 0);
            if (*p == 's')
              skip <<= GRUB_DISK_SECTOR_BITS;
	    break;

	  case 'n':
	    leng = grub_strtoul (optarg, &p, 0);
            if (*p == 's')
              leng <<= GRUB_DISK_SECTOR_BITS;
	    break;

          case 'c':
            num_disks = grub_strtoul (optarg, NULL, 0);
            if (num_disks < 1)
              {
                fprintf (stderr, "Invalid disk count.\n");
                usage (1);
              }
            break;

          case 'd':
            debug_str = optarg;
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
  if (optind + num_disks - 1 >= argc)
    {
      fprintf (stderr, "Not enough pathname.\n");
      usage (1);
    }

  image_index = optind;
  for (i = 0; i < num_disks; i++, optind++)
    if (argv[optind][0] != '/')
      {
        fprintf (stderr, "Must use absolute path.\n");
        usage (1);
      }

  cmd = 0;
  if (optind < argc)
    {
      int nparm = 0;

      if (!grub_strcmp (argv[optind], "ls"))
        {
          cmd = CMD_LS;
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
          nparm = 1;
	}
      else if (!grub_strcmp (argv[optind], "crc"))
	{
	  cmd = CMD_CRC;
          nparm = 1;
	}
      else if (!grub_strcmp (argv[optind], "blocklist"))
	{
	  cmd = CMD_BLOCKLIST;
          nparm = 1;
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

  /* Initialize all modules. */
  grub_init_all ();

  if (debug_str)
    grub_env_set ("debug", debug_str);

  default_root = (num_disks == 1) ? "loop0" : "md0";
  alloc_root = 0;
  if (root)
    {
      if ((*root >= '0') && (*root <= '9'))
        {
          alloc_root = xmalloc (strlen (default_root) + strlen (root) + 2);

          sprintf (alloc_root, "%s,%s", default_root, root);
          root = alloc_root;
        }
    }
  else
    root = default_root;

  grub_env_set ("root", root);

  if (alloc_root)
    free (alloc_root);

  /* Do it.  */
  fstest (argv + image_index, num_disks, cmd, argc - optind, argv + optind);

  /* Free resources.  */
  grub_fini_all ();

  return 0;
}
