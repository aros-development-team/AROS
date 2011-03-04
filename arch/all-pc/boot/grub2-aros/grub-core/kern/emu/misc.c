/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <config-util.h>
#include <config.h>

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <grub/mm.h>
#include <grub/err.h>
#include <grub/env.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/i18n.h>
#include <grub/time.h>
#include <grub/emu/misc.h>

#ifdef HAVE_DEVICE_MAPPER
# include <libdevmapper.h>
#endif

#ifdef HAVE_LIBZFS
# include <grub/util/libzfs.h>
#endif

#ifdef HAVE_LIBNVPAIR
# include <grub/util/libnvpair.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif

#ifdef HAVE_SYS_MNTTAB_H
# include <stdio.h> /* Needed by sys/mnttab.h.  */
# include <sys/mnttab.h>
#endif

#ifdef HAVE_SYS_MKDEV_H
# include <sys/mkdev.h> /* makedev */
#endif

int verbosity;

void
grub_util_warn (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, _("%s: warn:"), program_name);
  fprintf (stderr, " ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, ".\n");
  fflush (stderr);
}

void
grub_util_info (const char *fmt, ...)
{
  if (verbosity > 0)
    {
      va_list ap;

      fprintf (stderr, _("%s: info:"), program_name);
      fprintf (stderr, " ");
      va_start (ap, fmt);
      vfprintf (stderr, fmt, ap);
      va_end (ap);
      fprintf (stderr, ".\n");
      fflush (stderr);
    }
}

void
grub_util_error (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, _("%s: error:"), program_name);
  fprintf (stderr, " ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, ".\n");
  exit (1);
}

void *
xmalloc (grub_size_t size)
{
  void *p;

  p = malloc (size);
  if (! p)
    grub_util_error ("out of memory");

  return p;
}

void *
xrealloc (void *ptr, grub_size_t size)
{
  ptr = realloc (ptr, size);
  if (! ptr)
    grub_util_error ("out of memory");

  return ptr;
}

char *
xstrdup (const char *str)
{
  size_t len;
  char *newstr;

  len = strlen (str);
  newstr = (char *) xmalloc (len + 1);
  memcpy (newstr, str, len + 1);

  return newstr;
}

#ifndef HAVE_VASPRINTF

int
vasprintf (char **buf, const char *fmt, va_list ap)
{
  /* Should be large enough.  */
  *buf = xmalloc (512);

  return vsnprintf (*buf, 512, fmt, ap);
}

#endif

#ifndef  HAVE_ASPRINTF

int
asprintf (char **buf, const char *fmt, ...)
{
  int status;
  va_list ap;

  va_start (ap, fmt);
  status = vasprintf (buf, fmt, ap);
  va_end (ap);

  return status;
}

#endif

char *
xasprintf (const char *fmt, ...)
{ 
  va_list ap;
  char *result;
  
  va_start (ap, fmt);
  if (vasprintf (&result, fmt, ap) < 0)
    { 
      if (errno == ENOMEM)
        grub_util_error ("out of memory");
      return NULL;
    }
  
  return result;
}

void
grub_exit (void)
{
  exit (1);
}

grub_uint64_t
grub_get_time_ms (void)
{
  struct timeval tv;

  gettimeofday (&tv, 0);

  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

grub_uint32_t
grub_get_rtc (void)
{
  struct timeval tv;

  gettimeofday (&tv, 0);

  return (tv.tv_sec * GRUB_TICKS_PER_SECOND
	  + (((tv.tv_sec % GRUB_TICKS_PER_SECOND) * 1000000 + tv.tv_usec)
	     * GRUB_TICKS_PER_SECOND / 1000000));
}

char *
canonicalize_file_name (const char *path)
{
  char *ret;
#ifdef PATH_MAX
  ret = xmalloc (PATH_MAX);
  if (!realpath (path, ret))
    return NULL;
#else
  ret = realpath (path, NULL);
#endif
  return ret;
}

#ifdef __CYGWIN__
/* Convert POSIX path to Win32 path,
   remove drive letter, replace backslashes.  */
static char *
get_win32_path (const char *path)
{
  char winpath[PATH_MAX];
  if (cygwin_conv_path (CCP_POSIX_TO_WIN_A, path, winpath, sizeof(winpath)))
    grub_util_error ("cygwin_conv_path() failed");

  int len = strlen (winpath);
  int offs = (len > 2 && winpath[1] == ':' ? 2 : 0);

  int i;
  for (i = offs; i < len; i++)
    if (winpath[i] == '\\')
      winpath[i] = '/';
  return xstrdup (winpath + offs);
}
#endif

#ifdef HAVE_LIBZFS
static libzfs_handle_t *__libzfs_handle;

static void
fini_libzfs (void)
{
  libzfs_fini (__libzfs_handle);
}

libzfs_handle_t *
grub_get_libzfs_handle (void)
{
  if (! __libzfs_handle)
    {
      __libzfs_handle = libzfs_init ();

      if (__libzfs_handle)
	atexit (fini_libzfs);
    }

  return __libzfs_handle;
}
#endif /* HAVE_LIBZFS */

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
/* ZFS has similar problems to those of btrfs (see above).  */
void
grub_find_zpool_from_dir (const char *dir, char **poolname, char **poolfs)
{
  char *slash;

  *poolname = *poolfs = NULL;

#if defined(HAVE_STRUCT_STATFS_F_FSTYPENAME) && defined(HAVE_STRUCT_STATFS_F_MNTFROMNAME)
  /* FreeBSD and GNU/kFreeBSD.  */
  {
    struct statfs mnt;

    if (statfs (dir, &mnt) != 0)
      return;

    if (strcmp (mnt.f_fstypename, "zfs") != 0)
      return;

    *poolname = xstrdup (mnt.f_mntfromname);
  }
#elif defined(HAVE_GETEXTMNTENT)
  /* Solaris.  */
  {
    struct stat st;
    struct extmnttab mnt;

    if (stat (dir, &st) != 0)
      return;

    FILE *mnttab = fopen ("/etc/mnttab", "r");
    if (! mnttab)
      return;

    while (getextmntent (mnttab, &mnt, sizeof (mnt)) == 0)
      {
	if (makedev (mnt.mnt_major, mnt.mnt_minor) == st.st_dev
	    && !strcmp (mnt.mnt_fstype, "zfs"))
	  {
	    *poolname = xstrdup (mnt.mnt_special);
	    break;
	  }
      }

    fclose (mnttab);
  }
#endif

  if (! *poolname)
    return;

  slash = strchr (*poolname, '/');
  if (slash)
    {
      *slash = '\0';
      *poolfs = xstrdup (slash + 1);
    }
  else
    *poolfs = xstrdup ("");
}
#endif

/* This function never prints trailing slashes (so that its output
   can be appended a slash unconditionally).  */
char *
grub_make_system_path_relative_to_its_root (const char *path)
{
  struct stat st;
  char *p, *buf, *buf2, *buf3, *ret;
  uintptr_t offset = 0;
  dev_t num;
  size_t len;

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
  char *poolfs = NULL;
#endif

  /* canonicalize.  */
  p = canonicalize_file_name (path);
  if (p == NULL)
    grub_util_error ("failed to get canonical path of %s", path);

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
  /* For ZFS sub-pool filesystems, could be extended to others (btrfs?).  */
  {
    char *dummy;
    grub_find_zpool_from_dir (p, &dummy, &poolfs);
  }
#endif

  len = strlen (p) + 1;
  buf = xstrdup (p);
  free (p);

  if (stat (buf, &st) < 0)
    grub_util_error ("cannot stat %s: %s", buf, strerror (errno));

  buf2 = xstrdup (buf);
  num = st.st_dev;

  /* This loop sets offset to the number of chars of the root
     directory we're inspecting.  */
  while (1)
    {
      p = strrchr (buf, '/');
      if (p == NULL)
	/* This should never happen.  */
	grub_util_error ("FIXME: no / in buf. (make_system_path_relative_to_its_root)");
      if (p != buf)
	*p = 0;
      else
	*++p = 0;

      if (stat (buf, &st) < 0)
	grub_util_error ("cannot stat %s: %s", buf, strerror (errno));

      /* buf is another filesystem; we found it.  */
      if (st.st_dev != num)
	{
	  /* offset == 0 means path given is the mount point.
	     This works around special-casing of "/" in Un*x.  This function never
	     prints trailing slashes (so that its output can be appended a slash
	     unconditionally).  Each slash in is considered a preceding slash, and
	     therefore the root directory is an empty string.  */
	  if (offset == 0)
	    {
	      free (buf);
	      free (buf2);
#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
	      if (poolfs)
		return xasprintf ("/%s/@", poolfs);
#endif
	      return xstrdup ("");
	    }
	  else
	    break;
	}

      offset = p - buf;
      /* offset == 1 means root directory.  */
      if (offset == 1)
	{
	  /* Include leading slash.  */
	  offset = 0;
	  break;
	}
    }
  free (buf);
  buf3 = xstrdup (buf2 + offset);
  free (buf2);

#ifdef __CYGWIN__
  if (st.st_dev != (DEV_CYGDRIVE_MAJOR << 16))
    {
      /* Reached some mount point not below /cygdrive.
	 GRUB does not know Cygwin's emulated mounts,
	 convert to Win32 path.  */
      grub_util_info ("Cygwin path = %s\n", buf3);
      char * temp = get_win32_path (buf3);
      free (buf3);
      buf3 = temp;
    }
#endif

  /* Remove trailing slashes, return empty string if root directory.  */
  len = strlen (buf3);
  while (len > 0 && buf3[len - 1] == '/')
    {
      buf3[len - 1] = '\0';
      len--;
    }

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
  if (poolfs)
    {
      ret = xasprintf ("/%s/@%s", poolfs, buf3);
      free (buf3);
    }
  else
#endif
    ret = buf3;

  return ret;
}

#ifdef HAVE_DEVICE_MAPPER
static void device_mapper_null_log (int level __attribute__ ((unused)),
				    const char *file __attribute__ ((unused)),
				    int line __attribute__ ((unused)),
				    int dm_errno __attribute__ ((unused)),
				    const char *f __attribute__ ((unused)),
				    ...)
{
}

int
grub_device_mapper_supported (void)
{
  static int supported = -1;

  if (supported == -1)
    {
      struct dm_task *dmt;

      /* Suppress annoying log messages.  */
      dm_log_with_errno_init (&device_mapper_null_log);

      dmt = dm_task_create (DM_DEVICE_VERSION);
      supported = (dmt != NULL);
      if (dmt)
	dm_task_destroy (dmt);

      /* Restore the original logger.  */
      dm_log_with_errno_init (NULL);
    }

  return supported;
}
#endif /* HAVE_DEVICE_MAPPER */
