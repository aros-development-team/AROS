#ifndef __MMAKE_MEM_H
#define __MMAKE_MEM_H

/* MetaMake - A Make extension
   Copyright © 1995-2004, The AROS Development Team. All rights reserved.

This file is part of MetaMake.

MetaMake is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

MetaMake is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <ctype.h>
#ifdef HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif

/* Macros */
#define cfree(x)        if (x) free (x)
#define SETSTR(str,val) \
    cfree (str); \
    str = val ? xstrdup (val) : NULL

#define xstrdup(str)        _xstrdup(str,__FILE__,__LINE__)
#define xmalloc(size)       _xmalloc(size,__FILE__,__LINE__)
#define xfree(ptr)          _xfree(ptr,__FILE__,__LINE__)
#define new(x)              ((x *) xmalloc (sizeof (x)))

extern char * _xstrdup (const char * str, const char * file, int line);
extern void * _xmalloc (size_t size, const char * file, int line);
extern void _xfree (void * ptr, const char * file, int line);

#endif /* __MMAKE_MEM_H */
