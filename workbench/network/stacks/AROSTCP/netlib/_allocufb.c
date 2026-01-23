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
#include <libraries/fd.h>
#include <proto/fd.h>

/*
 * Allocate new ufb, which is returned as return value. The corresponding fd
 * is returned via fdp.
 */
struct UFB *
__allocufb(int *fdp, fd_owner_t owner)
{
  struct UFB *ufb = __ufbs, *last_ufb = NULL;
  int         last_fd = 0;
  int         check_shared = (owner != FD_OWNER_NONE) && FDBase;
  LONG        error;

  /*
   * find first free ufb
   */
  while (ufb != NULL) {
    if (ufb->ufbflg == 0) {
      if (!check_shared || FD_Check(last_fd) == 0) {
        if (check_shared) {
          error = FD_Reserve(last_fd, owner, NULL);
          if (error == 0) {
            *fdp = last_fd;
            return ufb;
          }
          if (error != EBUSY) {
            errno = error;
            return NULL;
          }
        } else {
          *fdp = last_fd;
          return ufb;
        }
      }
    }
    last_ufb = ufb;
    last_fd++;
    ufb = last_ufb->ufbnxt;
  }

  /*
   * Check if need to create one
   */
  for (;;) {
    if ((ufb = malloc(sizeof(*ufb))) == NULL) {
      errno = ENOMEM;
      return NULL;
    }
    ufb->ufbnxt = NULL;
    ufb->ufbflg = 0;            /* => unused ufb */

    if (last_ufb == NULL)
      __ufbs = ufb;
    else
      last_ufb->ufbnxt = ufb;

    __nufbs++;

    if (!check_shared || FD_Check(last_fd) == 0) {
      if (check_shared) {
        error = FD_Reserve(last_fd, owner, NULL);
        if (error == 0) {
          *fdp = last_fd;
          return ufb;
        }
        if (error != EBUSY) {
          errno = error;
          return NULL;
        }
      } else {
        *fdp = last_fd;
        return ufb;
      }
    }

    last_ufb = ufb;
    last_fd++;
  }
}
