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

/* Resources */
#   define RTT_ALLOCMEM     0
#   define RTT_ALLOCVEC     1
#   define RTT_LIBRARY	    2
#   define RTT_FILE	    3
#   define RTT_WINDOW	    4
#   define RTT_MAX	    5

#   define RTTB_DOS	    1		    /* Base: DOS operations */
#   define RTTO_Read	    (RTTB_DOS+0)    /* DOS-Op: Read() */
#   define RTTO_Write	    (RTTB_DOS+1)    /* DOS-Op: Write() */

#   define RTTB_INTUITION	1	    /* Base: Intuition ops */
#   define RTTO_WindowToFront	(RTTB_INTUITION+0)
#   define RTTO_WindowToBack	(RTTB_INTUITION+1)

void RT_Init (void);
void RT_Exit (void);
IPTR RT_IntAdd (int rtt, const char * file, int line, ...); /* Add a resource for tracking */
IPTR RT_IntCheck (int rtt, const char * file, int line, int op, ...); /* Check a resource before use */
IPTR RT_IntFree (int rtt, const char * file, int line, ...); /* Stop tracking of a resource */
void RT_IntEnter (const char * functionname, const char * filename, int line);
void RT_Leave (void);

#   if ENABLE_RT
#	define RT_Add(rtt, args...)       RT_IntAdd (rtt, __FILE__, __LINE__, ##args)
#	define RT_Check(rtt, op, args...) RT_IntCheck (rtt, __FILE__, __LINE__, op, ##args)
#	define RT_Free(rtt, args...)      RT_IntFree (rtt, __FILE__, __LINE__, ##args)
#	define RT_Enter(fn)               RT_IntEnter (fn,__FILE__, __LINE__)

#	ifndef PROTO_EXEC_H
#	    include <proto/exec.h>
#	endif
#	undef AllocMem
#	define AllocMem(size,flags)     (APTR)RT_Add(RTT_ALLOCMEM,(ULONG)(size),(ULONG)(flags))
#	undef FreeMem
#	define FreeMem(ptr,size)        (void)RT_Free(RTT_ALLOCMEM,(APTR)(ptr),(ULONG)(size))
#	undef AllocVec
#	define AllocVec(size,flags)     (APTR)RT_Add(RTT_ALLOCVEC,(ULONG)(size),(ULONG)(flags))
#	undef FreeVec
#	define FreeVec(ptr)             (void)RT_Free(RTT_ALLOCVEC,(APTR)(ptr))
#	undef OpenLibrary
#	define OpenLibrary(name,ver)    (APTR)RT_Add(RTT_LIBRARY,(STRPTR)(name),(ULONG)(ver))
#	undef CloseLibrary
#	define CloseLibrary(lib)        (void)RT_Free(RTT_LIBRARY,(struct Library *)(lib))

#	undef Open
#	define Open(path,mode)          (BPTR)RT_Add(RTT_FILE,(STRPTR)(path),(LONG)(mode))
#	undef Close
#	define Close(fh)                (void)RT_Free(RTT_FILE,(BPTR)(fh))
#	undef Read
#	define Read(fh,buffer,length)   (LONG)RT_Check(RTT_FILE,RTTO_Read,(BPTR)(fh),(APTR)(buffer),(LONG)(length))
#	undef Write
#	define Write(fh,buffer,length)  (LONG)RT_Check(RTT_FILE,RTTO_Write,(BPTR)(fh),(APTR)(buffer),(LONG)(length))

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
