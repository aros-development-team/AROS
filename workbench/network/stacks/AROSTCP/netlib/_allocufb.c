/* $Id$
 *
 *      _allocufb.c - get a free ufb (SAS/C)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <ios1.h>
#include <stdlib.h>
#include <errno.h>

/*
 * Allocate new ufb, which is returned as return value. The corresponding fd
 * is returned via fdp.
 */
struct UFB *
__allocufb(int *fdp)
{
  struct UFB *ufb, *last_ufb;
  int         last_fd = 0;

  /*
   * find first free ufb
   */
  last_ufb = ufb = __ufbs;
  while (ufb != NULL && ufb->ufbflg != 0) {
    last_ufb = ufb;
    last_fd++;
    ufb = last_ufb->ufbnxt;
  }
  /*
   * Check if need to create one
   */
  if (ufb == NULL) {
    if ((ufb = malloc(sizeof(*ufb))) == NULL) {
      errno = ENOMEM;
      return NULL;
    }
    ufb->ufbnxt = NULL;
    ufb->ufbflg = 0;		/* => unused ufb */

    if (last_ufb == NULL)
      __ufbs = ufb;
    else
      last_ufb->ufbnxt = ufb;
    
    *fdp = __nufbs++;
  }
  else
    *fdp = last_fd;
  
  return ufb;
}
