/*
 *  This file is taken from ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Copyright (C) 2005 Pavel Fedin
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  writev.c,v 1.1.1.1 1994/04/04 04:30:39 amiga Exp
 *
 *  writev.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:39  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#include <unistd.h>
#include <sys/uio.h>

ssize_t writev (int fd, const struct iovec *iov, int iovcnt)
{
  int written = 0;
  int res;

  while (iovcnt--)
    {
      res = write( fd, iov->iov_base, iov->iov_len);
      if (res == -1) return -1;
      written += res;
      if (res < iov->iov_len) break;
      iov++;
    }

  return written;
}

