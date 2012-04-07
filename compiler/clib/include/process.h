#ifndef _PROCESS_H_
#define	_PROCESS_H_

/*
    Copyright © 2003-2012, The AROS Development Team. All rights reserved.
    $Id$

    Windows replacement functions for fork()/exec()
*/

#include <aros/system.h>

#define P_WAIT	  0  /* Wait for the child process to terminate.  */
#define P_NOWAIT  1  /* Execute the parent and the child concurrently.  */
#define P_NOWAITIO P_NOWAIT
#define P_OVERLAY 2  /* Replace parent's image with child's. Same as exec*(3).  */
/* FIXME: Implement P_DETACH support */
#define P_DETACH  3 /* Run in background without access to the console or keyboard. */

__BEGIN_DECLS

/* NOTIMPL int spawnl(int mode, const char *path, char *arg0, ...); */
/* NOTIMPL int spawnle(int mode, const char *path, char *arg0, ..., char *const envp[]); */
/* NOTIMPL int spawnlp(int mode, const char *path, char *arg0, ...); */
/* NOTIMPL int spawnlpe(int mode, const char *path, char *arg0, ..., char *const envp[]); */
int spawnv(int mode, const char *path, char *const argv[]);
/* NOTIMPL int spawnve(int mode, const char *path, char *const argv[], char *const envp[]); */
int spawnvp(int mode, const char *path, char *const argv[]);
/* NOTIMPL int spawnvpe(int mode, const char *path, char *const argv[], char *const envp[]); */

__END_DECLS
#endif /* _PROCESS_H_ */
