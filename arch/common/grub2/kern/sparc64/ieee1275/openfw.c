/*  openfw.c -- Open firmware support funtions.  */
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

#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/machine/kernel.h> /* Needed ?  */
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

  grub_ieee1275_finddevice (devpath, &dev);
  if (((signed) dev) == -1)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Unknown device");

  grub_ieee1275_child (dev, &child);
  if (((signed) child) == -1)
    return grub_error (GRUB_ERR_BAD_DEVICE, "Device has no children");

  do
    {
      /* XXX: Don't use hardcoded path lengths.  */
      char childtype[64];
      char childpath[64];
      char childname[64];
      char fullname[64];
      struct grub_ieee1275_devalias alias;
      grub_ssize_t actual;

      grub_ieee1275_get_property (child, "device_type", childtype,
				  sizeof childtype, &actual);
      if (actual == -1)
	continue;

      grub_ieee1275_package_to_path (child, childpath, sizeof childpath,
      				     &actual);
      if (actual == -1)
	continue;

      grub_ieee1275_get_property (child, "name", childname,
				  sizeof childname, &actual);
      if (actual == -1)
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
  grub_ieee1275_phandle_t devalias;
  char aliasname[32];
  grub_ssize_t actual;
  grub_ieee1275_cell_t flags;
  struct grub_ieee1275_devalias alias;

  if (grub_ieee1275_finddevice ("/aliases", &devalias))
    return -1;

  aliasname[0] = '\0';

  while (grub_ieee1275_next_property (devalias, aliasname, aliasname, &flags) != -1
	 && ((signed) flags) != -1 )
    {
      grub_ieee1275_phandle_t dev;
      grub_ssize_t pathlen, typelen;
      char *devpath, *devtype;

      grub_dprintf ("devalias", "devalias name = %s\n", aliasname);
      
      /* The property `name' is a special case we should skip.  */
      if (!grub_strcmp (aliasname, "name"))
	  continue;
      
      grub_ieee1275_get_property_length (devalias, aliasname, &pathlen);
      devpath = grub_malloc (pathlen);
      if (! devpath)
	return grub_errno;

      if (grub_ieee1275_get_property (devalias, aliasname, devpath, pathlen,
				      &actual))
	{
          grub_dprintf ("devalias", "get_property (%s) failed\n", aliasname);
	  grub_free (devpath);
	  continue;
	}
      
      if (grub_ieee1275_finddevice (devpath, &dev) || ((signed) dev) == -1)
	{
	  grub_dprintf ("devalias", "finddevice (%s) failed\n", devpath);
	  grub_free (devpath);
	  continue;
	}

      grub_ieee1275_get_property_length (dev, "device_type", &typelen);
      devtype = grub_malloc (typelen);
      if (! devtype)
      {
        grub_free (devpath);
        return grub_errno;
      }
      if (grub_ieee1275_get_property (dev, "device_type", devtype, typelen, &actual))
	{
	  grub_dprintf ("devalias", "get device type failed\n");
          grub_free (devtype);
	  grub_free (devpath);
	  continue;
	}

      alias.name = aliasname;
      alias.path= devpath;
      alias.type = devtype;
      if((*hook) (&alias))
        {
          grub_free (devtype);
          grub_free (devpath);
          break;
        }

      grub_free (devtype);
      grub_free (devpath);
    }

  return 0;
}

/* FIXME (sparc64) */
#if 0
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
  grub_ieee1275_ihandle_t mmu;
  grub_ssize_t len;

  grub_ieee1275_get_property (grub_ieee1275_chosen, "mmu", &mmu, sizeof mmu,
			      &len);
  if (len != sizeof mmu)
    return -1;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 6, 1);
  args.method = "map";
  args.ihandle = mmu;
  args.phys = phys;
  args.virt = virt;
  args.size = size;
  args.mode = mode; /* Format is WIMG0PP.  */

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  return args.catch_result;
}
#endif

int
grub_claimmap (grub_addr_t addr, grub_size_t size)
{
  if (grub_ieee1275_claim (addr, size, 0, 0))
    return -1;
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
	  newpath = grub_strndup (curalias->name, grub_strlen (curalias->name));
	  return 1;
	}

      return 0;
    }

  if (colon)
    pathlen = (int)(colon - path);

  /* Try to find an alias for this device.  */
  grub_devalias_iterate (match_alias);

  if (! newpath)
    newpath = grub_strdup (path);

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
  if (grub_ieee1275_get_property (dev, "device_type", type, sizeof type, 0))
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
  grub_ieee1275_interpret ("power-off", 0);
}
