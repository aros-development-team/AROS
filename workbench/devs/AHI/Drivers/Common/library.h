#ifndef AHI_Drivers_Common_library_h
#define AHI_Drivers_Common_library_h

#include <exec/types.h>
#include <proto/exec.h>

#include "DriverData.h"

extern const char  LibName[];
extern const char  LibIDString[];
extern const UWORD LibVersion;
extern const UWORD LibRevision;

void
ReqA( const char*        text,
      APTR               args,
      struct DriverBase* AHIsubBase );

#define Req(a0, args...) \
        ({ULONG _args[] = { args }; ReqA((a0), (APTR)_args, AHIsubBase);})

void
MyKPrintFArgs( UBYTE*           fmt, 
	       ULONG*           args,
	       struct DriverBase* AHIsubBase );

#if !defined(__AMIGAOS4__)
#define KPrintF( fmt, ... )        \
({                                 \
  ULONG _args[] = { __VA_ARGS__ }; \
  MyKPrintFArgs( (fmt), _args, AHIsubBase ); \
})
#else
#define KPrintF DebugPrintF
#endif


#if defined(__MORPHOS__)

# include <emul/emulregs.h>
# define INTGW(q,t,n,f)							\
	q t n ## _code(void) { APTR d = (APTR) REG_A1; return f(d); }	\
	q struct EmulLibEntry n = { TRAP_LIB, 0, (APTR) n ## _code };
# define PROCGW(q,t,n,f)						\
	q struct EmulLibEntry n = { TRAP_LIB, 0, (APTR) f };
# define INTERRUPT_NODE_TYPE NT_INTERRUPT

#elif defined(__amithlon__)

# define INTGW(q,t,n,f)							\
	__asm("	.text");						\
	__asm("	.align 4");						\
	__asm("	.type " #n "_code,@function");				\
	__asm("	.type " #n ",@function");				\
	__asm(#n "_code: movl %ebp,%eax");				\
	__asm(" bswap %eax");						\
	__asm("	pushl %eax");						\
	__asm("	call " #f );						\
	__asm(" addl $4,%esp");						\
	__asm(" ret");							\
	__asm(#n "=" #n "_code+1");					\
	__asm(".section .rodata");					\
	q t n(APTR);
# define PROCGW(q,t,n,f)						\
	__asm(#n "=" #f "+1");						\
	q t n(void);
# define INTERRUPT_NODE_TYPE NT_INTERRUPT

#elif defined(__AROS__)

# include <aros/asmcall.h>
# define INTGW(q,t,n,f)							\
	q AROS_UFH3(t, n,						\
	  AROS_UFHA(APTR, d, A1),					\
	  AROS_UFHA(ULONG, _b, A5),					\
	  AROS_UFHA(struct ExecBase *, sysbase, A6)) {			\
      AROS_USERFUNC_INIT return f(d); AROS_USERFUNC_EXIT }
# define PROCGW(q,t,n,f)						\
	q t n(void) { return f(); }
# define INTERRUPT_NODE_TYPE NT_INTERRUPT

#elif defined(__AMIGAOS4__)

# define INTGW(q,t,n,f) \
    __asm(#n "=" #f ); \
    q t n(APTR);
# define PROCGW(q,t,n,f)						\
	q t n(void) {f();}
# define INTERRUPT_NODE_TYPE NT_EXTINTERRUPT

#elif defined(__amiga__) && defined(__mc68000__)

# define INTGW(q,t,n,f)                                                 \
       q t n(APTR d __asm("a1")) { return f(d); }
# define PROCGW(q,t,n,f)						\
	asm(".stabs \"_" #n "\",11,0,0,0;.stabs \"_" #f "\",1,0,0,0");	\
	/*q*/ t n(void);
# define INTERRUPT_NODE_TYPE NT_INTERRUPT

#else
# error Unknown OS/CPU
#endif

#endif /* AHI_Drivers_Common_library_h */
