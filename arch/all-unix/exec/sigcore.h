#ifndef _SIGCORE_H
#define _SIGCORE_H

#ifdef __linux__
#   define __KERNEL__
#endif
#include <signal.h>
#ifdef __KERNEL__
#   undef __KERNEL__
#endif


#define _PUSH(sp,val)       (*--sp = (SP_TYPE)(val))
#define _POP(sp)            (*sp++)

#ifdef __linux__
typedef struct sigcontext_struct sigcontext_t;
#   define SIGHANDLER	    linux_sighandler
#   define SIGHANDLER_T     SignalHandler
#   define SP_TYPE	    long

#   define CPU_NUMREGS	    6

#   define SP(sc)           (sc->esp)
#   define FP(sc)           (sc->ebp)
#   define PC(sc)           (sc->eip)
#   define R0(sc)           (sc->eax)
#   define R1(sc)           (sc->ebx)
#   define R2(sc)           (sc->ecx)
#   define R3(sc)           (sc->edx)
#   define R4(sc)           (sc->edi)
#   define R5(sc)           (sc->esi)

#   define PREPARE_INITIAL_FRAME(sp,pc) \
	_PUSH(sp,pc), \
	_PUSH(sp,0), /* Frame pointer */ \
	sp -= CPU_NUMREGS

#   define SAVEREGS(sp,sc) \
	sp = (long *)SP(sc), \
	_PUSH(sp,PC(sc)), \
	_PUSH(sp,FP(sc)), \
	_PUSH(sp,R0(sc)), \
	_PUSH(sp,R1(sc)), \
	_PUSH(sp,R2(sc)), \
	_PUSH(sp,R3(sc)), \
	_PUSH(sp,R4(sc)), \
	_PUSH(sp,R5(sc))

#   define RESTOREREGS(sp,sc) \
	R5(sc) = _POP(sp), \
	R4(sc) = _POP(sp), \
	R3(sc) = _POP(sp), \
	R2(sc) = _POP(sp), \
	R1(sc) = _POP(sp), \
	R0(sc) = _POP(sp), \
	FP(sc) = _POP(sp), \
	PC(sc) = _POP(sp), \
	SP(sc) = (long)sp

#endif /* __linux__ */

#ifdef __FreeBSD__
typedef struct sigcontext sigcontext_t;
#   define SIGHANDLER	   bsd_sighandler
#   define SIGHANDLER_T    __sighandler_t *

#   define SP_TYPE	long
#   define CPU_NUMREGS	6

#   define SP(sc)	(sc->sc_esp)
#   define FP(sc)	(sc->sc_ebp)
#   define PC(sc)	(sc->sc_eip)

#   define R0(sc)           (sc->sc_eax)
#   define R1(sc)           (sc->sc_ebx)
#   define R2(sc)           (sc->sc_ecx)
#   define R3(sc)           (sc->sc_edx)
#   define R4(sc)           (sc->sc_edi)
#   define R5(sc)           (sc->sc_esi)

#   define PREPARE_INITIAL_FRAME(sp,pc) \
	_PUSH(sp,pc), \
	_PUSH(sp,0), /* Frame pointer */ \
	sp -= CPU_NUMREGS

#   define SAVEREGS(sp,sc) \
	sp = (long *)SP(sc), \
	_PUSH(sp,PC(sc)), \
	_PUSH(sp,FP(sc)), \
	_PUSH(sp,R0(sc)), \
	_PUSH(sp,R1(sc)), \
	_PUSH(sp,R2(sc)), \
	_PUSH(sp,R3(sc)), \
	_PUSH(sp,R4(sc)), \
	_PUSH(sp,R5(sc))

#   define RESTOREREGS(sp,sc) \
	R5(sc) = _POP(sp), \
	R4(sc) = _POP(sp), \
	R3(sc) = _POP(sp), \
	R2(sc) = _POP(sp), \
	R1(sc) = _POP(sp), \
	R0(sc) = _POP(sp), \
	FP(sc) = _POP(sp), \
	PC(sc) = _POP(sp), \
	SP(sc) = (long)sp
#endif /* __FreeBSD__ */

#endif /* _SIGCORE_H */
