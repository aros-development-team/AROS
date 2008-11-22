/*  init.c -- Initialize GRUB on the newworld mac (PPC).  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007,2008 Free Software Foundation, Inc.
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
#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/normal.h>
#include <grub/fs.h>
#include <grub/setjmp.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/time.h>
#include <grub/machine/console.h>
#include <grub/machine/kernel.h>
#include <grub/cpu/kernel.h>
#include <grub/ieee1275/ofdisk.h>
#include <grub/ieee1275/ieee1275.h>

/* The minimal heap size we can live with. */
#define HEAP_MIN_SIZE		(unsigned long) (2 * 1024 * 1024)

/* The maximum heap size we're going to claim */
#define HEAP_MAX_SIZE		(unsigned long) (4 * 1024 * 1024)

/* If possible, we will avoid claiming heap above this address, because it
   seems to cause relocation problems with OSes that link at 4 MiB */
#define HEAP_MAX_ADDR		(unsigned long) (4 * 1024 * 1024)

extern char _start[];
extern char _end[];

void
grub_exit (void)
{
  grub_ieee1275_exit ();
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

void
grub_machine_set_prefix (void)
{
  char bootpath[64]; /* XXX check length */
  char *filename;
  char *prefix;

  if (grub_env_get ("prefix"))
    /* We already set prefix in grub_machine_init().  */
    return;

  if (grub_prefix[0])
    {
      grub_env_set ("prefix", grub_prefix);
      /* Prefix is hardcoded in the core image.  */
      return;
    }

  if (grub_ieee1275_get_property (grub_ieee1275_chosen, "bootpath", &bootpath,
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

/* Claim some available memory in the first /memory node. */
static void grub_claim_heap (void)
{
  unsigned long total = 0;

  auto int NESTED_FUNC_ATTR heap_init (grub_uint64_t addr, grub_uint64_t len);
  int NESTED_FUNC_ATTR heap_init (grub_uint64_t addr, grub_uint64_t len)
  {
    len -= 1; /* Required for some firmware.  */

    /* Never exceed HEAP_MAX_SIZE  */
    if (total + len > HEAP_MAX_SIZE)
      len = HEAP_MAX_SIZE - total;

    /* Avoid claiming anything above HEAP_MAX_ADDR, if possible. */
    if ((addr < HEAP_MAX_ADDR) &&				/* if it's too late, don't bother */
        (addr + len > HEAP_MAX_ADDR) &&				/* if it wasn't available anyway, don't bother */
        (total + (HEAP_MAX_ADDR - addr) > HEAP_MIN_SIZE))	/* only limit ourselves when we can afford to */
       len = HEAP_MAX_ADDR - addr;

    /* In theory, firmware should already prevent this from happening by not
       listing our own image in /memory/available.  The check below is intended
       as a safegard in case that doesn't happen.  It does, however, not protect
       us from corrupting our module area, which extends up to a
       yet-undetermined region above _end.  */
    if ((addr < (grub_addr_t) _end) && ((addr + len) > (grub_addr_t) _start))
      {
        grub_printf ("Warning: attempt to claim over our own code!\n");
        len = 0;
      }

    if (len)
      {
	/* Claim and use it.  */
	if (grub_claimmap (addr, len) < 0)
	  return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			     "Failed to claim heap at 0x%llx, len 0x%llx\n",
			     addr, len);
	grub_mm_init_region ((void *) (grub_addr_t) addr, len);
      }

    total += len;
    if (total >= HEAP_MAX_SIZE)
      return 1;

    return 0;
  }

  if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_CANNOT_INTERPRET))
    heap_init (HEAP_MAX_ADDR - HEAP_MIN_SIZE, HEAP_MIN_SIZE);
  else
    grub_available_iterate (heap_init);
}

#ifdef __i386__

grub_uint32_t grub_upper_mem;

/* We need to call this before grub_claim_memory.  */
static void
grub_get_extended_memory (void)
{
  auto int NESTED_FUNC_ATTR find_ext_mem (grub_uint64_t addr, grub_uint64_t len);
  int NESTED_FUNC_ATTR find_ext_mem (grub_uint64_t addr, grub_uint64_t len)
    {
      if (addr == 0x100000)
        {
          grub_upper_mem = len;
          return 1;
        }

      return 0;
    }

  grub_available_iterate (find_ext_mem);
}

#endif

static grub_uint64_t ieee1275_get_time_ms (void);

void
grub_machine_init (void)
{
  char args[256];
  int actual;

  grub_ieee1275_init ();

  grub_console_init ();
#ifdef __i386__
  grub_get_extended_memory ();
#endif
  grub_claim_heap ();
  grub_ofdisk_init ();

  /* Process commandline.  */
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

  grub_install_get_time_ms (ieee1275_get_time_ms);
}

void
grub_machine_fini (void)
{
  grub_ofdisk_fini ();
  grub_console_fini ();
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
  return ALIGN_UP((grub_addr_t) _end + GRUB_MOD_GAP, GRUB_MOD_ALIGN);
}
