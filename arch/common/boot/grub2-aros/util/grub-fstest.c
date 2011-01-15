/* grub-fstest.c - debug tool for filesystem driver */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009,2010 Free Software Foundation, Inc.
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
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/misc.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/fs.h>
#include <grub/env.h>
#include <grub/term.h>
#include <grub/mm.h>
#include <grub/lib/hexdump.h>
#include <grub/crypto.h>
#include <grub/command.h>
#include <grub/i18n.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "progname.h"
#include "argp.h"

static grub_err_t
execute_command (char *name, int n, char **args)
{
  grub_command_t cmd;

  cmd = grub_command_find (name);
  if (! cmd)
    grub_util_error (_("can\'t find command %s"), name);

  return (cmd->func) (cmd, n, args);
}

#define CMD_LS          1
#define CMD_CP          2
#define CMD_CMP         3
#define CMD_HEX         4
#define CMD_CRC         6
#define CMD_BLOCKLIST   7

#define BUF_SIZE  32256

static grub_disk_addr_t skip, leng;

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
        grub_util_error (_("can\'t open device"));

      grub_util_info ("total sectors : %lld",
                      (unsigned long long) dev->disk->total_sectors);

      if (! leng)
        leng = (dev->disk->total_sectors << GRUB_DISK_SECTOR_BITS) - skip;

      while (leng)
        {
          grub_size_t len;

          len = (leng > BUF_SIZE) ? BUF_SIZE : leng;

          if (grub_disk_read (dev->disk, 0, skip, len, buf))
            grub_util_error (_("disk read fails at offset %lld, length %d"),
                             skip, len);

          if (hook (skip, buf, len))
            break;

          skip += len;
          leng -= len;
        }

      grub_device_close (dev);
      return;
    }

  grub_file_filter_disable_compression ();
  file = grub_file_open (pathname);
  if (!file)
    {
      grub_util_error (_("cannot open file %s"), pathname);
      return;
    }

  grub_util_info ("file size : %lld", (unsigned long long) file->size);

  if (skip > file->size)
    {
      grub_util_error (_("invalid skip value %lld"), (unsigned long long) skip);
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
	  grub_util_error (_("read error at offset %llu: %s"), ofs,
			   grub_errmsg);
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
	grub_util_error (_("write error"));
	return 1;
      }

    return 0;
  }

  ff = fopen (dest, "wb");
  if (ff == NULL)
    {
      grub_util_error (_("open error"));
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
	grub_util_error (_("read error at offset %llu: %s"), ofs, grub_errmsg);
	return 1;
      }

    if (grub_memcmp (buf, buf_1, len))
      {
	int i;

	for (i = 0; i < len; i++, ofs++)
	  if (buf_1[i] != buf[i])
	    {
	      grub_util_error (_("compare fail at offset %llu"), ofs);
	      return 1;
	    }
      }
    return 0;
  }

  ff = fopen (dest, "rb");
  if (ff == NULL)
    {
      grub_util_error (_("open error"));
      return;
    }

  if ((skip) && (fseeko (ff, skip, SEEK_SET)))
    grub_util_error (_("seek error"));

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
  grub_uint8_t crc32_context[GRUB_MD_CRC32->contextsize];
  GRUB_MD_CRC32->init(crc32_context);

  auto int crc_hook (grub_off_t ofs, char *buf, int len);
  int crc_hook (grub_off_t ofs, char *buf, int len)
  {
    (void) ofs;

    GRUB_MD_CRC32->write(crc32_context, buf, len);
    return 0;
  }

  read_file (pathname, crc_hook);
  GRUB_MD_CRC32->final(crc32_context);
  printf ("%08x\n",
      grub_be_to_cpu32(*(grub_uint32_t*)GRUB_MD_CRC32->read(crc32_context)));
}

static char *root = NULL;
static int args_count = 0;
static int nparm = 0;
static int num_disks = 1;
static char **images = NULL;
static int cmd = 0;
static char *debug_str = NULL;
static char **args = NULL;

static void
fstest (int n, char **args)
{
  char *host_file;
  char *loop_name;
  int i;

  for (i = 0; i < num_disks; i++)
    {
      char *argv[2];
      loop_name = grub_xasprintf ("loop%d", i);
      if (!loop_name)
	grub_util_error (grub_errmsg);

      host_file = grub_xasprintf ("(host)%s", images[i]);
      if (!host_file)
	grub_util_error (grub_errmsg);

      argv[0] = loop_name;
      argv[1] = host_file;

      if (execute_command ("loopback", 2, argv))
        grub_util_error (_("loopback command fails"));

      grub_free (loop_name);
      grub_free (host_file);
    }

  grub_lvm_fini ();
  grub_mdraid09_fini ();
  grub_mdraid1x_fini ();
  grub_raid_fini ();
  grub_raid_init ();
  grub_mdraid09_init ();
  grub_mdraid1x_init ();
  grub_lvm_init ();

  switch (cmd)
    {
    case CMD_LS:
      execute_command ("ls", n, args);
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
      execute_command ("blocklist", n, args);
      grub_printf ("\n");
    }
    
  for (i = 0; i < num_disks; i++)
    {
      char *argv[2];

      loop_name = grub_xasprintf ("loop%d", i);
      if (!loop_name)
	grub_util_error (grub_errmsg);

      argv[0] = "-d";      
      argv[1] = loop_name;

      execute_command ("loopback", 2, argv);

      grub_free (loop_name);
    }
}

static struct argp_option options[] = {
  {0,          0, 0      , OPTION_DOC, N_("Commands:"), 1},
  {N_("ls PATH"),  0, 0      , OPTION_DOC, N_("List files in PATH."), 1},
  {N_("cp FILE LOCAL"),  0, 0, OPTION_DOC, N_("Copy FILE to local file LOCAL."), 1},
  {N_("cmp FILE LOCAL"), 0, 0, OPTION_DOC, N_("Compare FILE with local file LOCAL."), 1},
  {N_("hex FILE"), 0, 0      , OPTION_DOC, N_("Hex dump FILE."), 1},
  {N_("crc FILE"), 0, 0     , OPTION_DOC, N_("Get crc32 checksum of FILE."), 1},
  {N_("blocklist FILE"), 0, 0, OPTION_DOC, N_("Display blocklist of FILE."), 1},
  
  {"root",      'r', N_("DEVICE_NAME"), 0, N_("Set root device."),                 2},
  {"skip",      's', "N",           0, N_("Skip N bytes from output file."),   2},
  {"length",    'n', "N",           0, N_("Handle N bytes in output file."),   2},
  {"diskcount", 'c', "N",           0, N_("N input files."),                   2},
  {"debug",     'd', "S",           0, N_("Set debug environment variable."),  2},
  {"verbose",   'v', NULL, OPTION_ARG_OPTIONAL, N_("Print verbose messages."), 2},
  {0, 0, 0, 0, 0, 0}
};

/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state)
{
  fprintf (stream, "%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
}
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

error_t 
argp_parser (int key, char *arg, struct argp_state *state)
{
  char *p;

  switch (key)
    {
    case 'r':
      root = arg;
      return 0;

    case 's':
      skip = grub_strtoul (arg, &p, 0);
      if (*p == 's')
	skip <<= GRUB_DISK_SECTOR_BITS;
      return 0;

    case 'n':
      leng = grub_strtoul (arg, &p, 0);
      if (*p == 's')
	leng <<= GRUB_DISK_SECTOR_BITS;
      return 0;

    case 'c':
      num_disks = grub_strtoul (arg, NULL, 0);
      if (num_disks < 1)
	{
	  fprintf (stderr, "%s", _("Invalid disk count.\n"));
	  argp_usage (state);
	}
      if (args_count != 0)
	{
	  fprintf (stderr, "%s", _("Disk count must precede disks list.\n"));
	  argp_usage (state);
	}
      return 0;

    case 'd':
      debug_str = arg;
      return 0;

    case 'v':
      verbosity++;
      return 0;

    case ARGP_KEY_END:
      if (args_count < num_disks)
	{
	  fprintf (stderr, "%s", _("No command is specified.\n"));
	  argp_usage (state);
	}
      if (args_count - 1 - num_disks < nparm)
	{
	  fprintf (stderr, "%s", _("Not enough parameters to command.\n"));
	  argp_usage (state);
	}
      return 0;

    case ARGP_KEY_ARG:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  if (args_count < num_disks)
    {
      if (arg[0] != '/')
	{
	  fprintf (stderr, "%s", _("Must use absolute path.\n"));
	  argp_usage (state);
	}
      if (args_count == 0)
	images = xmalloc (num_disks * sizeof (images[0]));
      images[args_count] = xstrdup (arg);
      args_count++;
      return 0;
    }

  if (args_count == num_disks)
    {
      if (!grub_strcmp (arg, "ls"))
        {
          cmd = CMD_LS;
        }
      else if (!grub_strcmp (arg, "cp"))
	{
	  cmd = CMD_CP;
          nparm = 2;
	}
      else if (!grub_strcmp (arg, "cmp"))
	{
	  cmd = CMD_CMP;
          nparm = 2;
	}
      else if (!grub_strcmp (arg, "hex"))
	{
	  cmd = CMD_HEX;
          nparm = 1;
	}
      else if (!grub_strcmp (arg, "crc"))
	{
	  cmd = CMD_CRC;
          nparm = 1;
	}
      else if (!grub_strcmp (arg, "blocklist"))
	{
	  cmd = CMD_BLOCKLIST;
          nparm = 1;
	}
      else
	{
	  fprintf (stderr, _("Invalid command %s.\n"), arg);
	  argp_usage (state);
	}
      args_count++;
      return 0;
    }

  args[args_count - 1 - num_disks] = xstrdup (arg);
  args_count++;
  return 0;
}

struct argp argp = {
  options, argp_parser, N_("IMAGE_PATH COMMANDS"),
  N_("Debug tool for filesystem driver."), 
  NULL, NULL, NULL
};

int
main (int argc, char *argv[])
{
  char *default_root, *alloc_root;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  args = xmalloc (argc * sizeof (args[0]));

  argp_parse (&argp, argc, argv, 0, 0, 0);

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
  fstest (args_count - 1 - num_disks, args);

  /* Free resources.  */
  grub_fini_all ();

  return 0;
}
