/* getroot.c - Get root device */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2006,2007,2008  Free Software Foundation, Inc.
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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include <grub/util/misc.h>
#include <grub/util/biosdisk.h>
#include <grub/util/getroot.h>

static void
strip_extra_slashes (char *dir)
{
  char *p = dir;

  while ((p = strchr (p, '/')) != 0)
    {
      if (p[1] == '/')
	{
	  memmove (p, p + 1, strlen (p));
	  continue;
	}
      else if (p[1] == '\0')
	{
	  p[0] = '\0';
	  break;
	}
      
      p++;
    }
}

static char *
xgetcwd (void)
{
  size_t size = 10;
  char *path;

  path = xmalloc (size);
  while (! getcwd (path, size))
    {
      size <<= 1;
      path = xrealloc (path, size);
    }

  return path;
}

char *
grub_get_prefix (const char *dir)
{
  char *saved_cwd;
  char *abs_dir, *prev_dir;
  char *prefix;
  struct stat st, prev_st;
  
  /* Save the current directory.  */
  saved_cwd = xgetcwd ();

  if (chdir (dir) < 0)
    grub_util_error ("Cannot change directory to `%s'", dir);

  abs_dir = xgetcwd ();
  strip_extra_slashes (abs_dir);
  prev_dir = xstrdup (abs_dir);
  
  if (stat (".", &prev_st) < 0)
    grub_util_error ("Cannot stat `%s'", dir);

  if (! S_ISDIR (prev_st.st_mode))
    grub_util_error ("`%s' is not a directory", dir);

  while (1)
    {
      if (chdir ("..") < 0)
	grub_util_error ("Cannot change directory to the parent");

      if (stat (".", &st) < 0)
	grub_util_error ("Cannot stat current directory");

      if (! S_ISDIR (st.st_mode))
	grub_util_error ("Current directory is not a directory???");

      if (prev_st.st_dev != st.st_dev || prev_st.st_ino == st.st_ino)
	break;

      free (prev_dir);
      prev_dir = xgetcwd ();
      prev_st = st;
    }

  strip_extra_slashes (prev_dir);
  prefix = xmalloc (strlen (abs_dir) - strlen (prev_dir) + 2);
  prefix[0] = '/';
  strcpy (prefix + 1, abs_dir + strlen (prev_dir));
  strip_extra_slashes (prefix);

  if (chdir (saved_cwd) < 0)
    grub_util_error ("Cannot change directory to `%s'", dir);

  free (saved_cwd);
  free (abs_dir);
  free (prev_dir);

  grub_util_info ("prefix = %s", prefix);
  return prefix;
}

static char *
find_root_device (const char *dir, dev_t dev)
{
  DIR *dp;
  char *saved_cwd;
  struct dirent *ent;
  
  dp = opendir (dir);
  if (! dp)
    return 0;

  saved_cwd = xgetcwd ();

  grub_util_info ("changing current directory to %s", dir);
  if (chdir (dir) < 0)
    {
      free (saved_cwd);
      closedir (dp);
      return 0;
    }
  
  while ((ent = readdir (dp)) != 0)
    {
      struct stat st;
      
      if (strcmp (ent->d_name, ".") == 0 || strcmp (ent->d_name, "..") == 0)
	continue;

      if (lstat (ent->d_name, &st) < 0)
	/* Ignore any error.  */
	continue;

      if (S_ISLNK (st.st_mode))
	/* Don't follow symbolic links.  */
	continue;
      
      if (S_ISDIR (st.st_mode) && ent->d_name[0] != '.')
	{
	  /* Find it recursively, but avoid dotdirs (like ".static") since they
	     could contain duplicates, which would later break the
	     pathname-based check */
	  char *res;

	  res = find_root_device (ent->d_name, dev);

	  if (res)
	    {
	      if (chdir (saved_cwd) < 0)
		grub_util_error ("Cannot restore the original directory");
	      
	      free (saved_cwd);
	      closedir (dp);
	      return res;
	    }
	}

      if (S_ISBLK (st.st_mode) && st.st_rdev == dev)
	{
	  /* Found!  */
	  char *res;
	  char *cwd;

	  cwd = xgetcwd ();
	  res = xmalloc (strlen (cwd) + strlen (ent->d_name) + 2);
	  sprintf (res, "%s/%s", cwd, ent->d_name);
	  strip_extra_slashes (res);
	  free (cwd);

	  /* /dev/root is not a real block device keep looking, takes care
	     of situation where root filesystem is on the same partition as
	     grub files */

	  if (strcmp(res, "/dev/root") == 0)
		continue;

	  if (chdir (saved_cwd) < 0)
	    grub_util_error ("Cannot restore the original directory");

	  free (saved_cwd);
	  closedir (dp);
	  return res;
	}
    }

  if (chdir (saved_cwd) < 0)
    grub_util_error ("Cannot restore the original directory");

  free (saved_cwd);
  closedir (dp);
  return 0;
}

char *
grub_guess_root_device (const char *dir)
{
  struct stat st;
  char *os_dev;
  
  if (stat (dir, &st) < 0)
    grub_util_error ("Cannot stat `%s'", dir);

#ifdef __linux__
  /* We first try to find the device in the /dev/mapper directory.  If
     we don't do this, we get useless device names like /dev/dm-0 for
     LVM.  */
  os_dev = find_root_device ("/dev/mapper", st.st_dev);

  /* The same applies to /dev/evms directory (for EVMS volumes).  */
  if (! os_dev)
    os_dev = find_root_device ("/dev/evms", st.st_dev);

  if (! os_dev)
#endif
    {
      /* This might be truly slow, but is there any better way?  */
      os_dev = find_root_device ("/dev", st.st_dev);
    }

  return os_dev;
}

int
grub_util_get_dev_abstraction (const char *os_dev)
{
  /* Check for LVM.  */
  if (!strncmp (os_dev, "/dev/mapper/", 12))
    return GRUB_DEV_ABSTRACTION_LVM;

  /* Check for RAID.  */
  if (!strncmp (os_dev, "/dev/md", 7))
    return GRUB_DEV_ABSTRACTION_RAID;

  /* No abstraction found.  */
  return GRUB_DEV_ABSTRACTION_NONE;
}

char *
grub_util_get_grub_dev (const char *os_dev)
{
  char *grub_dev;

  switch (grub_util_get_dev_abstraction (os_dev))
    {
    case GRUB_DEV_ABSTRACTION_LVM:
      grub_dev = xmalloc (strlen (os_dev) - 12 + 1);

      strcpy (grub_dev, os_dev + 12);

      break;

    case GRUB_DEV_ABSTRACTION_RAID:
      grub_dev = xmalloc (20);

      if (os_dev[7] == '_' && os_dev[8] == 'd')
	{
	  const char *p;

	  /* This a partitionable RAID device of the form /dev/md_dNNpMM. */
	  int i;

	  grub_dev[0] = 'm';
	  grub_dev[1] = 'd';
	  i = 2;
	  
	  p = os_dev + 9;
	  while (*p >= '0' && *p <= '9')
	    {
	      grub_dev[i] = *p;
	      i++;
	      p++;
	    }

	  if (*p == '\0')
	    grub_dev[i] = '\0';
	  else if (*p == 'p')
	    {
	      p++;
	      grub_dev[i] = ',';
	      i++;

	      while (*p >= '0' && *p <= '9')
		{
		  grub_dev[i] = *p;
		  i++;
		  p++;
		}

	      grub_dev[i] = '\0';
	    }
	  else
	    grub_util_error ("Unknown kind of RAID device `%s'", os_dev);
	}
      else if (os_dev[7] >= '0' && os_dev[7] <= '9')
	{
	  memcpy (grub_dev, os_dev + 5, 7);
	  grub_dev[7] = '\0';
	}
      else
	grub_util_error ("Unknown kind of RAID device `%s'", os_dev);

      break;

    default:  /* GRUB_DEV_ABSTRACTION_NONE */
      grub_dev = grub_util_biosdisk_get_grub_dev (os_dev);
    }

  return grub_dev;
}

const char *
grub_util_check_block_device (const char *blk_dev)
{
  struct stat st;

  if (stat (blk_dev, &st) < 0)
    grub_util_error ("Cannot stat `%s'", blk_dev);

  if (S_ISBLK (st.st_mode))
    return (blk_dev);
  else
    return 0;
}
