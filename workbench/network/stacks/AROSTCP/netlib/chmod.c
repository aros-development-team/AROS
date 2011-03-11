/* $Id$
 *
 *      chmod.c - chmod() for usergroup link library
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *
 *      This function is based on SetOwner37 code
 *      Copyright © 1993 by Geert Uytterhoeven
 */

/****** net.lib/chmod *********************************************************

    NAME
        chmod, fchmod - change mode of file

    SYNOPSIS
        #include <sys/stat.h>

        int chmod(const char *path, mode_t mode);

        int fchmod(int fd, mode_t mode);

    DESCRIPTION
        The function chmod() sets the file permission bits of the file
        specified by the pathname path to mode. Fchmod() sets the permission
        bits of the specified file descriptor fd. Chmod() verifies that the
        process owner (user) either owns the file specified by path (or fd),
        or is the super-user.  A mode is created from or'd permission bit
        masks defined in <sys/stat.h>:

              #define S_IRWXU 0000700    \* RWX mask for owner *\
              #define S_IRUSR 0000400    \* R for owner *\
              #define S_IWUSR 0000200    \* W for owner *\
              #define S_IXUSR 0000100    \* X for owner *\

              #define S_IRWXG 0000070    \* RWX mask for group *\
              #define S_IRGRP 0000040    \* R for group *\
              #define S_IWGRP 0000020    \* W for group *\
              #define S_IXGRP 0000010    \* X for group *\

              #define S_IRWXO 0000007    \* RWX mask for other *\
              #define S_IROTH 0000004    \* R for other *\
              #define S_IWOTH 0000002    \* W for other *\
              #define S_IXOTH 0000001    \* X for other *\

              #define S_ISUID 0004000    \* set user id on execution *\
              #define S_ISGID 0002000    \* set group id on execution *\
              #define S_ISVTX 0001000    \* save swapped text even after use *\

        The ISVTX (the sticky bit) indicates to the system which executable
        files are shareable (pure).

        Writing or changing the owner of a file turns off the set-user-id
        and set-group-id bits unless the user is the super-user.  This makes
        the system somewhat more secure by protecting set-user-id
        (set-group-id) files from remaining set-user-id (set-group-id) if
        they are modified.

    RETURN VALUES
        Upon successful completion, a value of 0 is returned.  Otherwise, a
        value of -1 is returned and errno is set to indicate the error.

    ERRORS
        Chmod() will fail and the file mode will be unchanged if:

        [ENOTDIR]     A component of the path prefix is not a directory.

        [ENAMETOOLONG]
                      A component of a pathname exceeded 255 characters, or
                      an entire path name exceeded 1023 characters.

        [ENOENT]      The named file does not exist.

        [EACCES]      Search permission is denied for a component of the
                      path prefix.

        [EPERM]       The effective user ID does not match the owner of the
                      file and the effective user ID is not the super-user.

        [EROFS]       The named file resides on a read-only file system.

        [EFAULT]      Path points outside the process's allocated address
                      space.

        [EIO]         An I/O error occurred while reading from or writing to
                      the file system.

        Fchmod() will fail if:

        [EBADF]       The descriptor is not valid.

        [EINVAL]      Fd refers to a socket, not to a file.

        [EROFS]       The file resides on a read-only file system.

        [EIO]         An I/O error occurred while reading from or writing to
                      the file system.

    NOTES
        This call is provided for Unix compatibility.  It does not know all
        Amiga protection bits (Delete, Archive, Script).  The archive and
        script bits are cleared, Delete set according the Write bit.

    SEE ALSO
        open(),  chown(),  stat()

*****************************************************************************
*/

#include <proto/dos.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "fibex.h"
#include "netlib.h"

/*
 * rwx -> rwed
 */
const static BYTE pbits[8] = 
{ 
  0, 0X2, 0X5, 0X7, 0X8, 0XA, 0XD, 0XF, 
};

int chmod(const char *path, mode_t mode)
{
  LONG prot = 
    (pbits[mode & 7] << FIBB_OTR_DELETE) |
      (pbits[(mode >> 3) & 7] << FIBB_GRP_DELETE) |
	(pbits[(mode >> 6) & 7] ^ 0xf);

  if (mode & S_ISVTX)
    prot |= FIBF_PURE;
  if (mode & S_ISUID)
    prot |= FIBF_SUID;
  if (mode & S_ISGID)
    prot |= FIBF_SGID;

  if (!SetProtection((STRPTR)path, prot)) {
    set_errno(IoErr());
    return -1;
  } else {
    return 0;
  }
}

#ifdef DEBUGGING
#include <libraries/usergroup.h>
#include <proto/usergroup.h>
#include <stdlib.h>
#include <string.h>
#include <exec/execbase.h>
#include <ctype.h>

int errno;
extern struct ExecBase *SysBase;

void PrintUserFault(LONG code, const UBYTE *banner);

const static char usage[] = "usage: chmod [-fR] mode file ...";

void main(int argc, char *argv[])
{
  struct Process *p = (struct Process *)SysBase->ThisTask;
  BPTR Stderr = p->pr_CES ? p->pr_CES : p->pr_COS;

  short perrors = 1, recursive = 0;
  char *modenum; mode_t mode;

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

  modenum = argv[1]; argv++, argc--;

  mode = 0;
  while (isdigit(*modenum)) {
    mode <<= 3;
    mode += *modenum++ - '0';
  }

  if (*modenum) {
    FPrintf(Stderr, usage);
    exit(10);
  }

  while (argc-- > 1) {
    if (chmod(argv++[1], mode) == -1) {
      if (perrors)
	PrintUserFault(errno, "chmod");
      exit(RETURN_ERROR);
    }
  }

  exit(0);
}
#endif
