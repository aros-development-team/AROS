/*
    Copyright © 2002-2004, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef SYS_WAIT_H_
#define SYS_WAIT_H_

#include <aros/system.h>

#define WNOHANG         0x00000001
#define WUNTRACED       0x00000002
#define WSTOPPED        WUNTRACED
#define WEXITED         0x00000004
#define WCONTINUED      0x00000008

#define WIFEXITED(status)       1
#define WEXITSTATUS(status)     (status & 0xff)
#define WIFSIGNALED(status)     0
#define WTERMSIG(status)        0
#define WIFSTOPPED(status)      0
#define WSTOPSIG(status)        0
#define WIFCONTINUED(status)    0

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

//struct rusage;
//pid_t   wait3(int *, int, struct rusage *);
//pid_t   wait4(pid_t, int *, int, struct rusage *);

#endif /* SYS_WAIT_H_ */

