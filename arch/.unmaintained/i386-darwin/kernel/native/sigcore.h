/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SIGCORE_H
#define _SIGCORE_H

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ucontext.h>
#include <signal.h>
#include <errno.h>
//#include "etask.h"

/* Put a value of type SP_TYPE on the stack or get it off the stack. */
#define _PUSH(sp,val)       (*--sp = (SP_TYPE)(val))
#define _POP(sp)            (*sp++)

typedef ucontext_t sigcontext_t;

#define SIGHANDLER	darwin_sighandler

#define SP_TYPE		long

#define SC_DISABLE(sc)   ((sc)->uc_sigmask = sig_int_mask)
#define SC_ENABLE(sc)    (sigemptyset(&(sc)->uc_sigmask))

/* this is from wine's dlls/ntdll/signal_i386.c, which is under lgpl */

/* work around silly renaming of struct members in OS X 10.5 */
#if __DARWIN_UNIX03 && defined(_STRUCT_X86_EXCEPTION_STATE32)

#define R0(context)     ((context)->uc_mcontext->__ss.__eax)
#define R1(context)     ((context)->uc_mcontext->__ss.__ebx)
#define R2(context)     ((context)->uc_mcontext->__ss.__ecx)
#define R3(context)     ((context)->uc_mcontext->__ss.__edx)
#define R4(context)     ((context)->uc_mcontext->__ss.__edi)
#define R5(context)     ((context)->uc_mcontext->__ss.__esi)
#define R6(context)     ((context)->uc_mcontext->__ss.__eflags)

#define FP(context)     ((context)->uc_mcontext->__ss.__ebp)
#define PC(context)     ((context)->uc_mcontext->__ss.__eip)
#define SP(context)     ((context)->uc_mcontext->__ss.__esp)

#define FPSTATE(context) ((context)->uc_mcontext->__fs)

#else

#define R0(context)     ((context)->uc_mcontext->ss.eax)
#define R0(context)     ((context)->uc_mcontext->ss.eax)
#define R1(context)     ((context)->uc_mcontext->ss.ebx)
#define R2(context)     ((context)->uc_mcontext->ss.ecx)
#define R3(context)     ((context)->uc_mcontext->ss.edx)
#define R4(context)     ((context)->uc_mcontext->ss.edi)
#define R5(context)     ((context)->uc_mcontext->ss.esi)
#define R6(context)     ((context)->uc_mcontext->ss.eflags)

#define FP(context)     ((context)->uc_mcontext->ss.ebp)
#define PC(context)     ((context)->uc_mcontext->ss.eip))
#define SP(context)     ((context)->uc_mcontext->ss.esp))

#define FPSTATE(context) ((context)->uc_mcontext->fs)

#endif 

/* Here we have to setup a complete stack frame
 so Exec_Exception thinks it was called as a
 normal function. */
#define SETUP_EXCEPTION(sc,arg)\
do                                        \
{                                         \
_PUSH(GetSP(SysBase->ThisTask), arg); \
_PUSH(GetSP(SysBase->ThisTask), arg); \
} while (0)

/*
 * We can't have an #ifdef based on FreeBSD here because this structure
 * is (wrongly) accessed from rom/exec.
 */
struct AROS_cpu_context
{
  unsigned int regs[9];	/* eax, ebx, ecx, edx, edi, esi, isp, fp, pc */
  int	errno_backup;
  _STRUCT_X86_FLOAT_STATE32 fpstate;
	int eflags;
  struct AROS_cpu_context * sc;
};

#define SIZEOF_ALL_REGISTERS	(sizeof(struct AROS_cpu_context))
#define GetCpuContext(task)	((struct AROS_cpu_context *)\
				(GetIntETask(task)->iet_Context))
#define GetSP(task)		(*(SP_TYPE **)(&task->tc_SPReg))

#define GLOBAL_SIGNAL_INIT \
	static void sighandler (int sig, sigcontext_t * sc); \
							     \
	static void SIGHANDLER (int sig, int code, sigcontext_t *sc) \
	{						     \
	    sighandler( sig, (sigcontext_t*)sc);             \
	}

#define SAVE_CPU(cc,sc)                                              \
    do {                                                            \
	(cc)->regs[0] = R0(sc);                                     \
	(cc)->regs[1] = R1(sc);                                     \
	(cc)->regs[2] = R2(sc);                                     \
	(cc)->regs[3] = R3(sc);                                     \
	(cc)->regs[4] = R4(sc);                                     \
	(cc)->regs[5] = R5(sc);                                     \
	(cc)->regs[6] = R6(sc);                                     \
        (cc)->regs[7] = FP(sc);                                     \
        (cc)->regs[8] = PC(sc);                                     \
    } while (0)

#define RESTORE_CPU(cc,sc)                                          \
    do {                                                            \
	R0(sc) = (cc)->regs[0];                                     \
	R1(sc) = (cc)->regs[1];                                     \
	R2(sc) = (cc)->regs[2];                                     \
	R3(sc) = (cc)->regs[3];                                     \
	R4(sc) = (cc)->regs[4];                                     \
	R5(sc) = (cc)->regs[5];                                     \
	R6(sc) = (cc)->regs[6];                                     \
        FP(sc) = (cc)->regs[7];                                     \
        PC(sc) = (cc)->regs[8];                                     \
    } while (0)

#define SAVE_ERRNO(cc)                                              \
    do {                                                            \
	(cc)->errno_backup = errno;                                 \
    } while (0)

#define RESTORE_ERRNO(cc)                                           \
    do {                                                            \
	errno = (cc)->errno_backup;                                 \
    } while (0)


#ifndef NO_FPU

#       define SAVE_FPU(cc,sc)                                              \
            do {                                                            \
				(cc)->fpstate = FPSTATE(sc);								\
            } while (0)

#       define RESTORE_FPU(cc,sc)                                           \
            do {                                                            \
			  FPSTATE(sc) = (cc)->fpstate;									\
            } while (0)

#       define HAS_FPU(sc)      1

#       define PREPARE_FPU(cc)                                              \
            do {                                                            \
            } while (0)

#else
    /* NO FPU VERSION */

#   define SAVE_FPU(cc,sc)                                          \
        do {                                                        \
        } while (0)

#   define RESTORE_FPU(cc,sc)                                       \
        do {                                                        \
        } while (0)

#   define HAS_FPU(sc)      0

#   define PREPARE_FPU(cc)                                          \
        do {                                                        \
        } while (0)

#endif

#define PREPARE_INITIAL_FRAME(sp,startpc)                           \
    do {                                                            \
        GetCpuContext(task)->regs[7] = 0;                           \
        GetCpuContext(task)->regs[8] = (startpc);                   \
    } while (0)

#define PREPARE_INITIAL_CONTEXT(task,startpc)                       \
    do {                                                            \
        PREPARE_FPU(GetCpuContext(task));                           \
    } while (0)

#define SAVEREGS(task,sc)                                           \
    do {                                                            \
        struct AROS_cpu_context *cc = GetCpuContext(task);          \
        GetSP(task) = (SP_TYPE *)SP(sc);                            \
        if (HAS_FPU(sc))                                            \
            SAVE_FPU(cc,sc);                                        \
        SAVE_CPU(cc,sc);                                            \
        SAVE_ERRNO(cc);                                             \
    } while (0)

#define RESTOREREGS(task,sc)                                        \
    do {                                                            \
        struct AROS_cpu_context *cc = GetCpuContext(task);          \
	RESTORE_ERRNO(cc);                                          \
	RESTORE_CPU(cc,sc);                                         \
        if (HAS_FPU(sc))                                            \
            RESTORE_FPU(cc,sc);                                     \
	SP(sc) = (SP_TYPE *)GetSP(task);                            \
	} while (0)

#define PRINT_SC(sc) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx  FPU=%s\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , HAS_FPU(sc) ? "yes" : "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc), R6(sc) \
      );

#define PRINT_CPUCONTEXT(task) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , (ULONG)(GetSP(task)) \
	    , GetCpuContext(task)->regs[7] \
	    , GetCpuContext(task)->regs[8] \
	    , GetCpuContext(task)->regs[0] \
	    , GetCpuContext(task)->regs[1] \
	    , GetCpuContext(task)->regs[2] \
	    , GetCpuContext(task)->regs[3] \
	    , GetCpuContext(task)->regs[4] \
	    , GetCpuContext(task)->regs[5] \
	    , GetCpuContext(task)->regs[6] \
      );

#endif /* _SIGCORE_H */
