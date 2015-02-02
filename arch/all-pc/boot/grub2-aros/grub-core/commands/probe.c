/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/net.h>
#include <grub/fs.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static const struct grub_arg_option options[] =
  {
    {"set",             's', 0,
     N_("Set a variable to return value."), N_("VARNAME"), ARG_TYPE_STRING},
    /* TRANSLATORS: It's a driver that is currently in use to access
       the diven disk.  */
    {"driver",		'd', 0, N_("Determine driver."), 0, 0},
    {"partmap",		'p', 0, N_("Determine partition map type."), 0, 0},
    {"fs",		'f', 0, N_("Determine filesystem type."), 0, 0},
    {"fs-uuid",		'u', 0, N_("Determine filesystem UUID."), 0, 0},
    {"label",		'l', 0, N_("Determine filesystem label."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
grub_cmd_probe (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;
  grub_device_t dev;
  grub_fs_t fs;
  char *ptr;
  grub_err_t err;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "device name required");

  ptr = args[0] + grub_strlen (args[0]) - 1;
  if (args[0][0] == '(' && *ptr == ')')
    {
      *ptr = 0;
      dev = grub_device_open (args[0] + 1);
      *ptr = ')';
    }
  else
    dev = grub_device_open (args[0]);
  if (! dev)
    return grub_errno;

  if (state[1].set)
    {
      const char *val = "none";
      if (dev->net)
	val = dev->net->protocol->name;
      if (dev->disk)
	val = dev->disk->dev->name;
      if (state[0].set)
	grub_env_set (state[0].arg, val);
      else
	grub_printf ("%s", val);
      grub_device_close (dev);
      return GRUB_ERR_NONE;
    }
  if (state[2].set)
    {
      const char *val = "none";
      if (dev->disk && dev->disk->partition)
	val = dev->disk->partition->partmap->name;
      if (state[0].set)
	grub_env_set (state[0].arg, val);
      else
	grub_printf ("%s", val);
      grub_device_close (dev);
      return GRUB_ERR_NONE;
    }
  fs = grub_fs_probe (dev);
  if (! fs)
    return grub_errno;
  if (state[3].set)
    {
      if (state[0].set)
	grub_env_set (state[0].arg, fs->name);
      else
	grub_printf ("%s", fs->name);
      grub_device_close (dev);
      return GRUB_ERR_NONE;
    }
  if (state[4].set)
    {
      char *uuid;
      if (! fs->uuid)
	return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			   N_("%s does not support UUIDs"), fs->name);
      err = fs->uuid (dev, &uuid);
      if (err)
	return err;
      if (! uuid)
	return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			   N_("%s does not support UUIDs"), fs->name);

      if (state[0].set)
	grub_env_set (state[0].arg, uuid);
      else
	grub_printf ("%s", uuid);
      grub_free (uuid);
      grub_device_close (dev);
      return GRUB_ERR_NONE;
    }
  if (state[5].set)
    {
      char *label;
      if (! fs->label)
	return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			   N_("filesystem `%s' does not support labels"),
			   fs->name);
      err = fs->label (dev, &label);
      if (err)
	return err;
      if (! label)
	return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			   N_("filesystem `%s' does not support labels"),
			   fs->name);

      if (state[0].set)
	grub_env_set (state[0].arg, label);
      else
	grub_printf ("%s", label);
      grub_free (label);
      grub_device_close (dev);
      return GRUB_ERR_NONE;
    }
  grub_device_close (dev);
  return grub_error (GRUB_ERR_BAD_ARGUMENT, "unrecognised target");
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT (probe)
{
  cmd = grub_register_extcmd ("probe", grub_cmd_probe, 0, N_("DEVICE"),
			      N_("Retrieve device info."), options);
}

GRUB_MOD_FINI (probe)
{
  grub_unregister_extcmd (cmd);
}
