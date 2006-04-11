/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <malloc.h>
#include <unistd.h>

#include <grub/util/misc.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/machine/time.h>

char *progname = 0;
int verbosity = 0;

void
grub_util_info (const char *fmt, ...)
{
  if (verbosity > 0)
    {
      va_list ap;
      
      fprintf (stderr, "%s: info: ", progname);
      va_start (ap, fmt);
      vfprintf (stderr, fmt, ap);
      va_end (ap);
      fputc ('\n', stderr);
    }
}

void
grub_util_error (const char *fmt, ...)
{
  va_list ap;
  
  fprintf (stderr, "%s: error: ", progname);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

void *
xmalloc (size_t size)
{
  void *p;
  
  p = malloc (size);
  if (! p)
    grub_util_error ("out of memory");

  return p;
}

void *
xrealloc (void *ptr, size_t size)
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
  char *dup;
  
  len = strlen (str);
  dup = (char *) xmalloc (len + 1);
  memcpy (dup, str, len + 1);

  return dup;
}

char *
grub_util_get_path (const char *dir, const char *file)
{
  char *path;
  
  path = (char *) xmalloc (strlen (dir) + 1 + strlen (file) + 1);
  sprintf (path, "%s/%s", dir, file);
  return path;
}

size_t
grub_util_get_fp_size (FILE *fp)
{
  struct stat st;
  
  if (fflush (fp) == EOF)
    grub_util_error ("fflush failed");

  if (fstat (fileno (fp), &st) == -1)
    grub_util_error ("fstat failed");
  
  return st.st_size;
}

size_t
grub_util_get_image_size (const char *path)
{
  struct stat st;
  
  grub_util_info ("getting the size of %s", path);
  
  if (stat (path, &st) == -1)
    grub_util_error ("cannot stat %s", path);
  
  return st.st_size;
}

void
grub_util_read_at (void *img, size_t size, off_t offset, FILE *fp)
{
  if (fseek (fp, offset, SEEK_SET) == -1)
    grub_util_error ("fseek failed");

  if (fread (img, 1, size, fp) != size)
    grub_util_error ("read failed");
}

char *
grub_util_read_image (const char *path)
{
  char *img;
  FILE *fp;
  size_t size;
  
  grub_util_info ("reading %s", path);

  size = grub_util_get_image_size (path);
  img = (char *) xmalloc (size);

  fp = fopen (path, "rb");
  if (! fp)
    grub_util_error ("cannot open %s", path);

  grub_util_read_at (img, size, 0, fp);

  fclose (fp);
  
  return img;
}

void
grub_util_load_image (const char *path, char *buf)
{
  FILE *fp;
  size_t size;
  
  grub_util_info ("reading %s", path);

  size = grub_util_get_image_size (path);
  
  fp = fopen (path, "rb");
  if (! fp)
    grub_util_error ("cannot open %s", path);

  if (fread (buf, 1, size, fp) != size)
    grub_util_error ("cannot read %s", path);

  fclose (fp);
}

void
grub_util_write_image_at (const void *img, size_t size, off_t offset, FILE *out)
{
  grub_util_info ("writing 0x%x bytes at offset 0x%x", size, offset);
  if (fseek (out, offset, SEEK_SET) == -1)
    grub_util_error ("seek failed");
  if (fwrite (img, 1, size, out) != size)
    grub_util_error ("write failed");
}

void
grub_util_write_image (const char *img, size_t size, FILE *out)
{
  grub_util_info ("writing 0x%x bytes", size);
  if (fwrite (img, 1, size, out) != size)
    grub_util_error ("write failed");
}

void *
grub_malloc (grub_size_t size)
{
  return xmalloc (size);
}

void
grub_free (void *ptr)
{
  free (ptr);
}

void *
grub_realloc (void *ptr, grub_size_t size)
{
  return xrealloc (ptr, size);
}

void *
grub_memalign (grub_size_t align, grub_size_t size)
{
  void *p;
  
  p = memalign (align, size);
  if (! p)
    grub_util_error ("out of memory");
  
  return p;
}

/* Some functions that we don't use.  */
void
grub_mm_init_region (void *addr __attribute__ ((unused)),
		     grub_size_t size __attribute__ ((unused)))
{
}

void
grub_register_exported_symbols (void)
{
}

void
grub_stop (void)
{
  exit (1);
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

void 
grub_arch_sync_caches (void *address __attribute__ ((unused)),
		       grub_size_t len __attribute__ ((unused)))
{
}
