#ifndef AROS_RT_H
#define AROS_RT_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    Resource Tracking

*/
#ifndef ENABLE_RT
#   define ENABLE_RT	0
#endif

#if ENABLE_RT || defined(RT_INTERNAL)
#   ifndef EXEC_TYPES_H
#	include <exec/types.h>
#   endif

#   define RTT_ALLOCMEM     0
#   define RTT_ALLOCVEC     1
#   define RTT_MAX	    2

void RT_Init (void);
void RT_Exit (void);
IPTR RT_IntAdd (int rtt, const char * file, int line, ...); /* Add a resource for tracking */
IPTR RT_IntCheck (int rtt, const char * file, int line, ...); /* Check a resource before use */
IPTR RT_IntFree (int rtt, const char * file, int line, ...); /* Stop tracking of a resource */
void RT_IntEnter (const char * functionname, const char * filename, int line);
void RT_Leave (void);

#   if ENABLE_RT
#	define RT_Add(rtt, args...)    RT_IntAdd (rtt, __FILE__, __LINE__, ##args)
#	define RT_Check(rtt, args...)  RT_IntCheck (rtt, __FILE__, __LINE__, ##args)
#	define RT_Free(rtt, args...)   RT_IntFree (rtt, __FILE__, __LINE__, ##args)
#	define RT_Enter(fn)            RT_IntEnter (fn,__FILE__, __LINE__)

#	ifndef PROTO_EXEC_H
#	    include <proto/exec.h>
#	endif
#	undef AllocMem
#	define AllocMem(size,flags)     (APTR)RT_Add(RTT_ALLOCMEM,(ULONG)size,(ULONG)flags)
#	undef FreeMem
#	define FreeMem(ptr,size)        (void)RT_Free(RTT_ALLOCMEM,(APTR)ptr,(ULONG)size)
#	undef AllocVec
#	define AllocVec(size,flags)     (APTR)RT_Add(RTT_ALLOCVEC,(ULONG)size,(ULONG)flags)
#	undef FreeVec
#	define FreeVec(ptr)             (void)RT_Free(RTT_ALLOCVEC,(APTR)ptr)
#   endif
#else
#   define RT_Init()                /* eps */
#   define RT_Exit()                /* eps */
#   define RT_Add(rtt, args...)     /* eps */
#   define RT_Check(rtt, args...)   /* eps */
#   define RT_Free(rtt, args...)    /* eps */
#   define RT_Enter()               /* eps */
#   define RT_Leave()               /* eps */
#endif

#endif /* AROS_RT_H */
