#ifndef _PROCESS_H_
#define	_PROCESS_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/cdefs.h>

__BEGIN_DECLS

#define P_WAIT	  0  /* Wait for the child process to terminate.  */
#define P_NOWAIT  1  /* Execute the parent and the child concurrently.  */
#define P_OVERLAY 2  /* Replace parent's image with child's. Same as exec*(3).  */

int spawnv(int mode, const char *path, char *const argv[]);
int spawnvp(int mode, const char *path, char *const argv[]);

__END_DECLS
#endif /* _PROCESS_H_ */
