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

  fprintf (stderr, _("%s: warning:"), program_name);
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
    grub_util_error ("%s", _("out of memory"));

  return p;
}

void *
xrealloc (void *ptr, grub_size_t size)
{
  ptr = realloc (ptr, size);
  if (! ptr)
    grub_util_error ("%s", _("out of memory"));

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
        grub_util_error ("%s", _("out of memory"));
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
#ifdef __MINGW32__
  ret = xmalloc (PATH_MAX);
  if (!_fullpath (ret, path, PATH_MAX))
    return NULL;
#elif defined (PATH_MAX)
  ret = xmalloc (PATH_MAX);
  if (!realpath (path, ret))
    return NULL;
#else
  ret = realpath (path, NULL);
#endif
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
