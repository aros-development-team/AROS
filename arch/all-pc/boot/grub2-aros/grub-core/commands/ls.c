/* ls.c - command to list files and devices */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/disk.h>
#include <grub/device.h>
#include <grub/term.h>
#include <grub/partition.h>
#include <grub/file.h>
#include <grub/normal.h>
#include <grub/extcmd.h>
#include <grub/datetime.h>
#include <grub/i18n.h>
#include <grub/net.h>

GRUB_MOD_LICENSE ("GPLv3+");

static const struct grub_arg_option options[] =
  {
    {"long", 'l', 0, N_("Show a long list with more detailed information."), 0, 0},
    {"human-readable", 'h', 0, N_("Print sizes in a human readable format."), 0, 0},
    {"all", 'a', 0, N_("List all files."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static const char grub_human_sizes[] = {' ', 'K', 'M', 'G', 'T'};

static grub_err_t
grub_ls_list_devices (int longlist)
{
  auto int grub_ls_print_devices (const char *name);
  int grub_ls_print_devices (const char *name)
    {
      if (longlist)
	grub_normal_print_device_info (name);
      else
	grub_printf ("(%s) ", name);

      return 0;
    }

  grub_device_iterate (grub_ls_print_devices);
  grub_xputs ("\n");

#if 0
  {
    grub_net_app_level_t proto;
    int first = 1;
    FOR_NET_APP_LEVEL (proto)
    {
      if (first)
	grub_puts_ (N_ ("Network protocols:"));
      first = 0;
      grub_printf ("%s ", proto->name);
    }
    grub_xputs ("\n");
  }
#endif

  grub_refresh ();

  return 0;
}

static grub_err_t
grub_ls_list_files (char *dirname, int longlist, int all, int human)
{
  char *device_name;
  grub_fs_t fs;
  const char *path;
  grub_device_t dev;

  auto int print_files (const char *filename,
			const struct grub_dirhook_info *info);
  auto int print_files_long (const char *filename,
			     const struct grub_dirhook_info *info);

  int print_files (const char *filename, const struct grub_dirhook_info *info)
    {
      if (all || filename[0] != '.')
	grub_printf ("%s%s ", filename, info->dir ? "/" : "");

      return 0;
    }

  int print_files_long (const char *filename,
			const struct grub_dirhook_info *info)
    {
      if ((! all) && (filename[0] == '.'))
	return 0;

      if (! info->dir)
	{
	  grub_file_t file;
	  char *pathname;

	  if (dirname[grub_strlen (dirname) - 1] == '/')
	    pathname = grub_xasprintf ("%s%s", dirname, filename);
	  else
	    pathname = grub_xasprintf ("%s/%s", dirname, filename);

	  if (!pathname)
	    return 1;

	  /* XXX: For ext2fs symlinks are detected as files while they
	     should be reported as directories.  */
	  grub_file_filter_disable_compression ();
	  file = grub_file_open (pathname);
	  if (! file)
	    {
	      grub_errno = 0;
	      grub_free (pathname);
	      return 0;
	    }

	  if (! human)
	    grub_printf ("%-12llu", (unsigned long long) file->size);
	  else
	    {
	      grub_uint64_t fsize = file->size * 100ULL;
	      grub_uint64_t fsz = file->size;
	      int units = 0;
	      char buf[20];

	      while (fsz / 1024)
		{
		  fsize = (fsize + 512) / 1024;
		  fsz /= 1024;
		  units++;
		}

	      if (units)
		{
		  grub_uint64_t whole, fraction;

		  whole = grub_divmod64 (fsize, 100, &fraction);
		  grub_snprintf (buf, sizeof (buf),
				 "%" PRIuGRUB_UINT64_T
				 ".%02" PRIuGRUB_UINT64_T "%c", whole, fraction,
				 grub_human_sizes[units]);
		  grub_printf ("%-12s", buf);
		}
	      else
		grub_printf ("%-12llu", (unsigned long long) file->size);

	    }
	  grub_file_close (file);
	  grub_free (pathname);
	}
      else
	grub_printf ("%-12s", _("DIR"));

      if (info->mtimeset)
	{
	  struct grub_datetime datetime;
	  grub_unixtime2datetime (info->mtime, &datetime);
	  if (human)
	    grub_printf (" %d-%02d-%02d %02d:%02d:%02d %-11s ",
			 datetime.year, datetime.month, datetime.day,
			 datetime.hour, datetime.minute,
			 datetime.second,
			 grub_get_weekday_name (&datetime));
	  else
	    grub_printf (" %04d%02d%02d%02d%02d%02d ",
			 datetime.year, datetime.month,
			 datetime.day, datetime.hour,
			 datetime.minute, datetime.second);
	}
      grub_printf ("%s%s\n", filename, info->dir ? "/" : "");

      return 0;
    }

  device_name = grub_file_get_device_name (dirname);
  dev = grub_device_open (device_name);
  if (! dev)
    goto fail;

  fs = grub_fs_probe (dev);
  path = grub_strchr (dirname, ')');
  if (! path)
    path = dirname;
  else
    path++;

  if (! path && ! device_name)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid argument");
      goto fail;
    }

  if (! *path)
    {
      if (grub_errno == GRUB_ERR_UNKNOWN_FS)
	grub_errno = GRUB_ERR_NONE;

      grub_normal_print_device_info (device_name);
    }
  else if (fs)
    {
      if (longlist)
	(fs->dir) (dev, path, print_files_long);
      else
	(fs->dir) (dev, path, print_files);

      if (grub_errno == GRUB_ERR_BAD_FILE_TYPE
	  && path[grub_strlen (path) - 1] != '/')
	{
	  /* PATH might be a regular file.  */
	  char *p;
	  grub_file_t file;
	  struct grub_dirhook_info info;
	  grub_errno = 0;

	  grub_file_filter_disable_compression ();
	  file = grub_file_open (dirname);
	  if (! file)
	    goto fail;

	  grub_file_close (file);

	  p = grub_strrchr (dirname, '/') + 1;
	  dirname = grub_strndup (dirname, p - dirname);
	  if (! dirname)
	    goto fail;

	  all = 1;
	  grub_memset (&info, 0, sizeof (info));
	  if (longlist)
	    print_files_long (p, &info);
	  else
	    print_files (p, &info);

	  grub_free (dirname);
	}

      if (grub_errno == GRUB_ERR_NONE)
	grub_xputs ("\n");

      grub_refresh ();
    }

 fail:
  if (dev)
    grub_device_close (dev);

  grub_free (device_name);

  return 0;
}

static grub_err_t
grub_cmd_ls (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;
  int i;

  if (argc == 0)
    grub_ls_list_devices (state[0].set);
  else
    for (i = 0; i < argc; i++)
      grub_ls_list_files (args[i], state[0].set, state[2].set,
			  state[1].set);

  return 0;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(ls)
{
  cmd = grub_register_extcmd ("ls", grub_cmd_ls, 0,
			      N_("[-l|-h|-a] [FILE ...]"),
			      N_("List devices and files."), options);
}

GRUB_MOD_FINI(ls)
{
  grub_unregister_extcmd (cmd);
}
