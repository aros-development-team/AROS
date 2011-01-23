/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009, 2010  Free Software Foundation, Inc.
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

#ifndef GRUB_POSIX_STRING_H
#define GRUB_POSIX_STRING_H	1

#include <grub/misc.h>

static inline grub_size_t
strlen (const char *s)
{
  return grub_strlen (s);
}

static inline int 
strcmp (const char *s1, const char *s2)
{
  return grub_strcmp (s1, s2);
}

static inline int 
strcasecmp (const char *s1, const char *s2)
{
  return grub_strcasecmp (s1, s2);
}

#define memcpy grub_memcpy

#if 0
static inline void
memcpy (void *dest, const void *src, grub_size_t n)
{
    grub_memcpy (dest. src, n);
}
#endif

#endif
