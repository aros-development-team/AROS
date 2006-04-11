/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005  Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <argp.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <grub/mm.h>
#include <grub/setjmp.h>
#include <grub/fs.h>
#include <grub/i386/pc/util/biosdisk.h>
#include <grub/dl.h>
#include <grub/machine/console.h>
#include <grub/util/misc.h>
#include <grub/kernel.h>
#include <grub/normal.h>
#include <grub/util/getroot.h>
#include <grub/env.h>
#include <grub/partition.h>

#ifdef __NetBSD__
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/grub"
#else
# define DEFAULT_DIRECTORY	"/boot/grub"
#endif

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

/* Used for going back to the main function.  */
jmp_buf main_env;

grub_addr_t
grub_arch_modules_addr (void)
{
  return 0;
}

grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  (void) ehdr;

  return GRUB_ERR_BAD_MODULE;
}

grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  (void) mod;
  (void) ehdr;

  return GRUB_ERR_BAD_MODULE;
}

void
grub_machine_init (void)
{
  signal (SIGINT, SIG_IGN);
  grub_console_init ();
}

void
grub_machine_fini (void)
{
  grub_console_fini ();
}


const char *argp_program_version = PACKAGE_STRING;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;
static char doc[] = "GRUB emulator";

static struct argp_option options[] = {
  {"root-device", 'r', "DEV",  0, "use DEV as the root device [default=guessed]", 0},
  {"device-map",  'm', "FILE", 0, "use FILE as the device map", 0},
  {"directory",   'd', "DIR",  0, "use GRUB files in the directory DIR", 0},
  {"verbose",     'v', 0     , 0, "print verbose messages", 0},
  {"hold",        'H', "SECONDS", OPTION_ARG_OPTIONAL, "wait until a debugger will attach", 0},
  { 0, 0, 0, 0, 0, 0 }
};

struct arguments
{
  char *root_dev;
  char *dev_map;
  char *dir;
  int hold;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *args = state->input;
  
  switch (key)
    {
    case 'r':
      args->root_dev = arg;
      break;
    case 'd':
      args->dir = arg;
      break;
    case 'm':
      args->dev_map = arg;
      break;
    case 'v':
      verbosity++;
      break;
    case 'H':
      args->hold = arg ? atoi (arg) : -1;
      break;
    case ARGP_KEY_END:
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {options, parse_opt, 0, doc, 0, 0, 0};


int
main (int argc, char *argv[])
{
  char *prefix = 0;
  char rootprefix[100];
  struct arguments args =
    {
      .dir = DEFAULT_DIRECTORY,
      .dev_map = DEFAULT_DEVICE_MAP,
      .hold = 0
    };
  
  progname = "grub-emu";
  
  argp_parse (&argp, argc, argv, 0, 0, &args);

  /* Wait until the ARGS.HOLD variable is cleared by an attached debugger. */
  if (args.hold && verbosity > 0)
    printf ("Run \"gdb %s %d\", and set ARGS.HOLD to zero.\n",
            progname, (int) getpid ());
  while (args.hold)
    {
      if (args.hold > 0)
        args.hold--;

      sleep (1);
    }
  
  /* Make sure that there is a root device.  */
  if (! args.root_dev)
    {
      args.root_dev = grub_guess_root_device (args.dir ? : DEFAULT_DIRECTORY);
      if (! args.root_dev)
	{
	  grub_util_info ("guessing the root device failed, because of `%s'",
			  grub_errmsg);
	  grub_util_error ("Cannot guess the root device. Specify the option ``--root-device''.");
	}
    }

  prefix = grub_get_prefix (args.dir ? : DEFAULT_DIRECTORY);
  sprintf (rootprefix, "%s%s", args.root_dev, prefix);

  grub_env_set ("prefix", rootprefix);
  
  /* XXX: This is a bit unportable.  */
  grub_util_biosdisk_init (args.dev_map);
  grub_pc_partition_map_init ();
  grub_amiga_partition_map_init ();
  grub_apple_partition_map_init ();
  grub_sun_partition_map_init ();

  /* Initialize the default modules.  */
  grub_iso9660_init ();
  grub_xfs_init ();
  grub_fat_init ();
  grub_ext2_init ();
  grub_ufs_init ();
  grub_minix_init ();
  grub_hfs_init ();
  grub_jfs_init ();
  grub_xfs_init ();
  grub_sfs_init ();
  grub_affs_init ();
  grub_ls_init ();
  grub_boot_init ();
  grub_cmp_init ();
  grub_cat_init ();
  grub_terminal_init ();
  grub_loop_init ();
  grub_help_init ();
  grub_halt_init ();
  grub_reboot_init ();
  grub_default_init ();
  grub_timeout_init ();
  grub_configfile_init ();
  grub_search_init ();
  
  /* XXX: Should normal mode be started by default?  */
  grub_normal_init ();

  /* Start GRUB!  */
  if (setjmp (main_env) == 0)
    grub_main ();

  grub_search_fini ();
  grub_configfile_fini ();
  grub_timeout_fini ();
  grub_default_fini ();
  grub_reboot_fini ();
  grub_halt_fini ();
  grub_help_fini ();
  grub_loop_fini ();
  grub_util_biosdisk_fini ();
  grub_normal_fini ();
  grub_affs_fini ();
  grub_sfs_fini ();
  grub_xfs_fini ();
  grub_ufs_fini ();
  grub_ext2_fini ();
  grub_minix_fini ();
  grub_hfs_fini ();
  grub_jfs_fini ();
  grub_fat_fini ();
  grub_xfs_fini ();
  grub_boot_fini ();
  grub_cmp_fini ();
  grub_cat_fini ();
  grub_terminal_fini ();
  grub_amiga_partition_map_fini ();
  grub_pc_partition_map_fini ();
  grub_apple_partition_map_fini ();
  grub_sun_partition_map_fini ();

  grub_machine_fini ();
  
  return 0;
}
