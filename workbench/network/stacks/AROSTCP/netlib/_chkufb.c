/* $Id$
 *
 *      _chkufb.c - return struct ufb * from a file handle (SAS/C)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <ios1.h>
#include <stdlib.h>
#include <errno.h>
#include <bsdsocket.h>
#include <sys/cdefs.h>
#include <amitcp/socketbasetags.h>
#include <syslog.h>

extern unsigned long __fmask;
extern int (*__closefunc)(int);

long ASM fdCallback(REG(d0) int fd, REG(d1) int action);

/*
 * The initializator priority is just above the standard I/O, so that this
 * will be called after the standard I/O is initialized
 */
long __stdargs
_STI_510_install_AmiTCP_callback(void)
{
  if (SocketBaseTags(SBTM_SETVAL(SBTC_FDCALLBACK), &fdCallback, TAG_END)) {
    syslog(LOG_ERR, "Cannot install fdCallback!");
#if __VERSION__ > 6 || __REVISION__ > 3
    return 1;
#else
    exit(20);
#endif
  }

  /*
   * Set up __closefunc (which is used at stdio cleanup)
   */
  __closefunc = __close;

  /*
   * Set default file mask to UNIX style
   */
  __fmask = 0644; 

  return 0;
}

long ASM SAVEDS
fdCallback(REG(d0) int fd, REG(d1) int action)
{
  struct UFB *ufb;
  int fd2;

#ifdef DEBUG
  syslog(LOG_INFO, "fdCallback(fd: %d, action: %d)", fd, action);
#endif

  switch (action) {
  case FDCB_FREE:
    ufb = __chkufb(fd);
    if (ufb == NULL)
      return EBADF;

    if (!(ufb->ufbflg & UFB_SOCK) && ufb->ufbflg != 0) {
#ifdef DEBUG
      syslog(LOG_ERR, "fdCallback: fd (%d) is not a socket!", fd);
#endif
      return ENOTSOCK;
    }

    ufb->ufbflg = 0;
    return 0;

  case FDCB_ALLOC:
    do {
      ufb = __allocufb(&fd2);
      if (ufb == NULL)
	return ENOMEM;
#ifdef DEBUG
      if (fd2 > fd) {
	syslog(LOG_ERR, "fdCallback: fd2(%d) > fd(%d)!", fd2, fd);
	return EINVAL;
      }
#endif
      ufb->ufbflg = UFB_SOCK | UFB_WA | UFB_RA; /* read/write socket */
      ufb->ufbfh = NULL; /* no file handle */
      ufb->ufbfn = NULL; /* no name */
    } while (fd2 < fd);
    return 0;

  case FDCB_CHECK:
    ufb = __chkufb(fd);
    if (ufb != NULL && ufb->ufbflg != 0) 
      return EBADF;
    
    return 0;

  default:
#ifdef DEBUG
    syslog(LOG_ERR, "fdCallback: invalid action.");
#endif
    return EINVAL;
  }
}


struct UFB *
__chkufb(int fd)
{
  struct UFB *ufb;

  /* a single element cache */
  static struct UFB *last_ufb = NULL;
  static int         last_fd = -1;

  _OSERR = 0;

  if ((unsigned int)fd >= __nufbs) { /* unsigned cast checks for (fd < 0) */
    errno = EBADF;
    return NULL;
  }

  /*
   * Check the cache first
   */
  if (fd == last_fd)
    return last_ufb;

  last_fd = fd; /* update cache */
  ufb = __ufbs;
  while (fd > 0 && ufb != NULL) {
    fd--;
    ufb = ufb->ufbnxt;
  }
  last_ufb = ufb; /* update cache */
  
  if (ufb == NULL) {
    last_fd = -1; /* invalidate cache */
    errno = EIO;
    return NULL;
  }
  else
    return ufb;
}
