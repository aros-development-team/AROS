#ifndef __MMAKE_IO_H
#define __MMAKE_IO_H

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

#include <stdio.h>
#include <stdint.h>

/* These functions write out the specified data in a machine independent way, e.g.
 * file written with these function can be read in on all other platforms.
 * All io functions return 0 when they fail, 1 for succes */

int writestring (FILE * fh, const char *s);
int readstring (FILE * fh, char **strptr); /* if *strptr != NULL it has to be xfreed after use */
int writeint32 (FILE * fh, int32_t i);
int readint32 (FILE * fh, int32_t * iptr);
int writeuint32 (FILE * fh, uint32_t i);
int readuint32 (FILE * fh, uint32_t * iptr);

#endif /* __MMAKE_IO_H */
