/* $Id$
 *
 *      chown.c - chown() for usergroup link library
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 * 
 *      This function is based on SetOwner37 code
 *      Copyright © 1993 by Geert Uytterhoeven
 */


/****** net.lib/chown *********************************************************

    NAME
        chown - change owner and group of a file

    SYNOPSIS
        #include <unistd.h>

        success = chown(path, owner, group)

        int chown(const char *, uid_t, gid_t);

    DESCRIPTION
        The owner ID and group ID of the file named by path or referenced by
        fd is changed as specified by the arguments owner and group.  The
        owner of a file may change the group to a group of which he or she is
        a member, but the change owner capability is restricted to the
        super-user.

        Chown() clears the set-user-id and set-group-id bits on the file to
        prevent accidental or mischievious creation of set-user-id and
        set-group-id programs.

        One of the owner or group id's may be left unchanged by specifying it
        as -1.

        If the final component of path is a symbolic link, the ownership and
        group of the symbolic link is changed, not the ownership and group of
        the file or directory to which it points.

    RETURN VALUES
        Zero is returned if the operation was successful; -1 is returned if an
        error occurs, with a more specific error code being placed in the
        global variable errno.

    ERRORS
        Chown() will fail and the file will be unchanged if:

        [ENOTDIR]     A component of the path prefix is not a directory.

        [EINVAL]      The pathname contains a character with the high-order
                      bit set.

        [ENAMETOOLONG]
                      A component of a pathname exceeded 80 characters, or an
                      entire path name exceeded 1023 characters.

        [ENOENT]      The named file does not exist.

        [EACCES]      Search permission is denied for a component of the path
                      prefix.

        [ELOOP]       Too many symbolic links were encountered in translating
                      the pathname.

        [EPERM]       The effective user ID is not the super-user.

        [EROFS]       The named file resides on a read-only file system.

        [EFAULT]      Path points outside the process's allocated address
                      space.

        [EIO]         An I/O error occurred while reading from or writing to
                      the file system.

    SEE ALSO
        chmod(2)

*****************************************************************************
*/

#include <libraries/usergroup.h>
#include <proto/dos.h>
#include <dos/dosextens.h>

#include "netlib.h"

#ifndef ACTION_SET_OWNER	
#define ACTION_SET_OWNER	1036
#endif

int chown(const char *name, uid_t uid, gid_t gid)
{
  BPTR lock;
  short rc = -1;

  if (lock = Lock((STRPTR)name, ACCESS_READ)) {
    if (uid == -1 || gid == -1) {
      /* XXX We are supposed to do stat() and find out the suitable value */
      
    }
    rc = DoPkt(((struct FileLock *)BADDR(lock))->fl_Task,
	       ACTION_SET_OWNER, 
	       NULL, lock, MKBADDR("\0"), 
	       (UG2MU(uid) << 16) | UG2MU(gid), NULL);
    UnLock(lock);
  }

  if (!rc) {
    set_errno(IoErr());
    return -1;
  } else {
    return 0;
  }
}

#ifdef DEBUGGING
#include <proto/usergroup.h>
#include <stdlib.h>
#include <string.h>
#include <exec/execbase.h>
int errno;
extern struct ExecBase *SysBase;

void PrintUserFault(LONG code, const UBYTE *banner);

const static char usage[] = "usage: chown [-fR] owner[:group] file ...";

void main(int argc, char *argv[])
{
  struct Process *p = (struct Process *)SysBase->ThisTask;
  BPTR Stderr = p->pr_CES ? p->pr_CES : p->pr_COS;

  short perrors = 1, recursive = 0;
  uid_t uid; gid_t gid = -1;
  char *group, *user;

  while (argc > 1 && argv[1][0] == '-') {
    switch (argv[1][1]) {
    case 'f':
      perrors = 0;
      break;
    case 'R':
      recursive = 1;
      break;
    default:
      FPrintf(Stderr, usage);
      exit(10);
    }
    argv++, argc--;
  }

  if (argc <= 2) {
    FPrintf(Stderr, usage);
    exit(10);
  }

  user = argv[1]; argv++, argc--;
  group = rindex(user, ':');

  if (group) {
    *group++ = '\0';
    if (StrToLong(group, &gid) < 1) {
      struct group *gr = getgrnam(group);
      if (gr) {
	gid = gr->gr_gid;
      } else {
	if (perrors)
	  PrintUserFault(errno, "chown: getgrnam");
	exit(RETURN_ERROR);
      }
    }
  }

  if (StrToLong(user, &uid) < 1) {
    struct passwd *pw = getpwnam(user);
    if (pw) {
      uid = pw->pw_uid;
    } else {
      if (perrors)
	PrintUserFault(errno, "chown: getpwnam");
      exit(RETURN_ERROR);
    }
  }

  while (argc-- > 1) {
    if (chown(argv++[1], uid, gid) == -1) {
      if (perrors)
	PrintUserFault(errno, "chmod");
      exit(RETURN_ERROR);
    }
  }

  exit(0);
}
#endif
