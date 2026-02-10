/* $Id$
 *
 *      _lseek.c - lseek() which knows sockets (SAS/C)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <ios1.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dos.h>
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>

long
__lseek(int fd, long rpos, int mode)
{
  struct UFB *ufb;
  long        apos;

  /*
   * Check for the break signals
   */
  __chkabort();
  /*
   * find the ufb *
   */
  if ((ufb = __chkufb(fd)) == NULL) {
    errno = EINVAL;
    return -1;
  }
  
  _OSERR = 0;

  if (ufb->ufbflg & UFB_SOCK) {
    errno = ESPIPE; /* illegal seek */
    return -1;
  }

  if ((apos = Seek(ufb->ufbfh, rpos, mode - 1)) == -1) {
    _OSERR = IoErr();
    errno = EIO;
    return -1;
  }
  
  switch (mode) {
  case 0:
    return rpos;
  case 1:
    return apos + rpos;
  case 2:
    return Seek(ufb->ufbfh, 0, 0);
  }
}
