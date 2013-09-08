#ifndef _POSIXC_SIGNAL_H_
#define _POSIXC_SIGNAL_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 header file <signal.h>
*/

/* C99 */
#include <aros/stdc/signal.h>

#define SIG_HOLD    ((__sighandler_t *)2)   /* hold this signal */

/* TODO: support for pthread_t, pthread_attr_t */
/* NOTSUPP pthread_t */
#include <aros/types/size_t.h>
#include <aros/types/uid_t.h>
#include <aros/types/timespec_s.h>
#include <aros/types/sigset_t.h>
#include <aros/types/pid_t.h>
/* NOTSUPP pthread_attr_t */
#include <aros/types/sigevent_s.h>

/* Real time signals as specified by POSIX.1-2008 */
/* TODO: implement SIGRT signals
#define SIGRTMIN	33
#define SIGRT1		33
#define SIGRT2		34
#define SIGRT3		35
#define SIGRT4		36
#define SIGRT5		37
#define SIGRT6		38
#define SIGRT7		39
#define SIGRT8		40
#define SIGRT9		41
#define SIGRT10		42
#define SIGRT11		43
#define SIGRT12		44
#define SIGRT13		45
#define SIGRT14		46
#define SIGRT15		47
#define SIGRT16		48
#define SIGRT17		49
#define SIGRT18		50
#define SIGRT19		51
#define SIGRT20		52
#define SIGRT21		53
#define SIGRT22		54
#define SIGRT23		55
#define SIGRT24		56
#define SIGRT25		57
#define SIGRT26		58
#define SIGRT27		59
#define SIGRT28		60
#define SIGRT29		61
#define SIGRT30		62
#define SIGRT31		63
#define SIGRT32		64
#define SIGRTMAX	64

#define RTSIG_MAX	32
*/

#define _SIGMAX 64

/* Extra POSIX-2008.1 signal values */
#define SIGALRM		14	/* alarm clock */
#define SIGBUS		10	/* bus error */
#define SIGCHLD		20	/* to parent on child stop or exit */
#define SIGCONT		19	/* continue a stopped process */
#define SIGHUP		1	/* hangup */
#define SIGKILL		9	/* kill (cannot be caught or ignored) */
#define SIGPIPE		13	/* write on a pipe without a reader */
#define SIGQUIT		3	/* quit */
#define SIGSTOP		17	/* sendable stop signal */
#define SIGTSTP		18	/* terminal stop signal */
#define SIGTTIN		21	/* to reader to background tty read */
#define SIGTTOU		22	/* to writer on background tty write */
#define SIGUSR1		28	/* user defined signal 1 */
#define SIGUSR2		29	/* user defined signal 2 */
//FIXME: #define SIGPOLL
#define SIGPROF		26	/* profiling time alarm */
#define SIGSYS		12	/* non-existent system call */
#define SIGTRAP		5	/* trace/breakpoint track */
#define SIGURG		16	/* urgent IO condition */
#define SIGVTALRM	25	/* virtual timer alarm */
#define SIGXCPU		23	/* exceeded CPU time limit */
#define SIGXFSZ		24	/* exceeded file size limit */

#include <aros/types/sigaction_s.h>

/* Flags for sigprocmask() */
#define SIG_BLOCK	1
#define SIG_UNBLOCK	2
#define SIG_SETMASK	34

/* TODO: determine proper values for MINSIGSTKSZ and SIGSTKSZ */
#define MINSIGSTKSZ	8192
#define SIGSTKSZ	(MINSIGSTKSZ + 40960)

#include <aros/types/ucontext_t.h> /* Also defines mcontext_t */
#include <aros/types/stack_t.h>
#include <aros/types/siginfo_t.h>

/* Non-standard signals
   FIXME: remove ?
*/
#if !defined(_POSIX_SOURCE)
// NOTIMPL #define SIGEMT		7	/* EMT instruction (emulator trap) */
// NOTIMPL #define SIGWINCH	27	/* window size changes */
/*  BSD defines SIGIO and SIGINFO, we should probably include these */
// NOTIMPL #define SIGIO		30	/* IO event */
// NOTIMPL #define SIGINFO		31	/* terminal info request */
#endif


__BEGIN_DECLS

int	kill(pid_t, int);
/* NOTIMPL int	killpg(pid_t, int); */
/* NOTIMPL void   psiginfo(const siginfo_t *, const char *); */
/* NOTIMPL void   psignal(int, const char *); */
/* NOTIMPL int    pthread_kill(pthread_t, int); */
/* NOTIMPL int    pthread_sigmask(int, const sigset_t *restrict, sigset_t *restrict); */
int	sigaction(int, const struct sigaction *, struct sigaction *);
int	sigaddset(sigset_t *, int);
/* NOTIMPL int	sigaltstack(const stack_t *, stack_t *); */
int	sigdelset(sigset_t *, int);
int	sigemptyset(sigset_t *);
int	sigfillset(sigset_t *);
/* NOTIMPL int	sighold(int); */
/* NOTIMPL int	sigignore(int); */
/* NOTIMPL int	siginterrupt(int); */
int	sigismember(const sigset_t *, int);
/* NOTIMPL int	sigpause(int); */
int	sigpending(sigset_t *);
int	sigprocmask(int, const sigset_t *, sigset_t *);
/* NOTIMPL int	sigqueue(pid_t, int, const union sigval); */
/* NOTIMPL int	sigrelse(int); */
/* NOTIMPL int	sigset(__sighandler_t *, int); */
int	sigsuspend(const sigset_t *);
/* NOTIMPL int	sigtimedwait(const sigset_t *, siginfo_t *, const struct timespec *); */
/* NOTIMPL int	sigwait(const sigset_t *, int *); */
/* NOTIMPL int	sigwaitinfo(const sigset_t *, siginfo_t *); */

__END_DECLS

#endif /* _POSIXC_SIGNAL_H_ */
