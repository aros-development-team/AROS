#ifndef _SIGNAL_H_
#define _SIGNAL_H_

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    System header file <signal.h>
    Based on SUSv2 with help from C99.
*/

#include <aros/system.h>

#include <aros/types/__sighandler_t.h>

#define SIG_DFL	    ((__sighandler_t *)0)   /* default signal handling */
#define SIG_IGN	    ((__sighandler_t *)1)   /* ignore this signal */
#if !defined(_ANSI_SOURCE)
#define SIG_HOLD    ((__sighandler_t *)2)   /* hold this signal */
#endif
#define SIG_ERR	    ((__sighandler_t *)-1)  /* return from signal() on error */

typedef AROS_SIG_ATOMIC_T   sig_atomic_t;

/* Definitions to make signal manipulation easier. From FreeBSD */
#define _SIG_IDX(sig)		((sig) - 1)
#define _SIG_WORD(sig)		(_SIG_IDX(sig) >> 5)
#define _SIG_BIT(sig)		(1 << (_SIG_IDX(sig) & 31))
#define _SIG_VALID(sig)		((sig) < _SIG_MAXSIG && (sig) > 0)

/* Almost all the remaining definitions are not part of ANSI C */
#if !defined(_ANSI_SOURCE)

#include <aros/types/sigset_t.h>
#include <aros/types/pid_t.h>

#endif /* !_ANSI_SOURCE */

/* Signal values */
#define SIGHUP		1	/* hangup */
#define SIGINT		2	/* interrupt */
#define SIGQUIT		3	/* quit */
#define SIGILL		4	/* illegal instr. */
#if !defined(_POSIX_SOURCE)
#define SIGTRAP		5	/* trace/breakpoint track */
#endif
#define SIGABRT		6	/* abort() */
#if !defined(_POSIX_SOURCE)
#define SIGEMT		7	/* EMT instruction (emulator trap) */
#endif
#define SIGFPE		8	/* floating point exception */
#define SIGKILL		9	/* kill (cannot be caught or ignored) */
#if !defined(_POSIX_SOURCE)
#define SIGBUS		10	/* bus error */
#endif
#define SIGSEGV		11	/* segmentation violation */
#if !defined(_POSIX_SOURCE)
#define SIGSYS		12	/* non-existent system call */
#endif
#define SIGPIPE		13	/* write on a pipe without a reader */
#define SIGALRM		14	/* alarm clock */
#define SIGTERM		15	/* software termination */
#if !defined(_POSIX_SOURCE)
#define SIGURG		16	/* urgent IO condition */
#endif
#define SIGSTOP		17	/* sendable stop signal */
#define SIGTSTP		18	/* terminal stop signal */
#define SIGCONT		19	/* continue a stopped process */
#define SIGCHLD		20	/* to parent on child stop or exit */
#define SIGTTIN		21	/* to reader to background tty read */
#define SIGTTOU		22	/* to writer on background tty write */
#if !defined(_POSIX_SOURCE)
#define SIGXCPU		23	/* exceeded CPU time limit */
#define SIGXFSZ		24	/* exceeded file size limit */
#define SIGVTALRM	25	/* virtual timer alarm */
#define SIGPROF		26	/* profiling time alarm */
#define SIGWINCH	27	/* window size changes */
#endif
#define SIGUSR1		28	/* user defined signal 1 */
#define SIGUSR2		29	/* user defined signal 2 */

#if !defined(_POSIX_SOURCE)
/*  BSD defines SIGIO and SIGINFO, we should probably include these */
#define SIGIO		30	/* IO event */
#define SIGINFO		31	/* terminal info request */
#endif

/* Real time signals as specified by POSIX 1003.1B */
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

/*
    sigevent() is an advanced call that allows a process to request the
    system to perform more advanced signal delivery that calling signal
    handlers.

    AROS allows for signals to be queued, delivered normally, or by
    creating a new thread and calling a function.

    See the sigevent() manual page for more information.
*/
#include <aros/types/sigevent_s.h>

/*
    Call a function.
    XXX Note that we do not support sigev_notify_attributes
*/
#define sigev_notify_function	\
    __sigev_u.__sigev_notify_call.__sigenv_notify_function

#if !defined(_ANSI_SOURCE)

#include <aros/types/sigaction_s.h>

#if !defined(_POSIX_SOURCE)

#include <aros/types/size_t.h>
#include <aros/types/stack_t.h>

struct sigstack
{
    int		     ss_onstack;	/* non-zero when signal stack in use */
    void	    *ss_sp;		/* signal stack pointer */
};

#define MINSIGSTKSZ	8192
#define SIGSTKSZ	(MINSIGSTKSZ + 40960)

#include <aros/types/ucontext_t.h>
#include <aros/types/siginfo_t.h>

/* Tag for struct timespec */
struct timespec;

#endif /* !_POSIX_SOURCE */

/* Flags for sigprocmask() */
#define SIG_BLOCK	1
#define SIG_UNBLOCK	2
#define SIG_SETMASK	34

#endif /* !_ANSI_SOURCE */

/* Function Prototypes */
__BEGIN_DECLS

int	raise(int);
__sighandler_t *signal(int, __sighandler_t *);

#if !defined(_ANSI_SOURCE)
int	kill(pid_t, int);

int	sigaction(int, const struct sigaction *, struct sigaction *);
int	sigaddset(sigset_t *, int);
int	sigdelset(sigset_t *, int);
int	sigemptyset(sigset_t *);
int	sigfillset(sigset_t *);
/* NOTIMPL int	sighold(int); */
/* NOTIMPL int	sigignore(int); */
int	sigismember(const sigset_t *, int);
int	sigpending(sigset_t *);
int	sigprocmask(int, const sigset_t *, sigset_t *);
/* NOTIMPL int	sigrelse(int); */
/* NOTIMPL int	sigset(__sighandler_t *, int); */
int	sigsuspend(const sigset_t *);
/* NOTIMPL int	sigwait(const sigset_t *, int *); */

#if !defined(_POSIX_SOURCE)
/* NOTIMPL int	killpg(pid_t, int); */
/* NOTIMPL int	sigaltstack(const stack_t *, stack_t *); */
/* NOTIMPL int	siginterrupt(int); */
/* NOTIMPL int	sigpause(int); */
#endif /* !_POSIX_SOURCE */

/* NOTIMPL int	sigqueue(pid_t, int, const union sigval); */
/* NOTIMPL int	sigtimedwait(const sigset_t *, siginfo_t *, const struct timespec *); */
/* NOTIMPL int	sigwaitinfo(const sigset_t *, siginfo_t *); */

#endif /* !_ANSI_SOURCE */

__END_DECLS

#endif /* _SIGNAL_H_ */
