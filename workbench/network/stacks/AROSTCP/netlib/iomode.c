/* $Id$
 *
 *      iomode.c - set file io mode (normal or translated) (SAS/C)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <ios1.h>
#include <stdlib.h>

int
iomode(int fd, int mode)
{
  struct UFB *ufb;

  /*
   * find the ufb *
   */
  if ((ufb = __chkufb(fd)) == NULL) {
    return -1;
  }
  /*
   * Set the translation mode
   *
   * mode == 0 - translate mode on
   * mode == 1 - translate mode off
   */
  if (mode)
    ufb->ufbflg &= ~UFB_XLAT;
  else
    ufb->ufbflg |= UFB_XLAT;

  return 0;
}
