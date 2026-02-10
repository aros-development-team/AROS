/* $Id$
 *
 *      access.c - check access to a file or a directory
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/memory.h>

#include <bsdsocket.h>

/*
 * I know, the goto's are ugly, but they make the code smaller and help
 * to prevent duplicating code.
 */
int
access(const char *name, int mode)
{
  BPTR    lock, parentLock;
  LONG    prot;
  UBYTE   bytes[sizeof(struct FileInfoBlock) + sizeof(struct InfoData) + 3];
  struct FileInfoBlock *fib;
  struct InfoData      *info;

  /*
   * align the data areas
   */
  fib = (struct FileInfoBlock *) (((ULONG) bytes+3) & (0xFFFFFFFF-3));
  info = (struct InfoData *) (ULONG)(fib + 1);

  /*
   * Lock the file (or directory)
   */
  if ((lock = Lock((STRPTR)name, SHARED_LOCK)) == NULL)
    goto osfail;
  
  if (!Examine(lock, fib))
    goto osfail;
  
  prot = fib->fib_Protection;

  /*
   * Check each access mode
   */
  if (mode & R_OK && prot & FIBF_READ) {
    errno = EACCES;
    goto fail;
  }
  if (mode & W_OK) {
    /*
     * Check for write protected disks
     */
    if (!Info(lock, info))
      goto osfail;

    if (info->id_DiskState == ID_WRITE_PROTECTED) {
      errno = EROFS;
      goto fail;
    }

    /*
     * not write protected: Check if the lock is to the root of the 
     * disk, if it is, force writing to be allowed.
     * Check if the lock is a directory before taking ParentDir()
     */
    if (fib->fib_DirEntryType >= 0) { /* lock is a directory */
      parentLock = ParentDir(lock);
      if (parentLock != NULL)
	UnLock(parentLock); /* not the root, prot is valid */
      else
	prot &= ~FIBF_WRITE; /* the root, force writing to be allowed */
    }
    if (prot & FIBF_WRITE) {
      errno = EACCES;
      goto fail;
    }
  }
  if (mode & X_OK && prot & FIBF_EXECUTE) {
    errno = EACCES;
    goto fail;
  }
  
  /* F_OK */

  UnLock(lock);
  return 0;
  
 osfail:
#if __SASC
  errno = __io2errno(_OSERR = IoErr());
#else
  _ug_set_errno(IoErr());
#endif

 fail:
  if (lock != NULL)
    UnLock(lock);

  return -1;
}
