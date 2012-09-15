/* grub-probe.c - probe device information for a given path */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/fs.h>
#include <grub/partition.h>
#include <grub/msdos_partition.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/diskfilter.h>
#include <grub/i18n.h>
#include <grub/emu/misc.h>
#include <grub/util/ofpath.h>
#include <grub/crypto.h>
#include <grub/cryptodisk.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>

#define _GNU_SOURCE	1
#include <argp.h>

#include "progname.h"

enum {
  PRINT_FS,
  PRINT_FS_UUID,
  PRINT_FS_LABEL,
  PRINT_DRIVE,
  PRINT_DEVICE,
  PRINT_PARTMAP,
  PRINT_ABSTRACTION,
  PRINT_CRYPTODISK_UUID,
  PRINT_HINT_STR,
  PRINT_BIOS_HINT,
  PRINT_IEEE1275_HINT,
  PRINT_BAREMETAL_HINT,
  PRINT_EFI_HINT,
  PRINT_ARC_HINT,
  PRINT_COMPATIBILITY_HINT,
  PRINT_MSDOS_PARTTYPE,
  PRINT_ZERO_CHECK,
  PRINT_DISK
};

static int print = PRINT_FS;
static unsigned int argument_is_device = 0;

static void
probe_partmap (grub_disk_t disk)
{
  grub_partition_t part;
  grub_disk_memberlist_t list = NULL, tmp;

  if (disk->partition == NULL)
    {
      grub_util_info ("no partition map found for %s", disk->name);
    }

  for (part = disk->partition; part; part = part->parent)
    printf ("%s ", part->partmap->name);

  if (disk->dev->id == GRUB_DISK_DEVICE_DISKFILTER_ID)
    grub_diskfilter_print_partmap (disk);

  /* In case of LVM/RAID, check the member devices as well.  */
  if (disk->dev->memberlist)
    {
      list = disk->dev->memberlist (disk);
    }
  while (list)
    {
      probe_partmap (list->disk);
      tmp = list->next;
      free (list);
      list = tmp;
    }
}

static void
probe_cryptodisk_uuid (grub_disk_t disk)
{
  grub_disk_memberlist_t list = NULL, tmp;

  /* In case of LVM/RAID, check the member devices as well.  */
  if (disk->dev->memberlist)
    {
      list = disk->dev->memberlist (disk);
    }
  while (list)
    {
      probe_cryptodisk_uuid (list->disk);
      tmp = list->next;
      free (list);
      list = tmp;
    }
  if (disk->dev->id == GRUB_DISK_DEVICE_CRYPTODISK_ID)
    grub_util_cryptodisk_print_uuid (disk);
}

static int
probe_raid_level (grub_disk_t disk)
{
  /* disk might be NULL in the case of a LVM physical volume with no LVM
     signature.  Ignore such cases here.  */
  if (!disk)
    return -1;

  if (disk->dev->id != GRUB_DISK_DEVICE_DISKFILTER_ID)
    return -1;

  if (disk->name[0] != 'm' || disk->name[1] != 'd')
    return -1;

  if (!((struct grub_diskfilter_lv *) disk->data)->segments)
    return -1;
  return ((struct grub_diskfilter_lv *) disk->data)->segments->type;
}

/* Since OF path names can have "," characters in them, and GRUB
   internally uses "," to indicate partitions (unlike OF which uses
   ":" for this purpose) we escape such commas.  */
static char *
escape_of_path (const char *orig_path)
{
  char *new_path, *d, c;
  const char *p;

  if (!strchr (orig_path, ','))
    return (char *) xstrdup (orig_path);

  new_path = xmalloc (strlen (orig_path) * 2 + 1);

  p = orig_path;
  d = new_path;
  while ((c = *p++) != '\0')
    {
      if (c == ',')
	*d++ = '\\';
      *d++ = c;
    }
  *d = 0;

  return new_path;
}

static char *
guess_bios_drive (const char *orig_path)
{
  char *canon;
  char *ptr;
  canon = canonicalize_file_name (orig_path);
  if (!canon)
    return NULL;
  ptr = strrchr (orig_path, '/');
  if (ptr)
    ptr++;
  else
    ptr = canon;
  if ((ptr[0] == 's' || ptr[0] == 'h') && ptr[1] == 'd')
    {
      int num = ptr[2] - 'a';
      free (canon);
      return xasprintf ("hd%d", num);
    }
  if (ptr[0] == 'f' && ptr[1] == 'd')
    {
      int num = atoi (ptr + 2);
      free (canon);
      return xasprintf ("fd%d", num);
    }
  free (canon);
  return NULL;
}

static char *
guess_efi_drive (const char *orig_path)
{
  char *canon;
  char *ptr;
  canon = canonicalize_file_name (orig_path);
  if (!canon)
    return NULL;
  ptr = strrchr (orig_path, '/');
  if (ptr)
    ptr++;
  else
    ptr = canon;
  if ((ptr[0] == 's' || ptr[0] == 'h') && ptr[1] == 'd')
    {
      int num = ptr[2] - 'a';
      free (canon);
      return xasprintf ("hd%d", num);
    }
  if (ptr[0] == 'f' && ptr[1] == 'd')
    {
      int num = atoi (ptr + 2);
      free (canon);
      return xasprintf ("fd%d", num);
    }
  free (canon);
  return NULL;
}

static char *
guess_baremetal_drive (const char *orig_path)
{
  char *canon;
  char *ptr;
  canon = canonicalize_file_name (orig_path);
  if (!canon)
    return NULL;
  ptr = strrchr (orig_path, '/');
  if (ptr)
    ptr++;
  else
    ptr = canon;
  if (ptr[0] == 'h' && ptr[1] == 'd')
    {
      int num = ptr[2] - 'a';
      free (canon);
      return xasprintf ("ata%d", num);
    }
  if (ptr[0] == 's' && ptr[1] == 'd')
    {
      int num = ptr[2] - 'a';
      free (canon);
      return xasprintf ("ahci%d", num);
    }
  free (canon);
  return NULL;
}

static void
print_full_name (const char *drive, grub_device_t dev)
{
  char *dname = escape_of_path (drive);
  if (dev->disk->partition)
    {
      char *pname = grub_partition_get_name (dev->disk->partition);
      printf ("%s,%s", dname, pname);
      free (pname);
    }
  else
    printf ("%s", dname);
  free (dname);
} 

static void
probe_abstraction (grub_disk_t disk)
{
  grub_disk_memberlist_t list = NULL, tmp;
  int raid_level;

  if (disk->dev->memberlist)
    list = disk->dev->memberlist (disk);
  while (list)
    {
      probe_abstraction (list->disk);

      tmp = list->next;
      free (list);
      list = tmp;
    }

  if (disk->dev->id == GRUB_DISK_DEVICE_DISKFILTER_ID
      && grub_memcmp (disk->name, "lvm/", sizeof ("lvm/") - 1) == 0)
    printf ("lvm ");

  if (disk->dev->id == GRUB_DISK_DEVICE_DISKFILTER_ID
      && grub_memcmp (disk->name, "ldm/", sizeof ("ldm/") - 1) == 0)
    printf ("ldm ");

  if (disk->dev->id == GRUB_DISK_DEVICE_CRYPTODISK_ID)
    grub_util_cryptodisk_print_abstraction (disk);

  raid_level = probe_raid_level (disk);
  if (raid_level >= 0)
    {
      printf ("diskfilter ");
      if (disk->dev->raidname)
	printf ("%s ", disk->dev->raidname (disk));
    }
  if (raid_level == 5)
    printf ("raid5rec ");
  if (raid_level == 6)
    printf ("raid6rec ");
}

static void
probe (const char *path, char **device_names, char delim)
{
  char **drives_names = NULL;
  char **curdev, **curdrive;
  char *grub_path = NULL;
  int ndev = 0;

  if (path != NULL)
    {
      grub_path = canonicalize_file_name (path);
      if (! grub_path)
	grub_util_error (_("failed to get canonical path of %s"), path);
      device_names = grub_guess_root_devices (grub_path);
      free (grub_path);
    }

  if (! device_names)
    grub_util_error (_("cannot find a device for %s (is /dev mounted?)"), path);

  if (print == PRINT_DEVICE)
    {
      for (curdev = device_names; *curdev; curdev++)
	{
	  printf ("%s", *curdev);
	  putchar (delim);
	}
      return;
    }

  if (print == PRINT_DISK)
    {
      for (curdev = device_names; *curdev; curdev++)
	{
	  char *disk;
	  disk = grub_util_get_os_disk (*curdev);
	  if (!disk)
	    {
	      grub_print_error ();
	      continue;
	    }
	  printf ("%s", disk);
	  putchar (delim);
	}
      return;
    }

  for (curdev = device_names; *curdev; curdev++)
    {
      grub_util_pull_device (*curdev);
      ndev++;
    }
  
  drives_names = xmalloc (sizeof (drives_names[0]) * (ndev + 1)); 

  for (curdev = device_names, curdrive = drives_names; *curdev; curdev++,
       curdrive++)
    {
      *curdrive = grub_util_get_grub_dev (*curdev);
      if (! *curdrive)
	grub_util_error (_("cannot find a GRUB drive for %s.  Check your device.map"),
			 *curdev);
    }
  *curdrive = 0;

  if (print == PRINT_DRIVE)
    {
      for (curdrive = drives_names; *curdrive; curdrive++)
	{
	  printf ("(%s)", *curdrive);
	  putchar (delim);
	}
      goto end;
    }

  if (print == PRINT_ZERO_CHECK)
    {
      for (curdev = drives_names; *curdev; curdev++)
	{
	  grub_device_t dev = NULL;
	  grub_uint32_t buffer[32768];
	  grub_disk_addr_t addr;
	  grub_disk_addr_t dsize;

	  grub_util_info ("opening %s", *curdev);
	  dev = grub_device_open (*curdev);
	  if (! dev || !dev->disk)
	    grub_util_error ("%s", grub_errmsg);

	  dsize = grub_disk_get_size (dev->disk);
	  for (addr = 0; addr < dsize;
	       addr += sizeof (buffer) / GRUB_DISK_SECTOR_SIZE)
	    {
	      grub_size_t sz = sizeof (buffer);
	      grub_uint32_t *ptr;

	      if (sizeof (buffer) / GRUB_DISK_SECTOR_SIZE > dsize - addr)
		sz = (dsize - addr) * GRUB_DISK_SECTOR_SIZE;
	      grub_disk_read (dev->disk, addr, 0, sz, buffer);

	      for (ptr = buffer; ptr < buffer + sz / sizeof (*buffer); ptr++)
		if (*ptr)
		  {
		    grub_printf ("false\n");
		    grub_device_close (dev);
		    goto end;
		  }
	    }

	  grub_device_close (dev);
	}
      grub_printf ("true\n");
    }

  if (print == PRINT_FS || print == PRINT_FS_UUID
      || print == PRINT_FS_LABEL)
    {
      grub_device_t dev = NULL;
      grub_fs_t fs;

      grub_util_info ("opening %s", drives_names[0]);
      dev = grub_device_open (drives_names[0]);
      if (! dev)
	grub_util_error ("%s", grub_errmsg);
      
      fs = grub_fs_probe (dev);
      if (! fs)
	grub_util_error ("%s", grub_errmsg);

      if (print == PRINT_FS)
	{
	  printf ("%s", fs->name);
	  putchar (delim);
	}
      else if (print == PRINT_FS_UUID)
	{
	  char *uuid;
	  if (! fs->uuid)
	    grub_util_error (_("%s does not support UUIDs"), fs->name);

	  if (fs->uuid (dev, &uuid) != GRUB_ERR_NONE)
	    grub_util_error ("%s", grub_errmsg);

	  printf ("%s", uuid);
	  putchar (delim);
	}
      else if (print == PRINT_FS_LABEL)
	{
	  char *label;
	  if (! fs->label)
	    grub_util_error (_("filesystem `%s' does not support labels"),
			     fs->name);

	  if (fs->label (dev, &label) != GRUB_ERR_NONE)
	    grub_util_error ("%s", grub_errmsg);

	  printf ("%s", label);
	  putchar (delim);
	}
      goto end;
    }

  for (curdrive = drives_names, curdev = device_names; *curdrive;
       curdrive++, curdev++)
    {
      grub_device_t dev = NULL;

      grub_util_info ("opening %s", *curdrive);
      dev = grub_device_open (*curdrive);
      if (! dev)
	grub_util_error ("%s", grub_errmsg);

      if (print == PRINT_HINT_STR)
	{
	  const char *osdev = grub_util_biosdisk_get_osdev (dev->disk);
	  const char *ofpath = osdev ? grub_util_devname_to_ofpath (osdev) : 0;
	  char *biosname, *bare, *efi;
	  const char *map;

	  if (ofpath)
	    {
	      char *tmp = xmalloc (strlen (ofpath) + sizeof ("ieee1275/"));
	      char *p;
	      p = stpcpy (tmp, "ieee1275/");
	      strcpy (p, ofpath);
	      printf ("--hint-ieee1275='");
	      print_full_name (tmp, dev);
	      printf ("' ");
	      free (tmp);
	    }

	  biosname = guess_bios_drive (*curdev);
	  if (biosname)
	    {
	      printf ("--hint-bios=");
	      print_full_name (biosname, dev);
	      printf (" ");
	    }
	  free (biosname);

	  efi = guess_efi_drive (*curdev);
	  if (efi)
	    {
	      printf ("--hint-efi=");
	      print_full_name (efi, dev);
	      printf (" ");
	    }
	  free (efi);

	  bare = guess_baremetal_drive (*curdev);
	  if (bare)
	    {
	      printf ("--hint-baremetal=");
	      print_full_name (bare, dev);
	      printf (" ");
	    }
	  free (bare);

	  /* FIXME: Add ARC hint.  */

	  map = grub_util_biosdisk_get_compatibility_hint (dev->disk);
	  if (map)
	    {
	      printf ("--hint='");
	      print_full_name (map, dev);
	      printf ("' ");
	    }
	  printf ("\n");

	  grub_device_close (dev);
	  continue;
	}
      
      if ((print == PRINT_COMPATIBILITY_HINT || print == PRINT_BIOS_HINT
	   || print == PRINT_IEEE1275_HINT || print == PRINT_BAREMETAL_HINT
	   || print == PRINT_EFI_HINT || print == PRINT_ARC_HINT)
	  && dev->disk->dev->id != GRUB_DISK_DEVICE_HOSTDISK_ID)
	{
	  print_full_name (dev->disk->name, dev);
	  putchar (delim);
	  continue;
	}

      if (print == PRINT_COMPATIBILITY_HINT)
	{
	  const char *map;
	  char *biosname;
	  map = grub_util_biosdisk_get_compatibility_hint (dev->disk);
	  if (map)
	    {
	      print_full_name (map, dev);
	      putchar (delim);
	      grub_device_close (dev);
	      /* Compatibility hint is one device only.  */
	      break;
	    }
	  biosname = guess_bios_drive (*curdev);
	  if (biosname)
	    {
	      print_full_name (biosname, dev);
	      putchar (delim);
	    }
	  free (biosname);
	  grub_device_close (dev);
	  /* Compatibility hint is one device only.  */
	  if (biosname)
	    break;
	  continue;
	}

      if (print == PRINT_BIOS_HINT)
	{
	  char *biosname;
	  biosname = guess_bios_drive (*curdev);
	  if (biosname)
	    {
	      print_full_name (biosname, dev);
	      putchar (delim);
	    }
	  free (biosname);
	  grub_device_close (dev);
	  continue;
	}
      if (print == PRINT_IEEE1275_HINT)
	{
	  const char *osdev = grub_util_biosdisk_get_osdev (dev->disk);
	  const char *ofpath = grub_util_devname_to_ofpath (osdev);
	  const char *map;

	  map = grub_util_biosdisk_get_compatibility_hint (dev->disk);
	  if (map)
	    {
	      print_full_name (map, dev);
	      putchar (delim);
	    }

	  if (ofpath)
	    {
	      char *tmp = xmalloc (strlen (ofpath) + sizeof ("ieee1275/"));
	      char *p;
	      p = stpcpy (tmp, "ieee1275/");
	      strcpy (p, ofpath);
	      print_full_name (tmp, dev);
	      free (tmp);
	      putchar (delim);
	    }

	  grub_device_close (dev);
	  continue;
	}
      if (print == PRINT_EFI_HINT)
	{
	  char *biosname;
	  const char *map;
	  biosname = guess_efi_drive (*curdev);

	  map = grub_util_biosdisk_get_compatibility_hint (dev->disk);
	  if (map)
	    {
	      print_full_name (map, dev);
	      putchar (delim);
	    }
	  if (biosname)
	    {
	      print_full_name (biosname, dev);
	      putchar (delim);
	    }

	  free (biosname);
	  grub_device_close (dev);
	  continue;
	}

      if (print == PRINT_BAREMETAL_HINT)
	{
	  char *biosname;
	  const char *map;

	  biosname = guess_baremetal_drive (*curdev);

	  map = grub_util_biosdisk_get_compatibility_hint (dev->disk);
	  if (map)
	    {
	      print_full_name (map, dev);
	      putchar (delim);
	    }
	  if (biosname)
	    {
	      print_full_name (biosname, dev);
	      putchar (delim);
	    }

	  free (biosname);
	  grub_device_close (dev);
	  continue;
	}

      if (print == PRINT_ARC_HINT)
	{
	  const char *map;

	  map = grub_util_biosdisk_get_compatibility_hint (dev->disk);
	  if (map)
	    {
	      print_full_name (map, dev);
	      putchar (delim);
	    }

	  /* FIXME */
	  grub_device_close (dev);
	  continue;
	}

      if (print == PRINT_ABSTRACTION)
	{
	  probe_abstraction (dev->disk);
	  putchar (delim);
	  grub_device_close (dev);
	  continue;
	}

      if (print == PRINT_CRYPTODISK_UUID)
	{
	  probe_cryptodisk_uuid (dev->disk);
	  putchar (delim);
	  grub_device_close (dev);
	  continue;
	}

      if (print == PRINT_PARTMAP)
	{
	  /* Check if dev->disk itself is contained in a partmap.  */
	  probe_partmap (dev->disk);
	  putchar (delim);
	  grub_device_close (dev);
	  continue;
	}

      if (print == PRINT_MSDOS_PARTTYPE)
	{
	  if (dev->disk->partition
	      && strcmp(dev->disk->partition->partmap->name, "msdos") == 0)
	    printf ("%02x", dev->disk->partition->msdostype);

	  putchar (delim);
	  grub_device_close (dev);
	  continue;
	}
    }

 end:
  for (curdrive = drives_names; *curdrive; curdrive++)
    free (*curdrive);
  free (drives_names);
}

static struct argp_option options[] = {
  {"device",  'd', 0, 0,
   N_("given argument is a system device, not a path"), 0},
  {"device-map",  'm', N_("FILE"), 0,
   N_("use FILE as the device map [default=%s]"), 0},
  {"target",  't', "(fs|fs_uuid|fs_label|drive|device|partmap|abstraction|cryptodisk_uuid|msdos_parttype)", 0,
   N_("print filesystem module, GRUB drive, system device, partition map module, abstraction module or cryptographic container UUID [default=fs]"), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  { 0, 0, 0, 0, 0, 0 }
};

static char *
help_filter (int key, const char *text, void *input __attribute__ ((unused)))
{
  switch (key)
    {
      case 'm':
        return xasprintf (text, DEFAULT_DEVICE_MAP);

      default:
        return (char *) text;
    }
}

struct arguments
{
  char **devices;
  size_t device_max;
  size_t ndevices;
  char *dev_map;
  int zero_delim;
};

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'd':
      argument_is_device = 1;
      break;

    case 'm':
      if (arguments->dev_map)
	free (arguments->dev_map);

      arguments->dev_map = xstrdup (arg);
      break;

    case 't':
      if (!strcmp (arg, "fs"))
	print = PRINT_FS;
      else if (!strcmp (arg, "fs_uuid"))
	print = PRINT_FS_UUID;
      else if (!strcmp (arg, "fs_label"))
	print = PRINT_FS_LABEL;
      else if (!strcmp (arg, "drive"))
	print = PRINT_DRIVE;
      else if (!strcmp (arg, "device"))
	print = PRINT_DEVICE;
      else if (!strcmp (arg, "partmap"))
	print = PRINT_PARTMAP;
      else if (!strcmp (arg, "abstraction"))
	print = PRINT_ABSTRACTION;
      else if (!strcmp (arg, "cryptodisk_uuid"))
	print = PRINT_CRYPTODISK_UUID;
      else if (!strcmp (arg, "msdos_parttype"))
	print = PRINT_MSDOS_PARTTYPE;
      else if (!strcmp (arg, "hints_string"))
	print = PRINT_HINT_STR;
      else if (!strcmp (arg, "bios_hints"))
	print = PRINT_BIOS_HINT;
      else if (!strcmp (arg, "ieee1275_hints"))
	print = PRINT_IEEE1275_HINT;
      else if (!strcmp (arg, "baremetal_hints"))
	print = PRINT_BAREMETAL_HINT;
      else if (!strcmp (arg, "efi_hints"))
	print = PRINT_EFI_HINT;
      else if (!strcmp (arg, "arc_hints"))
	print = PRINT_ARC_HINT;
      else if (!strcmp (arg, "compatibility_hint"))
	print = PRINT_COMPATIBILITY_HINT;
      else if (strcmp (arg, "zero_check") == 0)
	print = PRINT_ZERO_CHECK;
      else if (!strcmp (arg, "disk"))
	print = PRINT_DISK;
      else
	argp_usage (state);
      break;

    case '0':
      arguments->zero_delim = 1;
      break;

    case 'v':
      verbosity++;
      break;

    case ARGP_KEY_NO_ARGS:
      fprintf (stderr, "%s", _("No path or device is specified.\n"));
      argp_usage (state);
      break;

    case ARGP_KEY_ARG:
      assert (arguments->ndevices < arguments->device_max);
      arguments->devices[arguments->ndevices++] = xstrdup(arg);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("[OPTION]... [PATH|DEVICE]"),
  N_("\
Probe device information for a given path (or device, if the -d option is given)."),
  NULL, help_filter, NULL
};

int
main (int argc, char *argv[])
{
  char delim;
  struct arguments arguments;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  memset (&arguments, 0, sizeof (struct arguments));
  arguments.device_max = argc + 1;
  arguments.devices = xmalloc ((arguments.device_max + 1)
			       * sizeof (arguments.devices[0]));
  memset (arguments.devices, 0, (arguments.device_max + 1)
	  * sizeof (arguments.devices[0]));

  /* Parse our arguments */
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  if (verbosity > 1)
    grub_env_set ("debug", "all");

  /* Obtain ARGUMENT.  */
  if (arguments.ndevices != 1 && !argument_is_device)
    {
      char *program = xstrdup(program_name);
      fprintf (stderr, _("Unknown extra argument `%s'."), arguments.devices[1]);
      fprintf (stderr, "\n");
      argp_help (&argp, stderr, ARGP_HELP_STD_USAGE, program);
      free (program);
      exit(1);
    }

  /* Initialize the emulated biosdisk driver.  */
  grub_util_biosdisk_init (arguments.dev_map ? : DEFAULT_DEVICE_MAP);

  /* Initialize all modules. */
  grub_init_all ();
  grub_gcry_init_all ();

  grub_lvm_fini ();
  grub_mdraid09_fini ();
  grub_mdraid1x_fini ();
  grub_diskfilter_fini ();
  grub_diskfilter_init ();
  grub_mdraid09_init ();
  grub_mdraid1x_init ();
  grub_lvm_init ();

  if (print == PRINT_BIOS_HINT
      || print == PRINT_IEEE1275_HINT || print == PRINT_BAREMETAL_HINT
      || print == PRINT_EFI_HINT || print == PRINT_ARC_HINT)
    delim = ' ';
  else
    delim = '\n';

  if (arguments.zero_delim)
    delim = '\0';

  /* Do it.  */
  if (argument_is_device)
    probe (NULL, arguments.devices, delim);
  else
    probe (arguments.devices[0], NULL, delim);

  if (!arguments.zero_delim && (print == PRINT_BIOS_HINT
				|| print == PRINT_IEEE1275_HINT
				|| print == PRINT_BAREMETAL_HINT
				|| print == PRINT_EFI_HINT
				|| print == PRINT_ARC_HINT))
    putchar ('\n');

  /* Free resources.  */
  grub_gcry_fini_all ();
  grub_fini_all ();
  grub_util_biosdisk_fini ();

  {
    size_t i;
    for (i = 0; i < arguments.ndevices; i++)
      free (arguments.devices[i]);
  }
  free (arguments.devices);

  free (arguments.dev_map);

  return 0;
}
