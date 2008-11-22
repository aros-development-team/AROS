/* misc.h - prototypes for misc functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2006,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_MISC_HEADER
#define GRUB_MISC_HEADER	1

#include <stdarg.h>
#include <grub/types.h>
#include <grub/symbol.h>
#include <grub/err.h>

#define ALIGN_UP(addr, align) (((grub_uint64_t)addr + align - 1) & ~(align - 1))

#define grub_dprintf(condition, fmt, args...) grub_real_dprintf(__FILE__, __LINE__, condition, fmt, ## args);
/* XXX: If grub_memmove is too slow, we must implement grub_memcpy.  */
#define grub_memcpy(d,s,n)	grub_memmove ((d), (s), (n))

void *EXPORT_FUNC(grub_memmove) (void *dest, const void *src, grub_size_t n);
char *EXPORT_FUNC(grub_strcpy) (char *dest, const char *src);
char *EXPORT_FUNC(grub_strncpy) (char *dest, const char *src, int c);
char *EXPORT_FUNC(grub_stpcpy) (char *dest, const char *src);
char *EXPORT_FUNC(grub_strcat) (char *dest, const char *src);
char *EXPORT_FUNC(grub_strncat) (char *dest, const char *src, int c);

/* Prototypes for aliases.  */
void *EXPORT_FUNC(memmove) (void *dest, const void *src, grub_size_t n);
void *EXPORT_FUNC(memcpy) (void *dest, const void *src, grub_size_t n);

int EXPORT_FUNC(grub_memcmp) (const void *s1, const void *s2, grub_size_t n);
int EXPORT_FUNC(grub_strcmp) (const char *s1, const char *s2);
int EXPORT_FUNC(grub_strncmp) (const char *s1, const char *s2, grub_size_t n);
int EXPORT_FUNC(grub_strncasecmp) (const char *s1, const char *s2, int c);
char *EXPORT_FUNC(grub_strchr) (const char *s, int c);
char *EXPORT_FUNC(grub_strrchr) (const char *s, int c);
int EXPORT_FUNC(grub_strword) (const char *s, const char *w);
char *EXPORT_FUNC(grub_strstr) (const char *haystack, const char *needle);
int EXPORT_FUNC(grub_iswordseparator) (int c);
int EXPORT_FUNC(grub_isspace) (int c);
int EXPORT_FUNC(grub_isprint) (int c);
int EXPORT_FUNC(grub_isalpha) (int c);
int EXPORT_FUNC(grub_isgraph) (int c);
int EXPORT_FUNC(grub_isdigit) (int c);
int EXPORT_FUNC(grub_tolower) (int c);
unsigned long EXPORT_FUNC(grub_strtoul) (const char *str, char **end, int base);
unsigned long long EXPORT_FUNC(grub_strtoull) (const char *str, char **end, int base);
char *EXPORT_FUNC(grub_strdup) (const char *s);
char *EXPORT_FUNC(grub_strndup) (const char *s, grub_size_t n);
void *EXPORT_FUNC(grub_memset) (void *s, int c, grub_size_t n);
grub_size_t EXPORT_FUNC(grub_strlen) (const char *s);
int EXPORT_FUNC(grub_printf) (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void EXPORT_FUNC(grub_real_dprintf) (const char *file,
                                     const int line,
                                     const char *condition,
                                     const char *fmt, ...) __attribute__ ((format (printf, 4, 5)));
int EXPORT_FUNC(grub_vprintf) (const char *fmt, va_list args);
int EXPORT_FUNC(grub_sprintf) (char *str, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
int EXPORT_FUNC(grub_vsprintf) (char *str, const char *fmt, va_list args);
void EXPORT_FUNC(grub_exit) (void) __attribute__ ((noreturn));
void EXPORT_FUNC(grub_abort) (void) __attribute__ ((noreturn));
grub_uint8_t *EXPORT_FUNC(grub_utf16_to_utf8) (grub_uint8_t *dest,
					       grub_uint16_t *src,
					       grub_size_t size);
grub_ssize_t EXPORT_FUNC(grub_utf8_to_ucs4) (grub_uint32_t *dest,
					     const grub_uint8_t *src,
					     grub_size_t size);
grub_uint64_t EXPORT_FUNC(grub_divmod64) (grub_uint64_t n,
					  grub_uint32_t d, grub_uint32_t *r);

#ifdef NEED_ENABLE_EXECUTE_STACK
void EXPORT_FUNC(__enable_execute_stack) (void *addr);
#endif

/* Inline functions.  */

static inline unsigned int
grub_abs (int x)
{
  if (x < 0)
    return (unsigned int) (-x);
  else
    return (unsigned int) x;
}

static inline long
grub_max (long x, long y)
{
  if (x > y)
    return x;
  else
    return y;
}

/* Rounded-up division */
static inline unsigned int
grub_div_roundup (unsigned int x, unsigned int y)
{
  return (x + y - 1) / y;
}

#endif /* ! GRUB_MISC_HEADER */
