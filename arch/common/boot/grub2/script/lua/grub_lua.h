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

#ifndef GRUB_LUA_HEADER
#define GRUB_LUA_HEADER 1

#include <grub/types.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/setjmp.h>

#define INT_MAX		GRUB_LONG_MAX
#define UCHAR_MAX	255
#define SHRT_MAX	32767

#undef UNUSED
#define UNUSED		(void)

#define memcpy		grub_memcpy
#define memcmp		grub_memcmp
#define strcpy		grub_strcpy
#define strstr		grub_strstr
#define strchr		grub_strchr
#define strlen		grub_strlen
#define strtoul		grub_strtoul
#define strtod(s,e)	grub_strtoul(s,e,0)
#define sprintf		grub_sprintf
#define strncpy		grub_strncpy
#define strcat		grub_strcat
#define strncat		grub_strncat
#define strcoll		grub_strcmp
#define strcmp		grub_strcmp
#define tolower		grub_tolower
#define toupper		grub_toupper

#define malloc		grub_malloc
#define realloc		grub_realloc
#define free		grub_free

#define exit(a)		grub_exit()
#define jmp_buf		grub_jmp_buf
#define setjmp		grub_setjmp
#define longjmp		grub_longjmp

#define fputs(s,f)	grub_printf("%s", s)

#define isdigit		grub_isdigit
#define isalpha		grub_isalpha
#define isspace		grub_isspace

static inline int
isalnum (int c)
{
  return (isalpha (c) || isdigit (c));
}

static inline int
iscntrl (int c)
{
  return ((c <= 0x1f) || (c == 0x7f));
}

static inline int
isupper (int c)
{
  return ((c >= 'A') && (c <= 'Z'));
}

static inline int
islower (int c)
{
  return ((c >= 'a') && (c <= 'z'));
}

static inline int
ispunct (int c)
{
  return ((! isspace (c)) && (! isalnum (c)));
}

static inline int
isxdigit (int c)
{
  return (isdigit (c) || ((c >= 'a') && (c <= 'f')) ||
	  ((c >= 'A') && (c <= 'F')));
}

static inline int
abs (int c)
{
  return (c >= 0) ? : -c;
}

int strcspn (const char *s1, const char *s2);
char *strpbrk (const char *s1, const char *s2);
void *memchr (const void *s, int c, size_t n);

#endif
