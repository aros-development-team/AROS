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

#include "config.h"

#include <stdio.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif
#include <stdlib.h>

#include "mem.h"

/* Functions */
char *
_xstrdup (const char * str, const char * file, int line)
{
    char * nstr;

    assert (str);

    nstr = strdup (str);

    if (!nstr)
    {
	fprintf (stderr, "Out of memory in %s:%d", file, line);
	exit (20);
    }

    return nstr;
}

char *
_xstrndup (const char * str, size_t len, const char * file, int line)
{
    char * nstr;

    assert (str);

    nstr = strndup (str);

    if (!nstr)
    {
	fprintf (stderr, "Out of memory in %s:%d", file, line);
	exit (20);
    }

    return nstr;
}

void *
_xmalloc (size_t size, const char * file, int line)
{
    void * ptr;

    ptr = malloc (size);

    if (size && !ptr)
    {
	fprintf (stderr, "Out of memory in %s:%d", file, line);
	exit (20);
    }

    return ptr;
}

void
_xfree (void * ptr, const char * file, int line)
{
    if (ptr)
	free (ptr);
    else
	fprintf (stderr, "Illegal free(NULL) in %s:%d", file, line);
}

