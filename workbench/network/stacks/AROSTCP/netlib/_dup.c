/* $Id$
 *
 *      _dup.c - duplicate a file descriptor (SAS/C)
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

/****** net.lib/dup ***********************************************************

    NAME
        dup, dup2 - duplicate an existing file descriptor

    SYNOPSIS
        #include <unistd.h>

        int dup(int oldd)

        int dup2(int oldd, int newd)

    FUNCTION
        Dup() duplicates an existing object descriptor and returns its value
        to the calling program (newd = dup(oldd)). The argument oldd is a
        small nonnegative integer index in the program's descriptor table.
        The value must be less than the size of the table, which is returned
        by getdtablesize().  The new descriptor returned by the call is the
        lowest numbered descriptor currently not in use by the program.

        The object referenced by the descriptor does not distinguish between
        oldd and newd in any way.  Thus if newd and oldd are duplicate
        references to an open file, read() and write() calls all move a single
        pointer into the file, and append mode, non-blocking I/O and
        asynchronous I/O options are shared between the references.  If a
        separate pointer into the file is desired, a different object
        reference to the file must be obtained by issuing an additional open()
        call.  The close-on-exec flag on the new file descriptor is unset.

        In dup2(), the value of the new descriptor newd is specified.  If this
        descriptor is already in use, the descriptor is first deallocated as
        if a close() call had been done first.

    RETURN VALUES
        The value -1 is returned if an error occurs in either call.  The
        external variable errno indicates the cause of the error.

    BUGS
        The current UFB implementation for SAS C allows only sockets to be
        duplicated.

    ERRORS
        Dup() and dup2() fail if:

        [EBADF]       Oldd or newd is not a valid active descriptor

        [EMFILE]      Too many descriptors are active.

    SEE ALSO
        accept(),  open(),  close(),  socket(),  getdtablesize()

    STANDARDS
        Dup() and dup2() are expected to conform to IEEE Std 1003.1-1988
        (``POSIX'').

    COPYRIGHT
        This manual page is copyright © 1980, 1991 Regents of the
        University of California.  All rights reserved.

*******************************************************************************
*/


int
dup(int old_fd)
{
  struct UFB *ufb;
  int ufbflg;
  /*
   * Check for the break signals
   */
  __chkabort();

  /*
   * Find the ufb * for the given FD
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
    return Dup2Socket(old_fd, -1);
  } else {
    errno = EBADF;
    return -1;
  }
}
