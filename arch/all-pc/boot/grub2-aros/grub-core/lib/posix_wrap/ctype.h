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

#ifndef GRUB_POSIX_CTYPE_H
#define GRUB_POSIX_CTYPE_H	1

#include <grub/misc.h>

static inline int
toupper (int c)
{
  return grub_toupper (c);
}

static inline int 
isspace (int c)
{
  return grub_isspace (c);
}

static inline int 
isdigit (int c)
{
  return grub_isdigit (c);
}

static inline int
islower (int c)
{
  return (c >= 'a' && c <= 'z');
}

static inline int
isupper (int c)
{
  return (c >= 'A' && c <= 'Z');
}

static inline int
isxdigit (int c)
{
  return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')
    || (c >= '0' && c <= '9');
}

static inline int 
isprint (int c)
{
  return grub_isprint (c);
}

static inline int 
iscntrl (int c)
{
  return !grub_isprint (c);
}

static inline int 
isgraph (int c)
{
  return grub_isprint (c) && !grub_isspace (c);
}

static inline int
isalnum (int c)
{
  return grub_isalpha (c) || grub_isdigit (c);
}

static inline int 
ispunct (int c)
{
  return grub_isprint (c) && !grub_isspace (c) && !isalnum (c);
}

static inline int 
isalpha (int c)
{
  return grub_isalpha (c);
}

static inline int
tolower (int c)
{
  return grub_tolower (c);
}

#endif
