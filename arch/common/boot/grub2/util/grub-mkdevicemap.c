/* grub-mkdevicemap.c - make a device map file automatically */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include <grub/util/misc.h>
#include <grub/util/deviceiter.h>

#define _GNU_SOURCE	1
#include <getopt.h>

static void
make_device_map (const char *device_map, int floppy_disks)
{
  int num_hd = 0;
  int num_fd = 0;
  FILE *fp;

  auto int NESTED_FUNC_ATTR process_device (const char *name, int is_floppy);

  int NESTED_FUNC_ATTR process_device (const char *name, int is_floppy)
  {
    grub_util_emit_devicemap_entry (fp, (char *) name,
				    is_floppy, &num_fd, &num_hd);
    return 0;
  }

  if (strcmp (device_map, "-") == 0)
    fp = stdout;
  else
    fp = fopen (device_map, "w");

  if (! fp)
    grub_util_error ("cannot open %s", device_map);

  grub_util_iterate_devices (process_device, floppy_disks);

  if (fp != stdout)
    fclose (fp);
}

static struct option options[] =
  {
    {"device-map", required_argument, 0, 'm'},
    {"probe-second-floppy", no_argument, 0, 's'},
    {"no-floppy", no_argument, 0, 'n'},
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
	     "Try ``grub-mkdevicemap --help'' for more information.\n");
  else
    printf ("\
Usage: grub-mkdevicemap [OPTION]...\n\
\n\
Generate a device map file automatically.\n\
\n\
  -n, --no-floppy           do not probe any floppy drive\n\
  -s, --probe-second-floppy probe the second floppy drive\n\
  -m, --device-map=FILE     use FILE as the device map [default=%s]\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n\
",
	    DEFAULT_DEVICE_MAP, PACKAGE_BUGREPORT);

  exit (status);
}

int
main (int argc, char *argv[])
{
  char *dev_map = 0;
  int floppy_disks = 1;

  progname = "grub-mkdevicemap";

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "snm:r:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'm':
	    if (dev_map)
	      free (dev_map);

	    dev_map = xstrdup (optarg);
	    break;

	  case 'n':
	    floppy_disks = 0;
	    break;

	  case 's':
	    floppy_disks = 2;
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

  make_device_map (dev_map ? : DEFAULT_DEVICE_MAP, floppy_disks);

  free (dev_map);

  return 0;
}
