/* $Id$
 *
 *      _open.c - Unix compatible open() (SAS/C)
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
#include <proto/usergroup.h>
#include <stdarg.h>
#include <unistd.h>

#include <bsdsocket.h>

#include "netlib.h"

extern int (*__closefunc)(int);

__stdargs int
__open(const char *name, int mode, ...)
{
  struct UFB *ufb;
  int         fd;
  int         flags;
  char        newfile = TRUE;
  BPTR        file;

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
   * malloc space for the name & copy it
   */
  if ((ufb->ufbfn = malloc(strlen(name)+1)) == NULL) {
    SET_OSERR(ERROR_NO_FREE_STORE);
    errno = ENOMEM;
    return -1;
  }
  strcpy(ufb->ufbfn, name);
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
  if (mode & O_TEMP)
    flags |= UFB_TEMP;
  if (mode & O_CREAT) {
    BPTR lock;
    if (lock = Lock((char *)name, SHARED_LOCK)) {
      if (mode & O_EXCL) {
	UnLock(lock);
	errno = EEXIST;
	free(ufb->ufbfn);
	return -1;
      }

      if (mode & O_TRUNC)
	newfile = FALSE;
      else
	mode &= ~O_CREAT;

      UnLock(lock);
    }
  }
  if (mode & O_CREAT) {
    if ((file = Open((char *)name, MODE_NEWFILE)) == NULL)
      goto osfail;

    if (newfile) {
      va_list va;
      int cmode;

      va_start(va, mode);

      cmode = va_arg(va, int) & ~getumask();
      
      chmod((char *)name, cmode); /* hope this doesn't fail :-) */
    }
  }
  else {
    if ((file = Open((char *)name,
		     (flags & UFB_WA && mode & O_LOCK) ? 
		     MODE_READWRITE : MODE_OLDFILE)) == NULL)
      goto osfail;
  }

  /*
   * All done! Setting the ufb->ufbflg field to non-zero value marks
   * this ufb used. 
   */
  ufb->ufbflg = flags;
  ufb->ufbfh = (long)file;

  return fd;

osfail:
  {
    int code = IoErr();
    if (ufb->ufbfn)
      free(ufb->ufbfn);
    set_errno(code);
  }
  return -1;
}
