/* $Id$
 *
 *      _close.c - close a file (SAS/C)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <ios1.h>
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <proto/dos.h>
#include <errno.h>
#include <bsdsocket.h>

int 
__close(int fd)
{
  struct UFB *ufb;

  /*
   * Check for the break signals
   */
  __chkabort();

  /*
   * Find the ufb *
   */
  if ((ufb = __chkufb(fd)) == NULL) {
    /* __chkufb sets the errno to EBADF */
    return -1;
  }

  /*
   * Check if close is not needed
   */
  if ((ufb->ufbflg & (UFB_NC | UFB_CLO)) != UFB_NC) {

    /*
     * Empty flags mean empty ufb
     */
    if (ufb->ufbflg == 0) {
      errno = EBADF;
      return -1;
    }

    /*
     * Close the file
     */
    if (!(ufb->ufbflg & UFB_SOCK) && ufb->ufbfh != NULL) {
      Close(ufb->ufbfh);
      
      /*
       * Remove the file if it was temporary
       */
      if (ufb->ufbflg & UFB_TEMP && ufb->ufbfn != NULL) 
	remove(ufb->ufbfn);
    }

  }

  /*
   * Free the file name
   */
  if (ufb->ufbfn != NULL) {
    free(ufb->ufbfn);
    ufb->ufbfn = NULL;
  }

  /*
   * Clear the flags to free this ufb
   */
  ufb->ufbflg = 0;
  ufb->ufbfh = NULL; /* just in case */

  /* 
   * closes the socket OR the file mark
   */
  CloseSocket(fd);
  
  return 0;
}

