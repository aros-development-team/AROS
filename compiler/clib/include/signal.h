#ifndef _SIGNAL_H_
#define _SIGNAL_H_
/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    System header file <signal.h>
    Based on SUSv2 with help from C99.
*/

#include <sys/_types.h>
#include <sys/_posix.h>
#include <sys/cdefs.h>

typedef void __sighandler_t (int);

#define SIG_DFL	    ((__sighandler_t *)0)   /* default signal handling */
#define SIG_IGN	    ((__sighandler_t *)1)   /* ignore this signal */
#if !defined(_ANSI_SOURCE)
#define SIG_HOLD    ((__sighandler_t *)2)   /* hold this signal */
#endif
#define SIG_ERR	    ((__sighandler_t *)-1)  /* return from signal() on error */

/* XXX Make sure that this agrees with the length in inttypes.h */
#if 0
typedef AROS_ATOMIC_TYPE    sig_atomic_t;
#endif

/* Definitions to make signal manipulation easier. From FreeBSD */
#define _SIG_WORDS		4
#define _SIG_MAXSIG		(_SIG_WORDS * 32)
#define _SIG_IDX(sig)		((sig) - 1)
#define _SIG_WORD(sig)		(_SIG_IDX(sig) >> 5)
#define _SIG_BIT(sig)		(1 << (_SIG_IDX(sig) & 31))
#define _SIG_VALID(sig)		((sig) < _SIG_MAXSIG && (sig) > 0)

/* Almost all the remaining definitions are not part of ANSI C */
#if !defined(_ANSI_SOURCE)

typedef struct __sigset {
    unsigned int	__val[_SIG_WORDS];
} sigset_t;

#ifndef __AROS_PID_T_DECLARED
#define __AROS_PID_T_DECLARED
typedef __pid_t         pid_t;
#endif

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

#if defined(_P1003_1B_VISIBLE)

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

/* Value passed to sigevent() SIGEV_THREAD functions */
union sigval
{
    int	    sigval_int;
    void *  sigval_ptr;
};

/*
    sigevent() is an advanced call that allows a process to request the
    system to perform more advanced signal delivery that calling signal
    handlers.

    AROS allows for signals to be queued, delivered normally, or by
    creating a new thread and calling a function.

    See the sigevent() manual page for more information.
*/
struct sigevent
{
    int		    sigev_notify;	/* notification type */
    union {
	int	    __sigev_signo;	/* signal number */
	struct {
	    void    (*__sigev_notify_function)(union sigval);
	} __sigev_notify_call;		/* call a function */
    } __sigev_u;
    union sigval    sigev_value;	/* signal value */
};

/* Send a signal to the process */
#define sigev_signo		__sigev_u.__sigev_signo

/*
    Call a function.
    XXX Note that we do not support sigev_notify_attributes
*/
#define sigev_notify_function	\
    __sigev_u.__sigev_notify_call.__sigenv_notify_function

#define SIGEV_NONE	0	/* No notification */
#define SIGEV_SIGNAL	1	/* Generate a queued signal */
#define SIGEV_THREAD	2	/* Call a notification function */

/*
    siginfo_t is delivered to sigaction() style signal handlers.
    It's part of the POSIX Realtime Extension
*/
typedef struct __siginfo
{
    int		    si_signo;	    /* signal number */
    int		    si_errno;	    /* errno value */
    int		    si_code;	    /* signal code */
    pid_t	    si_pid;	    /* sending process ID */
    __uid_t         si_uid;	    /* user ID of sending process XXX */
    void *	    si_addr;	    /* address of faulting instruction */
    int		    si_status;	    /* exit value or signal */
    long	    si_band;	    /* band event for SIGPOLL */
    union sigval    si_value;	    /* signal value */
} siginfo_t;

#endif /* P1003_1B_VISIBLE */

#if !defined(_ANSI_SOURCE)

struct __siginfo;

/*
    sigaction() provides an advanced interface for setting signal handling
    options.
*/
struct sigaction
{
    union {
	void		(*__sa_handler)(int);
	void		(*__sa_sigaction)(int, struct __siginfo *, void *);
    } __sigaction_u;			/* signal handler */
    int			sa_flags;	/* see below */
    sigset_t		sa_mask;	/* signal mask to apply */
};

#define sa_handler	__sigaction_u.__sa_handler

#if !defined(_POSIX_SOURCE)

#define __need_size_t
#include <stddef.h>

/* if SA_SIGINFO is set, use sa_sigaction rather than sa_handler */
#define sa_sigaction	__sigaction_u.__sa_sigaction

#define SA_NOCLDSTOP	0x0001
#define SA_ONSTACK	0x0002
#define	SA_RESETHAND	0x0004
#define SA_RESTART	0x0008
#define SA_SIGINFO	0x0010
#define SA_NOCLDWAIT	0x0020
#define SA_NODEFER	0x0040

/* For sigaltstack() and the sigaltstack structure */
typedef struct sigaltstack
{
    void	    *ss_sp;		/* signal stack base */
    size_t	    ss_size;		/* signal stack size */
    int		    ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
} stack_t;

#define SS_ONSTACK	0x0001
#define SS_DISABLE	0x0002

struct sigstack
{
    int		     ss_onstack;	/* non-zero when signal stack in use */
    void	    *ss_sp;		/* signal stack pointer */
};

#define MINSIGSTKSZ	8192
#define SIGSTKSZ	(MINSIGSTKSZ + 40960)

/* XXX - We need ucontext_t */

/* Reasons why a signal was generated */

/* For SIGILL */
#define ILL_ILLOPC	1	    /* illegal opcode */
#define ILL_ILLOPN	2	    /* illegal operand */
#define ILL_ILLADR	3	    /* illegal address mode */
#define ILL_ILLTRP	4	    /* illegal trap */
#define ILL_PRVOPC	5	    /* priviledged opcode */
#define ILL_PRVREG	6	    /* priviledged register */
#define ILL_COPROC	7	    /* coprocessor error */
#define ILL_BADSTACK	8	    /* internal stack error */

/* For SIGFPE */
#define FPE_INTDIV	1	    /* integer divide by zero */
#define FPE_INTOVF	2	    /* integer overflow */
#define FPE_FLTDIV	3	    /* floating point divide by zero */
#define FPE_FLTOVF	4	    /* floating point overflow */
#define FPE_FLTUND	5	    /* floating point underflow */
#define FPE_FLTRES	6	    /* floating point inexact result */
#define FPE_FLTINV	7	    /* invalid floating point operation */
#define FPE_FLTSUB	8	    /* subscript out of range */

/* For SIGSEGV */
#define SEGV_MAPERR	1	    /* address not mapped to object */
#define SEGV_ACCERR	2	    /* invalid permissions for object */

/* For SIGBUS */
#define BUS_ADRALN	1	    /* Address alignment */
#define BUS_ADRERR	2	    /* Bus address error */
#define BUS_OBJERR	3	    /* object specific hardware error */

/* For SIGTRAP */
#define TRAP_BRKPT	1	    /* Process breakpoint */
#define TRAP_TRACE	2	    /* Process trace trap */

/* For SIGCHLD */
#define CLD_EXITED	1	    /* Child has exited */
#define CLD_KILL	2	    /* Child terminated abnormally */
#define CLD_CORE	3	    /* Child dumped core */
#define CLD_TRAPPED	4	    /* Traced child has trapped */
#define CLD_STOPPED	5	    /* Traced child has stopped */
#define CLD_CONTINUED	6	    /* Traced child has continued */

/* For SIGPOLL */
#define POLL_IN		1	    /* data input available */
#define POLL_OUT	2	    /* data output available */
#define POLL_MSG	3	    /* input message available */
#define POLL_ERR	4	    /* I/O error */
#define POLL_PRI	5	    /* high priority input available */
#define POLL_HUP	6	    /* device disconnected */

/* Others */
#define SI_USER		0x10001	    /* Signal sent by kill() */
#define SI_QUEUE	0x10002	    /* Signal sent by sigqueue() */
#define SI_TIMER	0x10003	    /* Signal sent by timer */
#define SI_ASYNCIO	0x10004	    /* Signal sent by async I/O */
#define SI_MESGQ	0x10005	    /* Signal generated by message queue */

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
int	sighold(int);
int	sigignore(int);
int	sigismember(const sigset_t *, int);
int	sigpending(sigset_t *);
int	sigprocmask(int, const sigset_t *, sigset_t *);
int	sigrelse(int);
int	sigset(__sighandler_t *, int);
int	sigsuspend(const sigset_t *);
int	sigwait(const sigset_t *, int *);

#if !defined(_POSIX_SOURCE)
int	killpg(pid_t, int);
int	sigaltstack(const stack_t *, stack_t *);
int	siginterrupt(int);
int	sigpause(int);
#endif /* !_POSIX_SOURCE */

#if defined(_P1003_1B_VISIBLE)
int	sigqueue(pid_t, int, const union sigval);
int	sigtimedwait(const sigset_t *, siginfo_t *, const struct timespec *);
int	sigwaitinfo(const sigset_t *, siginfo_t *);
#endif /* _P1003_1B */

#endif /* !_ANSI_SOURCE */

__END_DECLS

#endif /* _SIGNAL_H_ */
