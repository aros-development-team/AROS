/* $Id$
 *
 *      stat.c - stat() for the netlib
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <string.h>
#include <stdlib.h>

/* DOS 3.0 and MuFS extensions to file info block */
#include "fibex.h"
#include "netlib.h"
#include <proto/dos.h>
#include <proto/utility.h>

int stat(const char *name, struct stat *st)
{
  short found;
  register int rc = -1;
  BPTR lock;

  if (st == NULL || ((1 & (long)st) == 1)) {
    errno = EFAULT;
    return -1;
  }

  lock = Lock((STRPTR)name, SHARED_LOCK);

  if (found = lock != NULL) {
    if (Examine(lock, __dostat_fib)) {
      __dostat(__dostat_fib, st);
      st->st_dev = (dev_t)((struct FileLock *)BADDR(lock))->fl_Task;
      rc = 0;
    } else {
      errno = EIO;
    }
  } else {
    UBYTE errcode = IoErr();
    
    if (errcode == ERROR_OBJECT_IN_USE) {
      rc = lstat(name, st);
    } else {
      set_errno(errcode);
    }
  }

  if (lock)
    UnLock(lock);

  return rc;
}

int lstat(const char *name, struct stat *st)
{
  /* Cannot lock - do examine via Examine()/ExNext() */
  int rc = -1;
  char *cname;

  if (st == NULL || ((1 & (long)st) == 1)) {
    errno = EFAULT;
    return -1;
  }

  cname = malloc(strlen(name) + 1);

  if (cname) {
    BPTR lock;
    char *pp = PathPart(strcpy(cname, name));
    *pp = '\0';

    if (lock = Lock(cname, SHARED_LOCK)) {
      pp = FilePart((STRPTR)name);
      
      if (Examine(lock, __dostat_fib)) {
	while (ExNext(lock, __dostat_fib)) {
	  if (Stricmp(pp, __dostat_fib->fib_FileName) == 0) {
	    __dostat(__dostat_fib, st);
	    st->st_dev = (dev_t)((struct FileLock *)BADDR(lock))->fl_Task;
	    rc = 0;
	    break;
	  }
	}
      } 
      if (rc != 0)
	errno = ENOENT;
    } else {
      set_errno(IoErr());
    }

    free(cname);
  } else {
    errno = ENOMEM;
  }

  return rc;
}

