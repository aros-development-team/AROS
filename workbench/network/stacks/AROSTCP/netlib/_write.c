/* $Id$
 *
 *      _write.c - write() for both files and sockets (SAS/C)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <ios1.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <bsdsocket.h>

extern int __io2errno(long);

int
__write(int fd, const void *buffer, unsigned int length)
{
  struct UFB *ufb;
  int         count, totcount;
  char       *ptr;

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
  /*
   * Check if write is allowed
   */
  if (!(ufb->ufbflg & UFB_WA)) {
    _OSERR = ERROR_WRITE_PROTECTED;
    errno = EIO;
    return -1;
  }

  /*
   * Seek to end of the file if necessary
   */
  if (ufb->ufbflg & UFB_APP)
    __lseek(fd, 0, 2);

  /*
   * Check if translation is not needed
   */
  if (!(ufb->ufbflg & UFB_XLAT) ||
      (ptr = memchr(buffer, 0x0A, length)) == NULL) {
    if (ufb->ufbflg & UFB_SOCK) {
      if ((count = send(fd, (char *)buffer, length, 0)) < 0)
	return -1;
    }
    else {
      if ((count = Write(ufb->ufbfh, (void *)buffer, length)) == -1)
	goto osfail;
    }
    return count;
  }

  totcount = length;

  /*
   * Translate, ie., append CR before each LF
   */
  do {
    count = ptr - (char *)buffer;
    if (ufb->ufbflg & UFB_SOCK) {
      if (send(fd, (char *)buffer, count, 0) < 0)
	return -1;
      if (send(fd, "\015"/* CR */, 1, 0) < 0)
	return -1;
    }
    else {
      if (Write(ufb->ufbfh, (void *)buffer, count) == -1)
	goto osfail;
      if (Write(ufb->ufbfh, "\015"/* CR */, 1) == -1)
	goto osfail;
    }
    length -= count;
    
    buffer = ptr;
  } while ((ptr = memchr((char *)buffer + 1, 0x0A, length)) != NULL);
  
  if (ufb->ufbflg & UFB_SOCK) {
    if ((count = send(fd, (char *)buffer, length, 0)) < 0)
      return -1;
  }
  else {
    if (Write(ufb->ufbfh, (void *)buffer, length) == -1)
      goto osfail;
  }

  return totcount;

osfail:
  errno = __io2errno(_OSERR = IoErr());
  return -1;
}
