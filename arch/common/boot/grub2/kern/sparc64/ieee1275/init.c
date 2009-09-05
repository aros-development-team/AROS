/*  init.c -- Initialize GRUB on SPARC64.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/time.h>
#include <grub/machine/console.h>
#include <grub/machine/kernel.h>
#include <grub/machine/time.h>
#include <grub/ieee1275/ofdisk.h>
#include <grub/ieee1275/ieee1275.h>

void
grub_exit (void)
{
  grub_ieee1275_exit ();
}

static grub_uint64_t
ieee1275_get_time_ms (void)
{
  grub_uint32_t msecs = 0;

  grub_ieee1275_milliseconds (&msecs);

  return msecs;
}

grub_uint32_t
grub_get_rtc (void)
{
  return ieee1275_get_time_ms ();
}

grub_addr_t
grub_arch_modules_addr (void)
{
  extern char _end[];
  return (grub_addr_t) _end;
}

void
grub_machine_set_prefix (void)
{
  if (grub_prefix[0] != '(')
    {
      char bootpath[IEEE1275_MAX_PATH_LEN];
      char *prefix, *path, *colon;
      grub_ssize_t actual;

      if (grub_ieee1275_get_property (grub_ieee1275_chosen, "bootpath",
				      &bootpath, sizeof (bootpath), &actual))
	{
	  /* Should never happen.  */
	  grub_printf ("/chosen/bootpath property missing!\n");
	  grub_env_set ("prefix", "");
	  return;
	}

      /* Transform an OF device path to a GRUB path.  */
      colon = grub_strchr (bootpath, ':');
      if (colon)
	{
	  char *part = colon + 1;

	  /* Consistently provide numbered partitions to GRUB.
	     OpenBOOT traditionally uses alphabetical partition
	     specifiers.  */
	  if (part[0] >= 'a' && part[0] <= 'z')
	    part[0] = '1' + (part[0] - 'a');
	}
      prefix = grub_ieee1275_encode_devname (bootpath);

      path = grub_malloc (grub_strlen (grub_prefix)
			  + grub_strlen (prefix)
			  + 2);
      grub_sprintf(path, "%s%s", prefix, grub_prefix);

      grub_strcpy (grub_prefix, path);

      grub_free (path);
      grub_free (prefix);
    }

  grub_env_set ("prefix", grub_prefix);
}

static void
grub_heap_init (void)
{
  grub_mm_init_region ((void *)(long)0x4000UL, 0x200000 - 0x4000);
}

static void
grub_parse_cmdline (void)
{
  grub_ssize_t actual;
  char args[256];

  if (grub_ieee1275_get_property (grub_ieee1275_chosen, "bootargs", &args,
				  sizeof args, &actual) == 0
      && actual > 1)
    {
      int i = 0;

      while (i < actual)
	{
	  char *command = &args[i];
	  char *end;
	  char *val;

	  end = grub_strchr (command, ';');
	  if (end == 0)
	    i = actual; /* No more commands after this one.  */
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
grub_machine_init (void)
{
  grub_ieee1275_init ();
  grub_console_init ();
  grub_heap_init ();

  grub_ieee1275_set_flag (GRUB_IEEE1275_FLAG_NO_PARTITION_0);
  grub_ofdisk_init ();

  grub_parse_cmdline ();
  grub_install_get_time_ms (ieee1275_get_time_ms);
}

void
grub_machine_fini (void)
{
  grub_ofdisk_fini ();
  grub_console_fini ();
}
