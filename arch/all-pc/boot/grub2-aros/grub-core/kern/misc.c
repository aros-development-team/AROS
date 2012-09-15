/* misc.c - definitions of misc functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <stdarg.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/i18n.h>

static int
grub_vsnprintf_real (char *str, grub_size_t n, const char *fmt, va_list args);

static int
grub_iswordseparator (int c)
{
  return (grub_isspace (c) || c == ',' || c == ';' || c == '|' || c == '&');
}

/* grub_gettext_dummy is not translating anything.  */
static const char *
grub_gettext_dummy (const char *s)
{
  return s;
}

const char* (*grub_gettext) (const char *s) = grub_gettext_dummy;

void *
grub_memmove (void *dest, const void *src, grub_size_t n)
{
  char *d = (char *) dest;
  const char *s = (const char *) src;

  if (d < s)
    while (n--)
      *d++ = *s++;
  else
    {
      d += n;
      s += n;

      while (n--)
	*--d = *--s;
    }

  return dest;
}

#ifndef __APPLE__
void *memmove (void *dest, const void *src, grub_size_t n)
  __attribute__ ((alias ("grub_memmove")));
/* GCC emits references to memcpy() for struct copies etc.  */
void *memcpy (void *dest, const void *src, grub_size_t n)
  __attribute__ ((alias ("grub_memmove")));
#else
void * __attribute__ ((regparm(0)))
memcpy (void *dest, const void *src, grub_size_t n)
{
	return grub_memmove (dest, src, n);
}
void * __attribute__ ((regparm(0)))
memmove (void *dest, const void *src, grub_size_t n)
{
	return grub_memmove (dest, src, n);
}
#endif

char *
grub_strcpy (char *dest, const char *src)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0')
    ;

  return dest;
}

char *
grub_strncpy (char *dest, const char *src, int c)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0' && --c)
    ;

  return dest;
}

int
grub_printf (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vprintf (fmt, ap);
  va_end (ap);

  return ret;
}

int
grub_printf_ (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vprintf (_(fmt), ap);
  va_end (ap);

  return ret;
}

int
grub_puts_ (const char *s)
{
  return grub_puts (_(s));
}

#if defined (__APPLE__) && ! defined (GRUB_UTIL)
int
grub_err_printf (const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start (ap, fmt);
	ret = grub_vprintf (fmt, ap);
	va_end (ap);

	return ret;
}
#endif

#if ! defined (__APPLE__) && ! defined (GRUB_UTIL)
int grub_err_printf (const char *fmt, ...)
__attribute__ ((alias("grub_printf")));
#endif

void
grub_real_dprintf (const char *file, const int line, const char *condition,
		   const char *fmt, ...)
{
  va_list args;
  const char *debug = grub_env_get ("debug");

  if (! debug)
    return;

  if (grub_strword (debug, "all") || grub_strword (debug, condition))
    {
      grub_printf ("%s:%d: ", file, line);
      va_start (args, fmt);
      grub_vprintf (fmt, args);
      va_end (args);
      grub_refresh ();
    }
}

#define PREALLOC_SIZE 255

int
grub_vprintf (const char *fmt, va_list args)
{
  grub_size_t s;
  static char buf[PREALLOC_SIZE + 1];
  char *curbuf = buf;
  va_list ap2;
  va_copy (ap2, args);

  s = grub_vsnprintf_real (buf, PREALLOC_SIZE, fmt, args);
  if (s > PREALLOC_SIZE)
    {
      curbuf = grub_malloc (s + 1);
      if (!curbuf)
	{
	  grub_errno = GRUB_ERR_NONE;
	  buf[PREALLOC_SIZE - 3] = '.';
	  buf[PREALLOC_SIZE - 2] = '.';
	  buf[PREALLOC_SIZE - 1] = '.';
	  buf[PREALLOC_SIZE] = 0;
	  curbuf = buf;
	}
      else
	s = grub_vsnprintf_real (curbuf, s, fmt, ap2);
    }

  va_end (ap2);

  grub_xputs (curbuf);

  if (curbuf != buf)
    grub_free (curbuf);
  
  return s;
}

int
grub_memcmp (const void *s1, const void *s2, grub_size_t n)
{
  const grub_uint8_t *t1 = s1;
  const grub_uint8_t *t2 = s2;

  while (n--)
    {
      if (*t1 != *t2)
	return (int) *t1 - (int) *t2;

      t1++;
      t2++;
    }

  return 0;
}
#ifndef __APPLE__
int memcmp (const void *s1, const void *s2, grub_size_t n)
  __attribute__ ((alias ("grub_memcmp")));
#else
int __attribute__ ((regparm(0)))
memcmp (const void *s1, const void *s2, grub_size_t n)
{
  return grub_memcmp (s1, s2, n);
}
#endif

int
grub_strcmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (*s1 != *s2)
	break;

      s1++;
      s2++;
    }

  return (int) (grub_uint8_t) *s1 - (int) (grub_uint8_t) *s2;
}

int
grub_strncmp (const char *s1, const char *s2, grub_size_t n)
{
  if (n == 0)
    return 0;

  while (*s1 && *s2 && --n)
    {
      if (*s1 != *s2)
	break;

      s1++;
      s2++;
    }

  return (int) (grub_uint8_t) *s1 - (int) (grub_uint8_t)  *s2;
}

char *
grub_strchr (const char *s, int c)
{
  do
    {
      if (*s == c)
	return (char *) s;
    }
  while (*s++);

  return 0;
}

char *
grub_strrchr (const char *s, int c)
{
  char *p = NULL;

  do
    {
      if (*s == c)
	p = (char *) s;
    }
  while (*s++);

  return p;
}

int
grub_strword (const char *haystack, const char *needle)
{
  const char *n_pos = needle;

  while (grub_iswordseparator (*haystack))
    haystack++;

  while (*haystack)
    {
      /* Crawl both the needle and the haystack word we're on.  */
      while(*haystack && !grub_iswordseparator (*haystack)
            && *haystack == *n_pos)
        {
          haystack++;
          n_pos++;
        }

      /* If we reached the end of both words at the same time, the word
      is found. If not, eat everything in the haystack that isn't the
      next word (or the end of string) and "reset" the needle.  */
      if ( (!*haystack || grub_iswordseparator (*haystack))
         && (!*n_pos || grub_iswordseparator (*n_pos)))
        return 1;
      else
        {
          n_pos = needle;
          while (*haystack && !grub_iswordseparator (*haystack))
            haystack++;
          while (grub_iswordseparator (*haystack))
            haystack++;
        }
    }

  return 0;
}

int
grub_isspace (int c)
{
  return (c == '\n' || c == '\r' || c == ' ' || c == '\t');
}

int
grub_isprint (int c)
{
  return (c >= ' ' && c <= '~');
}


unsigned long
grub_strtoul (const char *str, char **end, int base)
{
  unsigned long long num;

  num = grub_strtoull (str, end, base);
#if GRUB_CPU_SIZEOF_LONG != 8
  if (num > ~0UL)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, N_("overflow is detected"));
      return ~0UL;
    }
#endif

  return (unsigned long) num;
}

unsigned long long
grub_strtoull (const char *str, char **end, int base)
{
  unsigned long long num = 0;
  int found = 0;

  /* Skip white spaces.  */
  while (*str && grub_isspace (*str))
    str++;

  /* Guess the base, if not specified. The prefix `0x' means 16, and
     the prefix `0' means 8.  */
  if (str[0] == '0')
    {
      if (str[1] == 'x')
	{
	  if (base == 0 || base == 16)
	    {
	      base = 16;
	      str += 2;
	    }
	}
      else if (base == 0 && str[1] >= '0' && str[1] <= '7')
	base = 8;
    }

  if (base == 0)
    base = 10;

  while (*str)
    {
      unsigned long digit;

      digit = grub_tolower (*str) - '0';
      if (digit > 9)
	{
	  digit += '0' - 'a' + 10;
	  if (digit >= (unsigned long) base)
	    break;
	}

      found = 1;

      /* NUM * BASE + DIGIT > ~0ULL */
      if (num > grub_divmod64 (~0ULL - digit, base, 0))
	{
	  grub_error (GRUB_ERR_OUT_OF_RANGE,
		      N_("overflow is detected"));
	  return ~0ULL;
	}

      num = num * base + digit;
      str++;
    }

  if (! found)
    {
      grub_error (GRUB_ERR_BAD_NUMBER,
		  N_("unrecognized number"));
      return 0;
    }

  if (end)
    *end = (char *) str;

  return num;
}

char *
grub_strdup (const char *s)
{
  grub_size_t len;
  char *p;

  len = grub_strlen (s) + 1;
  p = (char *) grub_malloc (len);
  if (! p)
    return 0;

  return grub_memcpy (p, s, len);
}

char *
grub_strndup (const char *s, grub_size_t n)
{
  grub_size_t len;
  char *p;

  len = grub_strlen (s);
  if (len > n)
    len = n;
  p = (char *) grub_malloc (len + 1);
  if (! p)
    return 0;

  grub_memcpy (p, s, len);
  p[len] = '\0';
  return p;
}

void *
grub_memset (void *s, int c, grub_size_t len)
{
  void *p = s;
  grub_uint8_t pattern8 = c;

  if (len >= 3 * sizeof (unsigned long))
    {
      unsigned long patternl = 0;
      grub_size_t i;

      for (i = 0; i < sizeof (unsigned long); i++)
	patternl |= ((unsigned long) pattern8) << (8 * i);

      while (len > 0 && (((grub_addr_t) p) & (sizeof (unsigned long) - 1)))
	{
	  *(grub_uint8_t *) p = pattern8;
	  p = (grub_uint8_t *) p + 1;
	  len--;
	}
      while (len >= sizeof (unsigned long))
	{
	  *(unsigned long *) p = patternl;
	  p = (unsigned long *) p + 1;
	  len -= sizeof (unsigned long);
	}
    }

  while (len > 0)
    {
      *(grub_uint8_t *) p = pattern8;
      p = (grub_uint8_t *) p + 1;
      len--;
    }

  return s;
}
#ifndef __APPLE__
void *memset (void *s, int c, grub_size_t n)
  __attribute__ ((alias ("grub_memset")));
#else
void * __attribute__ ((regparm(0)))
memset (void *s, int c, grub_size_t n)
{
  return grub_memset (s, c, n);
}
#endif

grub_size_t
grub_strlen (const char *s)
{
  const char *p = s;

  while (*p)
    p++;

  return p - s;
}

static inline void
grub_reverse (char *str)
{
  char *p = str + grub_strlen (str) - 1;

  while (str < p)
    {
      char tmp;

      tmp = *str;
      *str = *p;
      *p = tmp;
      str++;
      p--;
    }
}

/* Divide N by D, return the quotient, and store the remainder in *R.  */
grub_uint64_t
grub_divmod64 (grub_uint64_t n, grub_uint64_t d, grub_uint64_t *r)
{
  /* This algorithm is typically implemented by hardware. The idea
     is to get the highest bit in N, 64 times, by keeping
     upper(N * 2^i) = (Q * D + M), where upper
     represents the high 64 bits in 128-bits space.  */
  unsigned bits = 64;
  grub_uint64_t q = 0;
  grub_uint64_t m = 0;

  /* Skip the slow computation if 32-bit arithmetic is possible.  */
  if (n < 0xffffffff && d < 0xffffffff)
    {
      if (r)
	*r = ((grub_uint32_t) n) % (grub_uint32_t) d;

      return ((grub_uint32_t) n) / (grub_uint32_t) d;
    }

  while (bits--)
    {
      m <<= 1;

      if (n & (1ULL << 63))
	m |= 1;

      q <<= 1;
      n <<= 1;

      if (m >= d)
	{
	  q |= 1;
	  m -= d;
	}
    }

  if (r)
    *r = m;

  return q;
}

/* Convert a long long value to a string. This function avoids 64-bit
   modular arithmetic or divisions.  */
static char *
grub_lltoa (char *str, int c, unsigned long long n)
{
  unsigned base = (c == 'x') ? 16 : 10;
  char *p;

  if ((long long) n < 0 && c == 'd')
    {
      n = (unsigned long long) (-((long long) n));
      *str++ = '-';
    }

  p = str;

  if (base == 16)
    do
      {
	unsigned d = (unsigned) (n & 0xf);
	*p++ = (d > 9) ? d + 'a' - 10 : d + '0';
      }
    while (n >>= 4);
  else
    /* BASE == 10 */
    do
      {
	grub_uint64_t m;

	n = grub_divmod64 (n, 10, &m);
	*p++ = m + '0';
      }
    while (n);

  *p = 0;

  grub_reverse (str);
  return p;
}

static int
grub_vsnprintf_real (char *str, grub_size_t max_len, const char *fmt0, va_list args_in)
{
  char c;
  grub_size_t n = 0;
  grub_size_t count = 0;
  grub_size_t count_args = 0;
  const char *fmt;
  auto void write_char (unsigned char ch);
  auto void write_str (const char *s);
  auto void write_fill (const char ch, int n);

  void write_char (unsigned char ch)
    {
      if (count < max_len)
	*str++ = ch;

      count++;
    }

  void write_str (const char *s)
    {
      while (*s)
	write_char (*s++);
    }

  void write_fill (const char ch, int count_fill)
    {
      int i;
      for (i = 0; i < count_fill; i++)
	write_char (ch);
    }

  fmt = fmt0;
  while ((c = *fmt++) != 0)
    {
      if (c != '%')
	continue;

      if (*fmt && *fmt =='-')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      if (*fmt && *fmt == '$')
	fmt++;

      if (*fmt && *fmt =='-')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      if (*fmt && *fmt =='.')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      c = *fmt++;
      if (c == 'l')
	{
	  c = *fmt++;
	  if (c == 'l')
	    c = *fmt++;
	}
      switch (c)
	{
	case 'p':
	case 'x':
	case 'u':
	case 'd':
	case 'c':
	case 'C':
	case 's':
	  count_args++;
	  break;
	}
    }

  enum { INT, WCHAR, LONG, LONGLONG, POINTER } types[count_args];
  union 
  { 
    int i;
    grub_uint32_t w;
    long l;
    long long ll;
    void *p;
  } args[count_args];

  grub_memset (types, 0, sizeof (types));

  fmt = fmt0;
  n = 0;
  while ((c = *fmt++) != 0)
    {
      int longfmt = 0;
      int longlongfmt = 0;
      grub_size_t curn;
      const char *p;

      if (c != '%')
	continue;

      curn = n++;

      if (*fmt && *fmt =='-')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      if (*fmt && *fmt =='.')
	fmt++;

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      p = fmt;

      if (*fmt && *fmt == '$')
	{
	  curn = grub_strtoull (p, 0, 10) - 1;
	  fmt++;
	}

      while (*fmt && grub_isdigit (*fmt))
	fmt++;

      c = *fmt++;
      if (c == 'l')
	{
	  c = *fmt++;
	  longfmt = 1;
	  if (c == 'l')
	    {
	      c = *fmt++;
	      longlongfmt = 1;
	    }
	}
      if (curn >= count_args)
	continue;
      switch (c)
	{
	case 'x':
	case 'u':
	case 'd':
	  if (longlongfmt)
	    types[curn] = LONGLONG;
	  else if (longfmt)
	    types[curn] = LONG;
	  else
	    types[curn] = INT;
	  break;
	case 'p':
	case 's':
	  types[curn] = POINTER;
	  break;
	case 'c':
	  types[curn] = INT;
	  break;
	case 'C':
	  types[curn] = WCHAR;
	  break;
	}
    }

  for (n = 0; n < count_args; n++)
    switch (types[n])
      {
      case WCHAR:
	args[n].w = va_arg (args_in, grub_uint32_t);
	break;
      case POINTER:
	args[n].p = va_arg (args_in, void *);
	break;
      case INT:
	args[n].i = va_arg (args_in, int);
	break;
      case LONG:
	args[n].l = va_arg (args_in, long);
	break;
      case LONGLONG:
	args[n].ll = va_arg (args_in, long long);
	break;
      }

  fmt = fmt0;

  n = 0;
  while ((c = *fmt++) != 0)
    {
      char tmp[32];
      char *p;
      unsigned int format1 = 0;
      unsigned int format2 = ~ 0U;
      char zerofill = ' ';
      int rightfill = 0;
      int longfmt = 0;
      int longlongfmt = 0;
      int unsig = 0;
      grub_size_t curn;
      
      if (c != '%')
	{
	  write_char (c);
	  continue;
	}

      curn = n++;

    rescan:;

      if (*fmt && *fmt =='-')
	{
	  rightfill = 1;
	  fmt++;
	}

      p = (char *) fmt;
      /* Read formatting parameters.  */
      while (*p && grub_isdigit (*p))
	p++;

      if (p > fmt)
	{
	  char s[p - fmt + 1];
	  grub_strncpy (s, fmt, p - fmt);
	  s[p - fmt] = 0;
	  if (s[0] == '0')
	    zerofill = '0';
	  format1 = grub_strtoul (s, 0, 10);
	  fmt = p;
	}

      if (*p && *p == '.')
	{
	  p++;
	  fmt++;
	  while (*p && grub_isdigit (*p))
	    p++;

	  if (p > fmt)
	    {
	      char fstr[p - fmt + 1];
	      grub_strncpy (fstr, fmt, p - fmt);
	      fstr[p - fmt] = 0;
	      format2 = grub_strtoul (fstr, 0, 10);
	      fmt = p;
	    }
	}
      if (*fmt == '$')
	{
	  curn = format1 - 1;
	  fmt++;
	  format1 = 0;
	  format2 = ~ 0U;
	  zerofill = ' ';
	  rightfill = 0;

	  goto rescan;
	}

      c = *fmt++;
      if (c == 'l')
	{
	  longfmt = 1;
	  c = *fmt++;
	  if (c == 'l')
	    {
	      longlongfmt = 1;
	      c = *fmt++;
	    }
	}

      if (curn >= count_args)
	continue;

      switch (c)
	{
	case 'p':
	  write_str ("0x");
	  c = 'x';
	  longlongfmt |= (sizeof (void *) == sizeof (long long));
	  /* Fall through. */
	case 'x':
	case 'u':
	  unsig = 1;
	  /* Fall through. */
	case 'd':
	  if (longlongfmt)
	    grub_lltoa (tmp, c, args[curn].ll);
	  else if (longfmt && unsig)
	    grub_lltoa (tmp, c, (unsigned long) args[curn].l);
	  else if (longfmt)
	    grub_lltoa (tmp, c, args[curn].l);
	  else if (unsig)
	    grub_lltoa (tmp, c, (unsigned) args[curn].i);
	  else
	    grub_lltoa (tmp, c, args[curn].i);
	  if (! rightfill && grub_strlen (tmp) < format1)
	    write_fill (zerofill, format1 - grub_strlen (tmp));
	  write_str (tmp);
	  if (rightfill && grub_strlen (tmp) < format1)
	    write_fill (zerofill, format1 - grub_strlen (tmp));
	  break;

	case 'c':
	  write_char (args[curn].i & 0xff);
	  break;

	case 'C':
	  {
	    grub_uint32_t code = args[curn].w;
	    int shift;
	    unsigned mask;

	    if (code <= 0x7f)
	      {
		shift = 0;
		mask = 0;
	      }
	    else if (code <= 0x7ff)
	      {
		shift = 6;
		mask = 0xc0;
	      }
	    else if (code <= 0xffff)
	      {
		shift = 12;
		mask = 0xe0;
	      }
	    else if (code <= 0x1fffff)
	      {
		shift = 18;
		mask = 0xf0;
	      }
	    else if (code <= 0x3ffffff)
	      {
		shift = 24;
		mask = 0xf8;
	      }
	    else if (code <= 0x7fffffff)
	      {
		shift = 30;
		mask = 0xfc;
	      }
	    else
	      {
		code = '?';
		shift = 0;
		mask = 0;
	      }

	    write_char (mask | (code >> shift));

	    for (shift -= 6; shift >= 0; shift -= 6)
	      write_char (0x80 | (0x3f & (code >> shift)));
	  }
	  break;

	case 's':
	  p = args[curn].p;
	  if (p)
	    {
	      grub_size_t len = 0;
	      while (len < format2 && p[len])
		len++;

	      if (!rightfill && len < format1)
		write_fill (zerofill, format1 - len);

	      grub_size_t i;
	      for (i = 0; i < len; i++)
		write_char (*p++);

	      if (rightfill && len < format1)
		write_fill (zerofill, format1 - len);
	    }
	  else
	    write_str ("(null)");

	  break;

	default:
	  write_char (c);
	  break;
	}
    }

  *str = '\0';

  return count;
}

int
grub_vsnprintf (char *str, grub_size_t n, const char *fmt, va_list ap)
{
  grub_size_t ret;

  if (!n)
    return 0;

  n--;

  ret = grub_vsnprintf_real (str, n, fmt, ap);

  return ret < n ? ret : n;
}

int
grub_snprintf (char *str, grub_size_t n, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = grub_vsnprintf (str, n, fmt, ap);
  va_end (ap);

  return ret;
}

char *
grub_xvasprintf (const char *fmt, va_list ap)
{
  grub_size_t s, as = PREALLOC_SIZE;
  char *ret;

  while (1)
    {
      va_list ap2;
      ret = grub_malloc (as + 1);
      if (!ret)
	return NULL;

      va_copy (ap2, ap);

      s = grub_vsnprintf_real (ret, as, fmt, ap2);

      va_end (ap2);

      if (s <= as)
	return ret;

      grub_free (ret);
      as = s;
    }
}

char *
grub_xasprintf (const char *fmt, ...)
{
  va_list ap;
  char *ret;

  va_start (ap, fmt);
  ret = grub_xvasprintf (fmt, ap);
  va_end (ap);

  return ret;
}

/* Abort GRUB. This function does not return.  */
void
grub_abort (void)
{
  grub_printf ("\nAborted.");
  
#ifndef GRUB_UTIL
  if (grub_term_inputs)
#endif
    {
      grub_printf (" Press any key to exit.");
      grub_getkey ();
    }

  grub_exit ();
}

#if ! defined (__APPLE__) && !defined (GRUB_UTIL)
/* GCC emits references to abort().  */
void abort (void) __attribute__ ((alias ("grub_abort")));
#endif

#if NEED_ENABLE_EXECUTE_STACK && !defined(GRUB_UTIL) && !defined(GRUB_MACHINE_EMU)
/* Some gcc versions generate a call to this function
   in trampolines for nested functions.  */
void __enable_execute_stack (void *addr __attribute__ ((unused)))
{
}
#endif

#if NEED_REGISTER_FRAME_INFO && !defined(GRUB_UTIL)
void __register_frame_info (void)
{
}

void __deregister_frame_info (void)
{
}
#endif

