/* misc.c - definitions of misc functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <stdarg.h>
#include <grub/term.h>
#include <grub/env.h>

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
void *memmove (void *dest, const void *src, grub_size_t n)
  __attribute__ ((alias ("grub_memmove")));
/* GCC emits references to memcpy() for struct copies etc.  */
void *memcpy (void *dest, const void *src, grub_size_t n)
  __attribute__ ((alias ("grub_memmove")));

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

char *
grub_stpcpy (char *dest, const char *src)
{
  char *d = dest;
  const char *s = src;

  do
    *d++ = *s;
  while (*s++ != '\0');

  return d - 1;
}

char *
grub_strcat (char *dest, const char *src)
{
  char *p = dest;

  while (*p)
    p++;

  while ((*p++ = *src++) != '\0')
    ;

  return dest;
}

char *
grub_strncat (char *dest, const char *src, int c)
{
  char *p = dest;

  while (*p)
    p++;

  while ((*p++ = *src++) != '\0' && --c)
    ;
  *(--p) = '\0';
  
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

void
grub_real_dprintf(const char *file, const int line, const char *condition,
                  const char *fmt, ...)
{
  va_list args;
  const char *debug = grub_env_get ("debug");
  if (! debug)
    return;
  if (grub_strword (debug, "all") || grub_strword (debug, condition))
    {
      grub_printf ("%s,%d : ", file, line);
      va_start (args, fmt);
      grub_vprintf (fmt, args);
      va_end (args);
    }
}

int
grub_vprintf (const char *fmt, va_list args)
{
  int ret;

  ret = grub_vsprintf (0, fmt, args);
  grub_refresh ();
  return ret;
}

int
grub_memcmp (const void *s1, const void *s2, grub_size_t n)
{
  const char *t1 = s1;
  const char *t2 = s2;
  
  while (n--)
    {
      if (*t1 != *t2)
	return (int) *t1 - (int) *t2;

      t1++;
      t2++;
    }

  return 0;
}
void *memcmp (const void *s1, const void *s2, grub_size_t n)
  __attribute__ ((alias ("grub_memcmp")));

int
grub_strcmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (*s1 != *s2)
	return (int) *s1 - (int) *s2;
      
      s1++;
      s2++;
    }

  return (int) *s1 - (int) *s2;
}

int
grub_strncmp (const char *s1, const char *s2, grub_size_t n)
{
  if (n == 0)
    return 0;
  
  while (*s1 && *s2 && --n)
    {
      if (*s1 != *s2)
	return (int) *s1 - (int) *s2;
      
      s1++;
      s2++;
    }

  return (int) *s1 - (int) *s2;
}

int
grub_strncasecmp (const char *s1, const char *s2, int c)
{
  int p = 1;

  while (grub_tolower (*s1) && grub_tolower (*s2) && p < c)
    {
      if (grub_tolower (*s1) != grub_tolower (*s2))
	return (int) grub_tolower (*s1) - (int) grub_tolower (*s2);
      
      s1++;
      s2++;
      p++;
    }

  return (int) *s1 - (int) *s2;
}

char *
grub_strchr (const char *s, int c)
{
  while (*s)
    {
      if (*s == c)
	return (char *) s;
      s++;
    }

  return 0;
}

char *
grub_strrchr (const char *s, int c)
{
  char *p = 0;

  while (*s)
    {
      if (*s == c)
	p = (char *) s;
      s++;
    }

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
grub_iswordseparator (int c)
{
  return (grub_isspace (c) || c == ',' || c == ';' || c == '|' || c == '&');
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

int
grub_isalpha (int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int
grub_isdigit (int c)
{
  return (c >= '0' && c <= '9');
}

int
grub_isgraph (int c)
{
  return (c >= '!' && c <= '~');
}

int
grub_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';

  return c;
}

unsigned long
grub_strtoul (const char *str, char **end, int base)
{
  unsigned long num = 0;
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
      else if (str[1] >= '0' && str[1] <= '7')
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
      
      if (num > (~0UL - digit) / base)
	{
	  grub_error (GRUB_ERR_OUT_OF_RANGE, "overflow is detected");
	  return 0;
	}

      num = num * base + digit;
      str++;
    }

  if (! found)
    {
      grub_error (GRUB_ERR_BAD_NUMBER, "unrecognized number");
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
grub_memset (void *s, int c, grub_size_t n)
{
  unsigned char *p = (unsigned char *) s;

  while (n--)
    *p++ = (unsigned char) c;

  return s;
}
void *memset (void *s, int c, grub_size_t n)
  __attribute__ ((alias ("grub_memset")));

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

static char *
grub_itoa (char *str, int c, unsigned n)
{
  unsigned base = (c == 'x') ? 16 : 10;
  char *p;
  
  if ((int) n < 0 && c == 'd')
    {
      n = (unsigned) (-((int) n));
      *str++ = '-';
    }

  p = str;
  do
    {
      unsigned d = n % base;
      *p++ = (d > 9) ? d + 'a' - 10 : d + '0';
    }
  while (n /= base);
  *p = 0;

  grub_reverse (str);
  return p;
}

static char *
grub_ftoa (char *str, double f, int round)
{
  unsigned int intp;
  unsigned int fractp;
  unsigned int power = 1;
  int i;

  for (i = 0; i < round; i++)
    power *= 10;

  intp = f;
  fractp = (f - (float) intp) * power;

  grub_sprintf (str, "%d.%d", intp, fractp);
  return str;
}

int
grub_vsprintf (char *str, const char *fmt, va_list args)
{
  char c;
  int count = 0;
  auto void write_char (unsigned char ch);
  auto void write_str (const char *s);
  auto void write_fill (const char ch, int n);
  
  void write_char (unsigned char ch)
    {
      if (str)
	*str++ = ch;
      else
	grub_putchar (ch);

      count++;
    }

  void write_str (const char *s)
    {
      while (*s)
	write_char (*s++);
    }

  void write_fill (const char ch, int n)
    {
      int i;
      for (i = 0; i < n; i++)
	write_char (ch);
    }
  
  while ((c = *fmt++) != 0)
    {
      if (c != '%')
	write_char (c);
      else
	{
	  char tmp[16];
	  char *p;
	  unsigned int format1 = 0;
	  unsigned int format2 = 3;
	  char zerofill = ' ';
	  int rightfill = 0;
	  int n;
	  int longfmt = 0;

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
	      char s[p - fmt];
	      grub_strncpy (s, fmt, p - fmt);
	      if (s[0] == '0')
		zerofill = '0';
	      format1 = grub_strtoul (s, 0, 10);
	      fmt = p;
	      if (*p && *p == '.')
		{
		  p++;
		  fmt++;
		  while (*p && grub_isdigit (*p))
		    p++;
		  
		  if (p > fmt)
		    {
		      char fstr[p - fmt];
		      grub_strncpy (fstr, fmt, p - fmt);
		      format2 = grub_strtoul (fstr, 0, 10);
		      fmt = p;
		    }
		}
	    }

	  c = *fmt++;
	  if (c == 'l')
	    {
	      longfmt = 1;
	      c = *fmt++;
	    }

	  switch (c)
	    {
	    case 'p':
	      write_str ("0x");
	      c = 'x';
	      /* fall through */
	    case 'x':
	    case 'u':
	    case 'd':
	      if (longfmt)
		n = va_arg (args, long);
	      else
		n = va_arg (args, int);
	      grub_itoa (tmp, c, n);
	      if (!rightfill && grub_strlen (tmp) < format1)
		write_fill (zerofill, format1 - grub_strlen (tmp));
	      write_str (tmp);
	      if (rightfill && grub_strlen (tmp) < format1)
		write_fill (zerofill, format1 - grub_strlen (tmp));
	      break;
	      
	    case 'c':
	      n = va_arg (args, int);
	      write_char (n & 0xff);
	      break;

	    case 'f':
	      {
		float f;
		f = va_arg (args, double);
		grub_ftoa (tmp, f, format2);
		if (!rightfill && grub_strlen (tmp) < format1)
		  write_fill (zerofill, format1 - grub_strlen (tmp));
		write_str (tmp);
		if (rightfill && grub_strlen (tmp) < format1)
		  write_fill (zerofill, format1 - grub_strlen (tmp));
		break;
	      }
	      
	    case 'C':
	      {
		grub_uint32_t code = va_arg (args, grub_uint32_t);
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
	      p = va_arg (args, char *);
	      if (p)
		{
		  if (!rightfill && grub_strlen (p) < format1)
		    write_fill (zerofill, format1 - grub_strlen (p));
		  
		  write_str (p);
		  
		  if (rightfill && grub_strlen (p) < format1)
		    write_fill (zerofill, format1 - grub_strlen (p));
		}
	      else
		write_str ("(null)");
	      
	      break;

	    default:
	      write_char (c);
	      break;
	    }
	}
    }

  if (str)
    *str = '\0';

  if (count && !str)
    grub_refresh ();
  
  return count;
}

int
grub_sprintf (char *str, const char *fmt, ...)
{
  va_list ap;
  int ret;
  
  va_start (ap, fmt);
  ret = grub_vsprintf (str, fmt, ap);
  va_end (ap);

  return ret;
}

/* Convert UTF-16 to UTF-8.  */
grub_uint8_t *
grub_utf16_to_utf8 (grub_uint8_t *dest, grub_uint16_t *src,
		    grub_size_t size)
{
  grub_uint32_t code_high = 0;

  while (size--)
    {
      grub_uint32_t code = *src++;

      if (code_high)
	{
	  if (code >= 0xDC00 && code <= 0xDFFF)
	    {
	      /* Surrogate pair.  */
	      code = ((code_high - 0xD800) << 12) + (code - 0xDC00) + 0x10000;
	      
	      *dest++ = (code >> 18) | 0xF0;
	      *dest++ = ((code >> 12) & 0x3F) | 0x80;
	      *dest++ = ((code >> 6) & 0x3F) | 0x80;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	  else
	    {
	      /* Error...  */
	      *dest++ = '?';
	    }

	  code_high = 0;
	}
      else
	{
	  if (code <= 0x007F)
	    *dest++ = code;
	  else if (code <= 0x07FF)
	    {
	      *dest++ = (code >> 6) | 0xC0;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	  else if (code >= 0xD800 && code <= 0xDBFF)
	    {
	      code_high = code;
	      continue;
	    }
	  else if (code >= 0xDC00 && code <= 0xDFFF)
	    {
	      /* Error... */
	      *dest++ = '?';
	    }
	  else
	    {
	      *dest++ = (code >> 16) | 0xE0;
	      *dest++ = ((code >> 12) & 0x3F) | 0x80;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	}
    }

  return dest;
}

/* Convert an UTF-8 string to an UCS-4 string. Return the number of
   characters converted. DEST must be able to hold at least SIZE
   characters (when the input is unknown). If an invalid sequence is found,
   return -1.  */
grub_ssize_t
grub_utf8_to_ucs4 (grub_uint32_t *dest, const grub_uint8_t *src,
		   grub_size_t size)
{
  grub_uint32_t *p = dest;
  int count = 0;
  grub_uint32_t code = 0;
  
  while (size--)
    {
      grub_uint32_t c = *src++;
      
      if (count)
	{
	  if ((c & 0xc0) != 0x80)
	    {
	      /* invalid */
	      return -1;
	    }
	  else
	    {
	      code <<= 6;
	      code |= (c & 0x3f);
	      count--;
	    }
	}
      else
	{
	  if ((c & 0x80) == 0x00)
	    code = c;
	  else if ((c & 0xe0) == 0xc0)
	    {
	      count = 1;
	      code = c & 0x1f;
	    }
	  else if ((c & 0xf0) == 0xe0)
	    {
	      count = 2;
	      code = c & 0x0f;
	    }
	  else if ((c & 0xf8) == 0xf0)
	    {
	      count = 3;
	      code = c & 0x07;
	    }
	  else if ((c & 0xfc) == 0xf8)
	    {
	      count = 4;
	      code = c & 0x03;
	    }
	  else if ((c & 0xfe) == 0xfc)
	    {
	      count = 5;
	      code = c & 0x01;
	    }
	  else
	    /* invalid */
	    return -1;
	}

      if (count == 0)
	*p++ = code;
    }

  return p - dest;
}

grub_err_t
grub_split_cmdline (const char *cmdline, grub_err_t (*getline) (char **),
		    int *argc, char ***argv)
{
  /* XXX: Fixed size buffer, perhaps this buffer should be dynamically
     allocated.  */
  char buffer[1024];
  char *bp = buffer;
  char *rd = (char *) cmdline;
  char unputbuf;
  int unput = 0;
  char *args;
  int i;

  auto char getchar (void);
  auto void unputc (char c);
  auto void getenvvar (void);
  auto int getarg (void);

  /* Get one character from the commandline.  If the caller reads
     beyond the end of the string a new line will be read.  This
     function will not chech for errors, the caller has to check for
     grub_errno.  */
  char getchar (void)
    {
      int c;
      if (unput)
	{
	  unput = 0;
	  return unputbuf;
	}

      if (! rd)
	{
	  getline (&rd);
	  /* Error is ignored here, the caller will check for this
	     when it reads beyond the EOL.  */
	  c = *(rd)++;
	  return c;
	}

      c = *(rd)++;
      if (! c)
	{
	  rd = 0;
	  return '\n';
	}

      return c;
    }

  void unputc (char c)
    {
      unputbuf = c;
      unput = 1;
    }

  /* Read a variable name from the commandline and insert its content
     into the buffer.  */
  void getenvvar (void)
    {
      char varname[100];
      char *p = varname;
      char *val;
      char c;

      c = getchar ();
      if (c == '{')
	while ((c = getchar ()) != '}')
	  *(p++) = c;
      else
	{
	  /* XXX: An env. variable can have characters and digits in
	     its name, are more characters allowed here?  */
	  while (c && (grub_isalpha (c) || grub_isdigit (c)))
	    {
	      *(p++) = c;
	      c = getchar ();
	    }
	  unputc (c);
	}
      *p = '\0';

      /* The variable does not exist.  */
      val = grub_env_get (varname);
      if (! val)
	return;

      /* Copy the contents of the variable into the buffer.  */
      for (p = val; *p; p++)
	*(bp++) = *p;
    }

  /* Read one argument.  Return 1 if no variables can be read anymore,
     otherwise return 0.  If there is an error, return 1, the caller
     has to check grub_errno.  */
  int getarg (void)
    {
      char c;

      /* Skip all whitespaces before an argument.  */
      do {
	c = getchar ();
      } while (c == ' ' || c == '\t');

      do {
	switch (c)
	  {
	  case '"':
	    /* Double quote.  */
	    while ((c = getchar ()))
	      {
		if (grub_errno)
		  return 1;
		/* Read in an escaped character.  */
		if (c == '\\')
		  {
		    c = getchar ();
		    *(bp++) = c;
		    continue;
		  }
		else if (c == '"')
		  break;
		/* Read a variable.  */
		if (c == '$')
		  {
		    getenvvar ();
		    continue;
		  }
		*(bp++) = c;
	      }
	    break;

	  case '\'':
	    /* Single quote.  */
	    while ((c = getchar ()) != '\'')
	      {
		if (grub_errno)
		  return 1;

		*(bp++) = c;
	      }
	    break;

	  case '\n':
	    /* This was not a argument afterall.  */
	    return 1;

	  default:
	    /* A normal option.  */
	    while (c && (grub_isalpha (c)
			 || grub_isdigit (c) || grub_isgraph (c)))
	      {
		/* Read in an escaped character.  */
		if (c == '\\')
		  {
		    c = getchar ();
		    *(bp++) = c;
		    c = getchar ();
		    continue;
		  }
		/* Read a variable.  */
		if (c == '$')
		  {
		    getenvvar ();
		    c = getchar ();
		    continue;
		  }
		*(bp++) = c;
		c = getchar ();
	      }
	    unputc (c);

	    break;
	  }
      } while (! grub_isspace (c) && c != '\'' && c != '"');

      return 0;
    }

  /* Read in all arguments and count them.  */
  *argc = 0;
  while (1)
    {
      if (getarg ())
	break;
      *(bp++) = '\0';
      (*argc)++;
    }

  /* Check if there were no errors.  */
  if (grub_errno)
    return grub_errno;

  /* Reserve memory for the return values.  */
  args = grub_malloc (bp - buffer);
  if (! args)
    return grub_errno;
  grub_memcpy (args, buffer, bp - buffer);
  
  *argv = grub_malloc (sizeof (char *) * (*argc + 1));
  if (! *argv)
    {
      grub_free (args);
      return grub_errno;
    }

  /* The arguments are separated with 0's, setup argv so it points to
     the right values.  */
  bp = args;
  for (i = 0; i < *argc; i++)
    {
      (*argv)[i] = bp;
      while (*bp)
	bp++;
      bp++;
    }

  (*argc)--;
  return 0;
}
