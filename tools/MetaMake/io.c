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
#ifdef HAVE_NETINET_IN_H
#   include <netinet/in.h> /* for htonl/ntohl() */
#endif

#include "mem.h"

int
writestring (FILE * fh, const char * s)
{
    uint32_t out;
    int32_t len;

    if (s != NULL)
	len = strlen(s);
    else
	len = -1;
    
    out = htonl(len);
    fwrite(&out, sizeof(out), 1, fh);

    if (!ferror(fh) && len>0)
	fwrite(s, len, 1, fh);
    
    return !ferror(fh);
}

int
readstring (FILE * fh, char **strptr)
{
    uint32_t in;
    int32_t len;
    
    fread(&in, sizeof(in), 1, fh);
    if (ferror(fh))
	return 0;
    
    len = ntohl(in);
    if (len>0)
    {
	*strptr = xmalloc (len+1);
	fread (*strptr, len, 1, fh);
	if (ferror(fh))
	{
	    xfree (*strptr);
	    return 0;
	}
	(*strptr)[len] = 0;
    }
    else if (len == 0)
	*strptr = xstrdup("");
    else /* len < 0 */
	*strptr = NULL;
    
    return 1;
}

int
writeint32 (FILE * fh, int32_t i)
{
    uint32_t out;

    out = htonl(i);
    fwrite(&out, sizeof(out), 1, fh);

    return !ferror(fh);
}

int
readint32 (FILE * fh, int32_t * iptr)
{
    uint32_t in;
    
    fread(&in, sizeof(in), 1, fh);
    if (!ferror(fh))
	*iptr = ntohl(in);
    
    return !ferror(fh);
}

int
writeuint32 (FILE * fh, uint32_t i)
{
    uint32_t out;

    out = htonl(i);
    fwrite(&out, sizeof(out), 1, fh);

    return !ferror(fh);
}

int
readuint32 (FILE * fh, uint32_t * iptr)
{
    uint32_t in;
    
    fread(&in, sizeof(in), 1, fh);
    if (!ferror(fh))
	*iptr = ntohl(in);
    
    return !ferror(fh);
}
    
