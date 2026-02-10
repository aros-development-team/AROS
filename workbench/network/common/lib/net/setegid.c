/* $Id$
 *
 *      setegid() - set effective group
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <sys/types.h>
#include <unistd.h>

#include <proto/usergroup.h>

#ifdef setegid
#undef setegid
#endif

int
setegid(gid_t g)
{
  return setregid(-1, g);
}
