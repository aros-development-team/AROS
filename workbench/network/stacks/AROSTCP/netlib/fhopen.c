/* $Id$
 *
 *      fhopen.c - open level 1 file from an AmigaDOS file handle (SAS/C)
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

extern int (*__closefunc)(int);

int
fhopen(long file, int mode)
{
  struct UFB *ufb;
  int         fd;
  int         flags;

  /*
   * Set up __closefunc (which is used at cleanup)
   */
  __closefunc = __close;
  /*
   * Check for the break signals
   */
  __chkabort();
  /*
   * find first free ufb
   */
  ufb = __allocufb(&fd);
  if (ufb == NULL)
    return -1; /* errno is set by the __allocufb() */

  /*
   * Translate mode to ufb flags
   */
  switch (mode & (O_WRONLY | O_RDWR)) {
  case O_RDONLY:
    if (mode & (O_APPEND | O_CREAT | O_TRUNC | O_EXCL)) {
      errno = EINVAL;
      return -1;
    }
    flags = UFB_RA;
    break;
  case O_WRONLY:
    flags = UFB_WA;
    break;
  case O_RDWR:
    flags = UFB_RA | UFB_WA;
    break;
  default:
    errno = EINVAL;
    return -1;
  }
  if (mode & O_APPEND)
    flags |= UFB_APP;
  if (mode & O_XLATE)
    flags |= UFB_XLAT;
  /*
   * All done!
   */
  ufb->ufbflg = flags;
  ufb->ufbfh = (long)file;
  ufb->ufbfn = NULL;

#if 0
  if (Dup2Socket(-1, fd) < 0)	/* mark the fd as used in the AmiTCP's dTable */
    perror("Dup2Socket failed on " __FILE__);
#endif

  return fd;
}
