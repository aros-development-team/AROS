/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007  Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <sys/stat.h>
#include <argp.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <grub/mm.h>
#include <grub/setjmp.h>
#include <grub/fs.h>
#include <grub/util/biosdisk.h>
#include <grub/dl.h>
#include <grub/machine/console.h>
#include <grub/util/misc.h>
#include <grub/kernel.h>
#include <grub/normal.h>
#include <grub/util/getroot.h>
#include <grub/env.h>
#include <grub/partition.h>

#include <grub_emu_init.h>

/* Used for going back to the main function.  */
jmp_buf main_env;

/* Store the prefix specified by an argument.  */
static char *prefix = 0;

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
grub_machine_set_prefix (void)
{
  grub_env_set ("prefix", prefix);
  free (prefix);
  prefix = 0;
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
  char *dir;
  
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
  
  /* XXX: This is a bit unportable.  */
  grub_util_biosdisk_init (args.dev_map);

  grub_hostfs_init ();

  grub_init_all ();

  /* Make sure that there is a root device.  */
  if (! args.root_dev)
    {
      char *device_name = grub_guess_root_device (args.dir ? : DEFAULT_DIRECTORY);
      if (! device_name)
        grub_util_error ("cannot find a device for %s.\n", args.dir ? : DEFAULT_DIRECTORY);

      args.root_dev = grub_util_get_grub_dev (device_name);
      if (! args.root_dev)
	{
	  grub_util_info ("guessing the root device failed, because of `%s'",
			  grub_errmsg);
	  grub_util_error ("Cannot guess the root device. Specify the option ``--root-device''.");
	}
    }

  dir = grub_get_prefix (args.dir ? : DEFAULT_DIRECTORY);
  prefix = xmalloc (strlen (args.root_dev) + strlen (dir) + 1);
  sprintf (prefix, "%s%s", args.root_dev, dir);
  free (dir);
  
  /* Start GRUB!  */
  if (setjmp (main_env) == 0)
    grub_main ();

  grub_fini_all ();

  grub_hostfs_fini ();

  grub_machine_fini ();
  
  return 0;
}
