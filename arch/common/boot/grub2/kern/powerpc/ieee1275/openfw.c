/*  openfw.c -- Open firmware support functions.  */
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

#include <alloca.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/machine/kernel.h>
#include <grub/ieee1275/ieee1275.h>

enum grub_ieee1275_parse_type
{
  GRUB_PARSE_FILENAME,
  GRUB_PARSE_PARTITION,
};

/* Walk children of 'devpath', calling hook for each.  */
grub_err_t
grub_children_iterate (char *devpath,
		  int (*hook) (struct grub_ieee1275_devalias *alias))
{
  grub_ieee1275_phandle_t dev;
  grub_ieee1275_phandle_t child;

  if (grub_ieee1275_finddevice (devpath, &dev))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Unknown device");

  if (grub_ieee1275_child (dev, &child))
    return grub_error (GRUB_ERR_BAD_DEVICE, "Device has no children");

  do
    {
      /* XXX: Don't use hardcoded path lengths.  */
      char childtype[64];
      char childpath[64];
      char childname[64];
      char fullname[64];
      struct grub_ieee1275_devalias alias;
      int actual;

      if (grub_ieee1275_get_property (child, "device_type", &childtype,
				      sizeof childtype, &actual))
	continue;

      if (grub_ieee1275_package_to_path (child, childpath, sizeof childpath,
					 &actual))
	continue;

      if (grub_ieee1275_get_property (child, "name", &childname,
				      sizeof childname, &actual))
	continue;

      grub_sprintf (fullname, "%s/%s", devpath, childname);

      alias.type = childtype;
      alias.path = childpath;
      alias.name = fullname;
      hook (&alias);
    }
  while (grub_ieee1275_peer (child, &child));

  return 0;
}

/* Iterate through all device aliases.  This function can be used to
   find a device of a specific type.  */
grub_err_t
grub_devalias_iterate (int (*hook) (struct grub_ieee1275_devalias *alias))
{
  grub_ieee1275_phandle_t aliases;
  char aliasname[32];
  int actual;
  struct grub_ieee1275_devalias alias;

  if (grub_ieee1275_finddevice ("/aliases", &aliases))
    return -1;

  /* Find the first property.  */
  aliasname[0] = '\0';

  while (grub_ieee1275_next_property (aliases, aliasname, aliasname))
    {
      grub_ieee1275_phandle_t dev;
      grub_ssize_t pathlen;
      char *devpath;
      /* XXX: This should be large enough for any possible case.  */
      char devtype[64];

      grub_dprintf ("devalias", "devalias name = %s\n", aliasname);

      grub_ieee1275_get_property_length (aliases, aliasname, &pathlen);

      /* The property `name' is a special case we should skip.  */
      if (!grub_strcmp (aliasname, "name"))
	continue;

      devpath = grub_malloc (pathlen);
      if (! devpath)
	return grub_errno;

      if (grub_ieee1275_get_property (aliases, aliasname, devpath, pathlen,
				      &actual))
	{
	  grub_dprintf ("devalias", "get_property (%s) failed\n", aliasname);
	  goto nextprop;
	}

      if (grub_ieee1275_finddevice (devpath, &dev))
	{
	  grub_dprintf ("devalias", "finddevice (%s) failed\n", devpath);
	  goto nextprop;
	}

      if (grub_ieee1275_get_property (dev, "device_type", devtype,
				      sizeof devtype, &actual))
	{
	  grub_dprintf ("devalias", "get device type failed\n");
	  goto nextprop;
	}

      alias.name = aliasname;
      alias.path = devpath;
      alias.type = devtype;
      hook (&alias);

nextprop:
      grub_free (devpath);
    }

  return 0;
}

grub_err_t grub_available_iterate (int (*hook) (grub_uint64_t, grub_uint64_t))
{
  grub_ieee1275_phandle_t root;
  grub_ieee1275_phandle_t memory;
  grub_uint32_t available[32];
  grub_ssize_t available_size;
  grub_uint32_t address_cells = 1;
  grub_uint32_t size_cells = 1;
  int i;

  /* Determine the format of each entry in `available'.  */
  grub_ieee1275_finddevice ("/", &root);
  grub_ieee1275_get_integer_property (root, "#address-cells", &address_cells,
				      sizeof address_cells, 0);
  grub_ieee1275_get_integer_property (root, "#size-cells", &size_cells,
				      sizeof size_cells, 0);

  /* Load `/memory/available'.  */
  if (grub_ieee1275_finddevice ("/memory", &memory))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		       "Couldn't find /memory node");
  if (grub_ieee1275_get_integer_property (memory, "available", available,
					  sizeof available, &available_size))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		       "Couldn't examine /memory/available property");

  /* Decode each entry and call `hook'.  */
  i = 0;
  available_size /= sizeof (grub_uint32_t);
  while (i < available_size)
    {
      grub_uint64_t address;
      grub_uint64_t size;

      address = available[i++];
      if (address_cells == 2)
	address = (address << 32) | available[i++];

      size = available[i++];
      if (size_cells == 2)
	size = (size << 32) | available[i++];

      if (hook (address, size))
	break;
    }

  return grub_errno;
}

/* Call the "map" method of /chosen/mmu.  */
static int
grub_map (grub_addr_t phys, grub_addr_t virt, grub_uint32_t size,
		   grub_uint8_t mode)
{
  struct map_args {
    struct grub_ieee1275_common_hdr common;
    char *method;
    grub_ieee1275_ihandle_t ihandle;
    grub_uint32_t mode;
    grub_uint32_t size;
    grub_uint32_t virt;
    grub_uint32_t phys;
    int catch_result;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 6, 1);
  args.method = "map";
  args.ihandle = grub_ieee1275_mmu;
  args.phys = phys;
  args.virt = virt;
  args.size = size;
  args.mode = mode; /* Format is WIMG0PP.  */

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  return args.catch_result;
}

int
grub_claimmap (grub_addr_t addr, grub_size_t size)
{
  if (grub_ieee1275_claim (addr, size, 0, 0))
    return -1;

  if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_REAL_MODE)
      && grub_map (addr, addr, size, 0x00))
    {
      grub_printf ("map failed: address 0x%x, size 0x%x\n", addr, size);
      grub_ieee1275_release (addr, size);
      return -1;
    }

  return 0;
}

/* Get the device arguments of the Open Firmware node name `path'.  */
static char *
grub_ieee1275_get_devargs (const char *path)
{
  char *colon = grub_strchr (path, ':');

  if (! colon)
    return 0;

  return grub_strdup (colon + 1);
}

/* Get the device path of the Open Firmware node name `path'.  */
static char *
grub_ieee1275_get_devname (const char *path)
{
  char *colon = grub_strchr (path, ':');
  char *newpath = 0;
  int pathlen = grub_strlen (path);
  auto int match_alias (struct grub_ieee1275_devalias *alias);

  int match_alias (struct grub_ieee1275_devalias *curalias)
    {
      /* briQ firmware can change capitalization in /chosen/bootpath.  */
      if (! grub_strncasecmp (curalias->path, path, pathlen))
        {
	  newpath = grub_strdup (curalias->name);
	  return 1;
	}

      return 0;
    }

  if (colon)
    pathlen = (int)(colon - path);

  /* Try to find an alias for this device.  */
  grub_devalias_iterate (match_alias);

  if (! newpath)
    newpath = grub_strndup (path, pathlen);

  return newpath;
}

static char *
grub_ieee1275_parse_args (const char *path, enum grub_ieee1275_parse_type ptype)
{
  char type[64]; /* XXX check size.  */
  char *device = grub_ieee1275_get_devname (path);
  char *args = grub_ieee1275_get_devargs (path);
  char *ret = 0;
  grub_ieee1275_phandle_t dev;

  if (!args)
    /* Shouldn't happen.  */
    return 0;

  /* We need to know what type of device it is in order to parse the full
     file path properly.  */
  if (grub_ieee1275_finddevice (device, &dev))
    {
      grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Device %s not found\n", device);
      goto fail;
    }
  if (grub_ieee1275_get_property (dev, "device_type", &type, sizeof type, 0))
    {
      grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		  "Device %s lacks a device_type property\n", device);
      goto fail;
    }

  if (!grub_strcmp ("block", type))
    {
      /* The syntax of the device arguments is defined in the CHRP and PReP
         IEEE1275 bindings: "[partition][,[filename]]".  */
      char *comma = grub_strchr (args, ',');

      if (ptype == GRUB_PARSE_FILENAME)
	{
	  if (comma)
	    {
	      char *filepath = comma + 1;

	      ret = grub_malloc (grub_strlen (filepath) + 1);
	      /* Make sure filepath has leading backslash.  */
	      if (filepath[0] != '\\')
		grub_sprintf (ret, "\\%s", filepath);
	      else
		grub_strcpy (ret, filepath);
	    }
	}
      else if (ptype == GRUB_PARSE_PARTITION)
        {
	  if (!comma)
	    ret = grub_strdup (args);
	  else
	    ret = grub_strndup (args, (grub_size_t)(comma - args));
	}
    }
  else
    {
      /* XXX Handle net devices by configuring & registering a grub_net_dev
	 here, then return its name?
	 Example path: "net:<server ip>,<file name>,<client ip>,<gateway
	 ip>,<bootp retries>,<tftp retries>".  */
      grub_printf ("Unsupported type %s for device %s\n", type, device);
    }

fail:
  grub_free (device);
  grub_free (args);
  return ret;
}

char *
grub_ieee1275_get_filename (const char *path)
{
  return grub_ieee1275_parse_args (path, GRUB_PARSE_FILENAME);
}

/* Convert a device name from IEEE1275 syntax to GRUB syntax.  */
char *
grub_ieee1275_encode_devname (const char *path)
{
  char *device = grub_ieee1275_get_devname (path);
  char *partition = grub_ieee1275_parse_args (path, GRUB_PARSE_PARTITION);
  char *encoding;

  if (partition)
    {
      unsigned int partno = grub_strtoul (partition, 0, 0);

      if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_0_BASED_PARTITIONS))
	/* GRUB partition 1 is OF partition 0.  */
	partno++;

      /* Assume partno will require less than five bytes to encode.  */
      encoding = grub_malloc (grub_strlen (device) + 3 + 5);
      grub_sprintf (encoding, "(%s,%d)", device, partno);
    }
  else
    {
      encoding = grub_malloc (grub_strlen (device) + 2);
      grub_sprintf (encoding, "(%s)", device);
    }

  grub_free (partition);
  grub_free (device);

  return encoding;
}

void
grub_reboot (void)
{
  grub_ieee1275_interpret ("reset-all", 0);
}

void
grub_halt (void)
{
  /* Not standarized.  We try both known commands.  */

  grub_ieee1275_interpret ("shut-down", 0);
  grub_ieee1275_interpret ("power-off", 0);
}
