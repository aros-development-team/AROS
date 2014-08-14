/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef VA_COPY
#if defined(__MORPHOS__)
#define VA_COPY(dest, src) __va_copy(dest, src)
#elif defined(__AROS__)
#define VA_COPY(dest, src) va_copy(dest, src)
#else
#define VA_COPY(dest, src) (dest) = (src)
#endif
#endif

int vasprintf(char **ptr, const char * format, va_list ap)
{
  int ret;
  va_list ap2;

  *ptr = NULL;

  VA_COPY(ap2, ap);
  ret = vsnprintf(NULL, 0, format, ap2);
  if(ret <= 0)
    return ret;

  *ptr = (char *)malloc(ret+1);
  if(*ptr == NULL)
    return -1;

  VA_COPY(ap2, ap);
  ret = vsnprintf(*ptr, ret+1, format, ap2);

  return ret;
}
