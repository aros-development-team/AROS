/* test.c -- The test command..  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2009  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/fs.h>
#include <grub/device.h>
#include <grub/file.h>
#include <grub/command.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

/* A simple implementation for signed numbers. */
static int
grub_strtosl (char *arg, char **end, int base)
{
  if (arg[0] == '-')
    return -grub_strtoul (arg + 1, end, base);
  return grub_strtoul (arg, end, base);
}

/* Parse a test expression starting from *argn. */
static int
test_parse (char **args, int *argn, int argc)
{
  int ret = 0, discard = 0, invert = 0;
  int file_exists;
  struct grub_dirhook_info file_info;

  auto void update_val (int val);
  auto void get_fileinfo (char *pathname);

  /* Take care of discarding and inverting. */
  void update_val (int val)
  {
    if (! discard)
      ret = invert ? ! val : val;
    invert = discard = 0;
  }

  /* Check if file exists and fetch its information. */
  void get_fileinfo (char *path)
  {
    char *filename, *pathname;
    char *device_name;
    grub_fs_t fs;
    grub_device_t dev;

    /* A hook for iterating directories. */
    auto int find_file (const char *cur_filename,
			const struct grub_dirhook_info *info);
    int find_file (const char *cur_filename,
		   const struct grub_dirhook_info *info)
    {
      if ((info->case_insensitive ? grub_strcasecmp (cur_filename, filename)
	   : grub_strcmp (cur_filename, filename)) == 0)
	{
	  file_info = *info;
	  file_exists = 1;
	  return 1;
	}
      return 0;
    }

    file_exists = 0;
    device_name = grub_file_get_device_name (path);
    dev = grub_device_open (device_name);
    if (! dev)
      {
	grub_free (device_name);
	return;
      }

    fs = grub_fs_probe (dev);
    if (! fs)
      {
	grub_free (device_name);
	grub_device_close (dev);
	return;
      }

    pathname = grub_strchr (path, ')');
    if (! pathname)
      pathname = path;
    else
      pathname++;

    /* Remove trailing '/'. */
    while (*pathname && pathname[grub_strlen (pathname) - 1] == '/')
      pathname[grub_strlen (pathname) - 1] = 0;

    /* Split into path and filename. */
    filename = grub_strrchr (pathname, '/');
    if (! filename)
      {
	path = grub_strdup ("/");
	filename = pathname;
      }
    else
      {
	filename++;
	path = grub_strdup (pathname);
	path[filename - pathname] = 0;
      }

    /* It's the whole device. */
    if (! *pathname)
      {
	file_exists = 1;
	grub_memset (&file_info, 0, sizeof (file_info));
	/* Root is always a directory. */
	file_info.dir = 1;

	/* Fetch writing time. */
	file_info.mtimeset = 0;
	if (fs->mtime)
	  {
	    if (! fs->mtime (dev, &file_info.mtime))
	      file_info.mtimeset = 1;
	    grub_errno = GRUB_ERR_NONE;
	  }
      }
    else
      (fs->dir) (dev, path, find_file);

    grub_device_close (dev);
    grub_free (path);
    grub_free (device_name);
  }

  /* Here we have the real parsing. */
  while (*argn < argc)
    {
      /* First try 3 argument tests. */
      if (*argn + 2 < argc)
	{
	  /* String tests. */
	  if (grub_strcmp (args[*argn + 1], "=") == 0
	      || grub_strcmp (args[*argn + 1], "==") == 0)
	    {
	      update_val (grub_strcmp (args[*argn], args[*argn + 2]) == 0);
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], "!=") == 0)
	    {
	      update_val (grub_strcmp (args[*argn], args[*argn + 2]) != 0);
	      (*argn) += 3;
	      continue;
	    }

	  /* GRUB extension: lexicographical sorting. */
	  if (grub_strcmp (args[*argn + 1], "<") == 0)
	    {
	      update_val (grub_strcmp (args[*argn], args[*argn + 2]) < 0);
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], "<=") == 0)
	    {
	      update_val (grub_strcmp (args[*argn], args[*argn + 2]) <= 0);
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], ">") == 0)
	    {
	      update_val (grub_strcmp (args[*argn], args[*argn + 2]) > 0);
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], ">=") == 0)
	    {
	      update_val (grub_strcmp (args[*argn], args[*argn + 2]) >= 0);
	      (*argn) += 3;
	      continue;
	    }

	  /* Number tests. */
	  if (grub_strcmp (args[*argn + 1], "-eq") == 0)
	    {
	      update_val (grub_strtosl (args[*argn], 0, 0)
			  == grub_strtosl (args[*argn + 2], 0, 0));
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], "-ge") == 0)
	    {
	      update_val (grub_strtosl (args[*argn], 0, 0)
			  >= grub_strtosl (args[*argn + 2], 0, 0));
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], "-gt") == 0)
	    {
	      update_val (grub_strtosl (args[*argn], 0, 0)
			  > grub_strtosl (args[*argn + 2], 0, 0));
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], "-le") == 0)
	    {
	      update_val (grub_strtosl (args[*argn], 0, 0)
		      <= grub_strtosl (args[*argn + 2], 0, 0));
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], "-lt") == 0)
	    {
	      update_val (grub_strtosl (args[*argn], 0, 0)
			  < grub_strtosl (args[*argn + 2], 0, 0));
	      (*argn) += 3;
	      continue;
	    }

	  if (grub_strcmp (args[*argn + 1], "-ne") == 0)
	    {
	      update_val (grub_strtosl (args[*argn], 0, 0)
			  != grub_strtosl (args[*argn + 2], 0, 0));
	      (*argn) += 3;
	      continue;
	    }

	  /* GRUB extension: compare numbers skipping prefixes.
	     Useful for comparing versions. E.g. vmlinuz-2 -plt vmlinuz-11. */
	  if (grub_strcmp (args[*argn + 1], "-pgt") == 0
	      || grub_strcmp (args[*argn + 1], "-plt") == 0)
	    {
	      int i;
	      /* Skip common prefix. */
	      for (i = 0; args[*argn][i] == args[*argn + 2][i]
		     && args[*argn][i]; i++);

	      /* Go the digits back. */
	      i--;
	      while (grub_isdigit (args[*argn][i]) && i > 0)
		i--;
	      i++;

	      if (grub_strcmp (args[*argn + 1], "-pgt") == 0)
		update_val (grub_strtoul (args[*argn] + i, 0, 0)
			    > grub_strtoul (args[*argn + 2] + i, 0, 0));
	      else
		update_val (grub_strtoul (args[*argn] + i, 0, 0)
			    < grub_strtoul (args[*argn + 2] + i, 0, 0));
	      (*argn) += 3;
	      continue;
	    }

	  /* -nt and -ot tests. GRUB extension: when doing -?t<bias> bias
	     will be added to the first mtime. */
	  if (grub_memcmp (args[*argn + 1], "-nt", 3) == 0
	      || grub_memcmp (args[*argn + 1], "-ot", 3) == 0)
	    {
	      struct grub_dirhook_info file1;
	      int file1exists;
	      int bias = 0;

	      /* Fetch fileinfo. */
	      get_fileinfo (args[*argn]);
	      file1 = file_info;
	      file1exists = file_exists;
	      get_fileinfo (args[*argn + 2]);

	      if (args[*argn + 1][3])
		bias = grub_strtosl (args[*argn + 1] + 3, 0, 0);

	      if (grub_memcmp (args[*argn + 1], "-nt", 3) == 0)
		update_val ((file1exists && ! file_exists)
			    || (file1.mtimeset && file_info.mtimeset
				&& file1.mtime + bias > file_info.mtime));
	      else
		update_val ((! file1exists && file_exists)
			    || (file1.mtimeset && file_info.mtimeset
				&& file1.mtime + bias < file_info.mtime));
	      (*argn) += 3;
	      continue;
	    }
	}

      /* Two-argument tests. */
      if (*argn + 1 < argc)
	{
	  /* File tests. */
	  if (grub_strcmp (args[*argn], "-d") == 0)
	    {
	      get_fileinfo (args[*argn + 1]);
	      update_val (file_exists && file_info.dir);
	      (*argn) += 2;
	      return ret;
	    }

	  if (grub_strcmp (args[*argn], "-e") == 0)
	    {
	      get_fileinfo (args[*argn + 1]);
	      update_val (file_exists);
	      (*argn) += 2;
	      return ret;
	    }

	  if (grub_strcmp (args[*argn], "-f") == 0)
	    {
	      get_fileinfo (args[*argn + 1]);
	      /* FIXME: check for other types. */
	      update_val (file_exists && ! file_info.dir);
	      (*argn) += 2;
	      return ret;
	    }

	  if (grub_strcmp (args[*argn], "-s") == 0)
	    {
	      grub_file_t file;
	      grub_file_filter_disable_compression ();
	      file = grub_file_open (args[*argn + 1]);
	      update_val (file && (grub_file_size (file) != 0));
	      if (file)
		grub_file_close (file);
	      grub_errno = GRUB_ERR_NONE;
	      (*argn) += 2;
	      return ret;
	    }

	  /* String tests. */
	  if (grub_strcmp (args[*argn], "-n") == 0)
	    {
	      update_val (args[*argn + 1][0]);

	      (*argn) += 2;
	      continue;
	    }
	  if (grub_strcmp (args[*argn], "-z") == 0)
	    {
	      update_val (! args[*argn + 1][0]);
	      (*argn) += 2;
	      continue;
	    }
	}

      /* Special modifiers. */

      /* End of expression. return to parent. */
      if (grub_strcmp (args[*argn], ")") == 0)
	{
	  (*argn)++;
	  return ret;
	}
      /* Recursively invoke if parenthesis. */
      if (grub_strcmp (args[*argn], "(") == 0)
	{
	  (*argn)++;
	  update_val (test_parse (args, argn, argc));
	  continue;
	}

      if (grub_strcmp (args[*argn], "!") == 0)
	{
	  invert = ! invert;
	  (*argn)++;
	  continue;
	}
      if (grub_strcmp (args[*argn], "-a") == 0)
	{
	  /* If current value is 0 second value is to be discarded. */
	  discard = ! ret;
	  (*argn)++;
	  continue;
	}
      if (grub_strcmp (args[*argn], "-o") == 0)
	{
	  /* If current value is 1 second value is to be discarded. */
	  discard = ret;
	  (*argn)++;
	  continue;
	}

      /* No test found. Interpret if as just a string. */
      update_val (args[*argn][0]);
      (*argn)++;
    }
  return ret;
}

static grub_err_t
grub_cmd_test (grub_command_t cmd __attribute__ ((unused)),
	       int argc, char **args)
{
  int argn = 0;

  if (argc >= 1 && grub_strcmp (args[argc - 1], "]") == 0)
    argc--;

  return test_parse (args, &argn, argc) ? GRUB_ERR_NONE
    : grub_error (GRUB_ERR_TEST_FAILURE, N_("false"));
}

static grub_command_t cmd_1, cmd_2;

GRUB_MOD_INIT(test)
{
  cmd_1 = grub_register_command ("[", grub_cmd_test,
				 N_("EXPRESSION ]"), N_("Evaluate an expression."));
  cmd_1->flags |= GRUB_COMMAND_FLAG_EXTRACTOR;
  cmd_2 = grub_register_command ("test", grub_cmd_test,
				 N_("EXPRESSION"), N_("Evaluate an expression."));
  cmd_2->flags |= GRUB_COMMAND_FLAG_EXTRACTOR;
}

GRUB_MOD_FINI(test)
{
  grub_unregister_command (cmd_1);
  grub_unregister_command (cmd_2);
}
