/* err.c - error handling routines */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/misc.h>
#include <stdarg.h>

#define GRUB_MAX_ERRMSG	256

grub_err_t grub_errno;
char grub_errmsg[GRUB_MAX_ERRMSG];

grub_err_t
grub_error (grub_err_t n, const char *fmt, ...)
{
  va_list ap;
  
  grub_errno = n;

  va_start (ap, fmt);
  grub_vsprintf (grub_errmsg, fmt, ap);
  va_end (ap);

  return n;
}

void
grub_fatal (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  grub_vprintf (fmt, ap);
  va_end (ap);

  grub_stop ();
}

void
grub_print_error (void)
{
  if (grub_errno != GRUB_ERR_NONE)
    grub_printf ("error: %s\n", grub_errmsg);
}
