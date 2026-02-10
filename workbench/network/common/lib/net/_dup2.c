/* $Id$
 *
 *      _dup2.c - duplicate a file descriptor (SAS/C)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <ios1.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dos.h>
#define USE_BUILTIN_MATH
#include <string.h>
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <bsdsocket.h>

/****** net.lib/dup2 **********************************************************
    SEE ALSO
        dup()
*******************************************************************************
*/


int
__dup2(int old_fd, int new_fd)
{
  struct UFB *ufb;
  int ufbflg;

  /*
   * Check if there is nothing to do
   */
  if (old_fd == new_fd)
    return old_fd;

  /*
   * Check for the break signals
   */
  __chkabort();

  __close(new_fd);

  /*
   * Find the ufb * for the old FD
   */
  if ((ufb = __chkufb(old_fd)) == NULL) {
    errno = EBADF;
    return -1;
  }

  ufbflg = ufb->ufbflg;

  /* 
   * The brain dead UFB system won't allow duplicating ordinary files
   */
  if ((ufbflg & UFB_SOCK) == UFB_SOCK) {
    return Dup2Socket(old_fd, new_fd);
  } else {
    errno = EBADF;
    return -1;
  }

}
