/*  init.c -- Initialize GRUB on the Ultra Sprac (sparc64).  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/kernel.h>
#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/normal.h>
#include <grub/fs.h>
#include <grub/setjmp.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/machine/console.h>
#include <grub/machine/time.h>
#include <grub/machine/kernel.h>
#include <grub/ieee1275/ofdisk.h>
#include <grub/ieee1275/ieee1275.h>

/* OpenBoot entry point.  */
int (*grub_ieee1275_entry_fn) (void *);
grub_ieee1275_phandle_t grub_ieee1275_chosen;
static grub_uint32_t grub_ieee1275_flags;
/* FIXME (sparc64).  */
static const grub_addr_t grub_heap_start = 0x40000;
static grub_addr_t grub_heap_len;

void
_start (uint64_t r0 __attribute__((unused)),
        uint64_t r1 __attribute__((unused)),
        uint64_t r2 __attribute__((unused)),
        uint64_t r3 __attribute__((unused)),
        uint64_t r4,
        uint64_t r5 __attribute__((unused)));
void
_start (uint64_t r0 __attribute__((unused)),
        uint64_t r1 __attribute__((unused)),
        uint64_t r2 __attribute__((unused)),
        uint64_t r3 __attribute__((unused)),
        uint64_t r4,
        uint64_t r5 __attribute__((unused)))
{
  grub_ieee1275_entry_fn = (int (*)(void *)) r4;

  grub_ieee1275_finddevice ("/chosen", &grub_ieee1275_chosen);

  /* Now invoke the main function.  */
  grub_main ();

  /* Never reached.  */
}

int
grub_ieee1275_test_flag (enum grub_ieee1275_flag flag)
{
  return (grub_ieee1275_flags & (1 << flag));
}

void
grub_ieee1275_set_flag (enum grub_ieee1275_flag flag)
{
  grub_ieee1275_flags |= (1 << flag);
}

void
abort (void)
{
  /* Trap to Open Firmware.  */
  grub_ieee1275_enter ();
}

/* Translate an OF filesystem path (separated by backslashes), into a GRUB
   path (separated by forward slashes).  */
static void
grub_translate_ieee1275_path (char *filepath)
{
  char *backslash;

  backslash = grub_strchr (filepath, '\\');
  while (backslash != 0)
    {
      *backslash = '/';
      backslash = grub_strchr (filepath, '\\');
    }
}

static void
grub_set_prefix (void)
{
  char bootpath[64]; /* XXX check length */
  char *filename;
  char *prefix;

  if (grub_ieee1275_get_property (grub_ieee1275_chosen, "bootpath", bootpath,
				  sizeof (bootpath), 0))
    {
      /* Should never happen.  */
      grub_printf ("/chosen/bootpath property missing!\n");
      grub_env_set ("prefix", "");
      return;
    }

  /* Transform an OF device path to a GRUB path.  */

  prefix = grub_ieee1275_encode_devname (bootpath);

  filename = grub_ieee1275_get_filename (bootpath);
  if (filename)
    {
      char *newprefix;
      char *lastslash = grub_strrchr (filename, '\\');

      /* Truncate at last directory.  */
      if (lastslash)
        {
	  *lastslash = '\0';
	  grub_translate_ieee1275_path (filename);

	  newprefix = grub_malloc (grub_strlen (prefix)
				   + grub_strlen (filename));
	  grub_sprintf (newprefix, "%s%s", prefix, filename);
	  grub_free (prefix);
	  prefix = newprefix;
	}
    }

  grub_env_set ("prefix", prefix);

  grub_free (filename);
  grub_free (prefix);
}

void
grub_machine_init (void)
{
  char *args;
  grub_ssize_t length;

  grub_console_init ();

  /* FIXME (sparc64).  */
  grub_heap_len = (grub_addr_t) &_start - 0x1000 - grub_heap_start;

  if (grub_ieee1275_claim (grub_heap_start, grub_heap_len, 0, 0))
      grub_fatal ("Failed to claim heap at %p, len 0x%x\n", grub_heap_start,
		   grub_heap_len);
  grub_mm_init_region ((void *) grub_heap_start, grub_heap_len);

  grub_set_prefix ();

  grub_ofdisk_init ();

  /* Process commandline.  */
  if (grub_ieee1275_get_property_length (grub_ieee1275_chosen, "bootargs",
                                         &length) == 0 &&
      length > 0)
    {
      grub_ssize_t i = 0;

      args = grub_malloc (length);
      grub_ieee1275_get_property (grub_ieee1275_chosen, "bootargs", args,
				  length, 0);

      while (i < length)
	{
	  char *command = &args[i];
	  char *end;
	  char *val;

	  end = grub_strchr (command, ';');
	  if (end == 0)
	    i = length; /* No more commands after this one.  */
	  else
	    {
	      *end = '\0';
	      i += end - command + 1;
	      while (grub_isspace(args[i]))
		i++;
	    }

	  /* Process command.  */
	  val = grub_strchr (command, '=');
	  if (val)
	    {
	      *val = '\0';
	      grub_env_set (command, val + 1);
	    }
	}
    }

}

void
grub_machine_fini (void)
{
  grub_ofdisk_fini ();
  grub_console_fini ();
}

void
grub_stop (void)
{
  grub_ieee1275_exit ();
}

grub_uint32_t
grub_get_rtc (void)
{
  grub_uint32_t msecs;

  if (grub_ieee1275_milliseconds (&msecs))
    return 0;

  return msecs;
}

grub_addr_t
grub_arch_modules_addr (void)
{
  return GRUB_IEEE1275_MODULE_BASE;
}
