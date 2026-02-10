/* $Id$
 *
 *      dostat.c - *stat() function common part 
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <proto/dos.h>
#include <proto/utility.h>
#include <libraries/usergroup.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <string.h>
#include <stdlib.h>

/* DOS 3.0 and MuFS extensions to file info block */
#include "fibex.h"

/*
 * Conversion table from Amiga filetypes to Unix filetypes
 */
const static mode_t ftype[ST_LINKDIR - ST_PIPEFILE + 1] = {
  S_IFIFO,
  S_IFREG,
  S_IFREG,
  S_IFREG,
  S_IFREG,
  S_IFREG,
  S_IFDIR,
  S_IFDIR,
  S_IFLNK,
  S_IFDIR,
};

/*
 * Conversion table from Amiga protections to Unix protections
 * rwed -> rwx
 */
const static UBYTE fbits[16] =
{
  00, 02, 01, 03, 02, 02, 03, 03,
  04, 06, 05, 07, 06, 06, 07, 07,
};

void __dostat(struct FileInfoBlock *fib,
	      struct stat *st)
{
  ULONG pbits = fib->fib_Protection ^ 0xf;
  short fibtype = fib->fib_DirEntryType - ST_PIPEFILE;
  mode_t mode;

  if (fibtype < 0)
    fibtype = 0;
  else if (fibtype > ST_LINKDIR - ST_PIPEFILE)
    fibtype = ST_LINKDIR - ST_PIPEFILE;

  bzero(st, sizeof(*st));

  mode = ftype[fibtype] | (fbits[pbits & 0xf] << 6)
    | (fbits[(pbits >> FIBB_GRP_DELETE) & 0xf] << 3)
      | fbits[(pbits >> FIBB_OTR_DELETE) & 0xf];

  if ((pbits & FIBF_PURE) != 0)
    mode |= S_ISVTX;
  if ((pbits & FIBF_SUID) != 0)
    mode |= S_ISUID;
  if ((pbits & FIBF_SGID) != 0)
    mode |= S_ISGID;

  st->st_ino = fib->fib_DiskKey;
  st->st_mode = mode;
  st->st_nlink = 1;
  st->st_uid = MU2UG(fib->fib_OwnerUID);
  st->st_gid = MU2UG(fib->fib_OwnerGID);
  st->st_rdev = 0;
  st->st_size = fib->fib_Size;

  /* 
   * Calculatory time since Jan 1 1970, UCT 
   * (in reality there are an odd number of leap seconds, 
   * which are not included)
   */
  st->st_atime = st->st_ctime = st->st_mtime =
    60 * ((fib->fib_Date.ds_Days + (8*365+2)) * 24 * 60
	  + fib->fib_Date.ds_Minute)
      + fib->fib_Date.ds_Tick / 50;

  st->st_blksize = 512;
  st->st_blocks = fib->fib_NumBlocks;
  st->st_dosmode = fib->fib_Protection;
  st->st_type = fib->fib_DirEntryType;
  st->st_comment = fib->fib_Comment;
}

/****** net.lib/stat *********************************************************

    NAME
        stat, lstat, fstat - get file status

    SYNOPSIS
        #include <sys/types.h>
        #include <sys/stat.h>

        success = stat(path, buf)

        int stat(const char *, struct stat *);

        success =  lstat(path, buf);

        int lstat(const char *, struct stat *);

        success = fstat(fd, buf);

        int fstat(int, struct stat *);

    DESCRIPTION
        The stat() function obtains information about the file pointed to by
        path. Read, write or execute permission of the named file is not
        required, but all directories listed in the path name leading to the
        file must be seachable.

        Lstat() is like stat() except in the case where the named file is a
        symbolic link, in which case lstat() returns information about the
        link, while stat() returns information about the file the link
        references.

        The fstat() obtains the same information about an open file known by
        the file descriptor fd, such as would be obtained by an open call.

        Buf is a pointer to a stat() structure as defined by <sys/stat.h>
        (shown below) and into which information is placed concerning the
        file.

           struct  stat
           {
             dev_t   st_dev;         \* unique device id *\ 
             ino_t   st_ino;         \* inode of file (key block) *\ 
             mode_t  st_mode;        \* Unix style mode *\ 
             ushort  st_nlink;       \* number of links (unimplemented) *\ 
             uid_t   st_uid;         \* owner's user ID *\ 
             gid_t   st_gid;         \* owner's group ID *\ 
             dev_t   st_rdev;        \* special file ID (unimplemented) *\ 
             off_t   st_size;        \* file size *\ 
             time_t  st_atime;       \* Time of last access *\ 
             time_t  st_mtime;       \* Last modification time *\ 
             time_t  st_ctime;       \* Last file status change time *\ 
             long    st_blksize;     \* Size of disk block *\ 
             long    st_blocks;      \* Size in blocks *\ 
             long    st_dosmode;     \* DOS protection bits *\ 
             short   st_type;        \* DOS file type *\ 
             char   *st_comment;     \* DOS file comment *\ 
           };

        The time-related fields of struct stat have same contents, time when
        file data last modified.

        The status information word st_mode has bits as follows:

          #define S_ISUID  0004000    \* set user id on execution *\ 
	  #define S_ISGID  0002000    \* set group id on execution *\ 
	  #define S_ISVTX  0001000    \* save swapped text even after use *\ 
	  #define S_IRUSR  0000400    \* read permission for owner *\ 
	  #define S_IWUSR  0000200    \* write permission for owner *\ 
	  #define S_IXUSR  0000100    \* execute permission for owner *\ 
	  #define S_IRGRP  0000040    \* read permission for group *\ 
	  #define S_IWGRP  0000020    \* write permission for group *\ 
	  #define S_IXGRP  0000010    \* execute permission for group *\ 
	  #define S_IROTH  0000004    \* read permission for other *\ 
	  #define S_IWOTH  0000002    \* write permission for other *\ 
	  #define S_IXOTH  0000001    \* execute permission for other *\ 
	  #define S_IFCHR  0020000    \* character special *\ 
	  #define S_IFDIR  0040000    \* directory *\ 
	  #define S_IFBLK  0060000    \* block special *\ 
	  #define S_IFREG  0100000    \* regular *\ 
	  #define S_IFLNK  0120000    \* symbolic link *\ 
	  #define S_IFSOCK 0140000    \* socket *\ 
	  #define S_IFIFO  0010000    \* named pipe (fifo) *\ 

        For a list of access modes, see <sys/stat.h>, access(2) and chmod(2).

    RETURN VALUES
        Upon successful completion a value of 0 is returned.  Otherwise, a
        value of -1 is returned and errno is set to indicate the error.

    ERRORS
        The functions stat() and lstat() will fail if:

        [ENOTDIR]       A component of the path prefix is not a directory.

        [ENAMETOOLONG]  A component of a pathname exceeded 255 characters,
                        or an entire path name exceeded 1023 characters.

        [ENOENT]        The named file does not exist.

        [ELOOP]         Too many symbolic links were encountered in
                        translating the pathname.

        [EACCES]        Search permission is denied for a component of the
                        path prefix.

        [EFAULT]        Buf or name points to an invalid address.

        [EIO]           An I/O error occurred while reading from or writing
                        to the file system.

        The function fstat() will fail if:

        [EBADF]   fd is not a valid open file descriptor.

        [EFAULT]  Buf points to an invalid address.

        [EIO]     An I/O error occurred while reading from or writing to the
                  file system.

    SEE ALSO
        chmod(),  chown()

    BUGS 
        Applying fstat to a socket returns a zero'd buffer.

*******************************************************************************
*/

/****** net.lib/fstat *********************************************************

    SEE ALSO
        stat()

*******************************************************************************
*/

/****** net.lib/lstat *********************************************************

    SEE ALSO
        stat()

*******************************************************************************
*/

#ifdef DEBUGGING

#include <stdio.h>
#include <fcntl.h>

void printstat(char *what, struct stat *st)
{
  printf("stat(%s):\n"
	 "  st_dev     =%lx\n"
	 "  st_ino     =%ld\n"
	 "  st_mode    =%lo\n"
	 "  st_nlink   =%ld\n"
	 "  st_uid     =%ld\n"
	 "  st_gid     =%ld\n"
	 "  st_rdev    =%lx\n"
	 "  st_size    =%ld\n"
	 "  st_atime   =%ld\n"
	 "  st_mtime   =%ld\n"
	 "  st_ctime   =%ld\n"
	 "  st_blksize =%ld\n"
	 "  st_blocks  =%ld\n",
	 what, 
	 st->st_dev,
	 st->st_ino,
	 st->st_mode,
	 st->st_nlink,
	 st->st_uid,
	 st->st_gid,
	 st->st_rdev,
	 st->st_size,
	 st->st_atime,
	 st->st_mtime,
	 st->st_ctime,
	 st->st_blksize,
	 st->st_blocks);
}
void main(int argc, char *argv[])
{
  struct stat st[1];

  if (argc > 1) {
    if (stat(argv[1], st) == 0) {
      printstat(argv[1], st);
    } else {
      perror(argv[1]);
    }
  } 

  argv++;
  if (argc > 2) {
    if (lstat(argv[1], st) == 0) {
      printstat(argv[1], st);
    } else {
      perror(argv[1]);
    }
  } 

  argv++;
  if (argc > 3) {
    int fd = open(argv[1], O_RDONLY, 0);

    if (fd > 0) {
      if (fstat(fd, st) == 0) {
	printstat(argv[1], st);
      } else {
	perror("fstat");
      }
    } else {
      perror(argv[1]);
    }
  }
}

#endif

