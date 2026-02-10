/* $Id$
 *
 *      getpid() - stub for the FindTask(NULL)
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *      Copyright © 2005 Pavel Fedin
 */

#include <sys/types.h>
#include <proto/exec.h>

pid_t
getpid(void)
{
  return (pid_t)FindTask(NULL);
}
