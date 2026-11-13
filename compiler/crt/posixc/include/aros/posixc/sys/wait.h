#ifndef _POSIXC_SYS_WAIT_H_
#define _POSIXC_SYS_WAIT_H_

/*
    Copyright © 2002-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file sys/wait.h
*/

#include <aros/system.h>

#define WNOHANG         0x00000001
#define WUNTRACED       0x00000002
#define WSTOPPED        WUNTRACED
#define WEXITED         0x00000004
#define WCONTINUED      0x00000008
/* NOTIMPL #define WNOWAIT */

#define WEXITSTATUS(status)     (status & 0xff)
#define WIFCONTINUED(status)    0
#define WIFEXITED(status)       1
#define WIFSIGNALED(status)     0
#define WIFSTOPPED(status)      0
#define WSTOPSIG(status)        0
#define WTERMSIG(status)        0

/* NOTIMPL
typedef enum {
     P_ALL,
     P_PGID,
     P_PID
} idtype_t;
*/

#include <aros/types/id_t.h>
#include <aros/types/pid_t.h>
#include <aros/types/siginfo_t.h>

__BEGIN_DECLS

pid_t wait(int *status);
/* NOTIMPL int    waitid(idtype_t, id_t, siginfo_t *, int); */
pid_t waitpid(pid_t pid, int *status, int options);

__END_DECLS

#endif /* _POSIXC_SYS_WAIT_H_ */
