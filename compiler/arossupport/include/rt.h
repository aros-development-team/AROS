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
enum
{
    RTT_ALLOCMEM,
    RTT_ALLOCVEC,
    RTT_PORTS,
    RTT_LIBRARY,
    RTT_FILE,
    RTT_SCREEN, /* Screen before Window, so the windows are closed before the screen */
    RTT_WINDOW,

    /* Don't add anything below here !!! */
    RTT_MAX
};

enum
{
    RTTB_EXEC,
    RTTO_PutMsg,
};

enum
{
    RTTB_DOS,
    RTTO_Read,
    RTTO_Write,
};

enum
{
    RTTB_INTUITION,
    RTTO_OpenScreen,
    RTTO_OpenScreenTags,
    RTTO_OpenScreenTagList,
    RTTO_ScreenToFront,
    RTTO_ScreenToBack,
    RTTO_OpenWindow,
    RTTO_OpenWindowTags,
    RTTO_OpenWindowTagList,
    RTTO_WindowToFront,
    RTTO_WindowToBack,
};

void RT_Init (void);
void RT_Exit (void);
IPTR RT_IntAdd (int rtt, const char * file, int line, ...); /* Add a resource for tracking */
IPTR RT_IntCheck (int rtt, const char * file, int line, int op, ...); /* Check a resource before use */
IPTR RT_IntFree (int rtt, const char * file, int line, ...); /* Stop tracking of a resource */
void RT_IntEnter (const char * functionname, const char * filename, int line);
void RT_IntTrack(int rtt, const char * file, int line, APTR res, ...);
void RT_Leave (void);

/* Add a resource for tracking which must not be freed. */

#   if ENABLE_RT
#	define RT_Add(rtt, args...)       RT_IntAdd (rtt, __FILE__, __LINE__, ##args)
#	define RT_Check(rtt, op, args...) RT_IntCheck (rtt, __FILE__, __LINE__, op, ##args)
#	define RT_Free(rtt, args...)      RT_IntFree (rtt, __FILE__, __LINE__, ##args)
#	define RT_Enter(fn)               RT_IntEnter (fn,__FILE__, __LINE__)
#	define RT_Track(rtt, res...)      RT_IntTrack (rtt, __FILE__, __LINE__, ##res)

#	ifndef PROTO_EXEC_H
#	    include <proto/exec.h>
#	endif
#	undef AllocMem
#	define AllocMem(size,flags)     (APTR)RT_Add(RTT_ALLOCMEM,(size),(flags))
#	undef FreeMem
#	define FreeMem(ptr,size)        (void)RT_Free(RTT_ALLOCMEM,(ptr),(size))
#	undef AllocVec
#	define AllocVec(size,flags)     (APTR)RT_Add(RTT_ALLOCVEC,(size),(flags))
#	undef FreeVec
#	define FreeVec(ptr)             (void)RT_Free(RTT_ALLOCVEC,(ptr))
#	undef OpenLibrary
#	define OpenLibrary(name,ver)    (APTR)RT_Add(RTT_LIBRARY,(name),(ver))
#	undef CloseLibrary
#	define CloseLibrary(lib)        (void)RT_Free(RTT_LIBRARY,(lib))
#	undef CreateMsgPort
#	define CreateMsgPort()          (APTR)RT_Add(RTT_PORT,NULL,0)
#	undef DeleteMsgPort
#	define DeleteMsgPort(mp)        (APTR)RT_Free(RTT_PORT,(mp))
#	undef CreatePort
#	define CreatePort(name,pri)     (APTR)RT_Add(RTT_PORT,(name),(pri))
#	undef DeletePort
#	define DeletePort(mp)           (APTR)RT_Free(RTT_PORT,(mp))
#	undef PutMsg
#	define PutMsg(mp,msg)           (APTR)RT_Check(RTT_PORT,RTTO_PutMsg,(mp),(msg))

#	undef Open
#	define Open(path,mode)          (BPTR)RT_Add(RTT_FILE,(path),(mode))
#	undef Close
#	define Close(fh)                (void)RT_Free(RTT_FILE,(fh))
#	undef Read
#	define Read(fh,buffer,length)   (LONG)RT_Check(RTT_FILE,RTTO_Read,(fh),(buffer),(length))
#	undef Write
#	define Write(fh,buffer,length)  (LONG)RT_Check(RTT_FILE,RTTO_Write,(fh),(buffer),(length))

#	undef OpenScreen
#	define OpenScreen(ns)           (struct Screen *)RT_Add(RTT_SCREEN,RTTO_OpenScreen,(ns))
#	undef OpenScreenTagList
#	define OpenScreenTagList(ns,tl) (struct Screen *)RT_Add(RTT_SCREEN,RTTO_OpenScreenTagList,(ns),(tl))
#	undef OpenScreenTags
#	define OpenScreenTags(ns,tag...) (struct Screen *)RT_Add(RTT_SCREEN,RTTO_OpenScreenTags,(ns),##tag)
#	undef CloseScreen
#	define CloseScreen(s)           (void)RT_Free(RTT_SCREEN,(s))
#	undef ScreenToFront
#	define ScreenToFront(s)         (void)RT_Check(RTT_SCREEN,RTTO_ScreenToFront,(s))
#	undef ScreenToBack
#	define ScreenToBack(s)          (void)RT_Check(RTT_SCREEN,RTTO_ScreenToBack,(s))

#	undef OpenWindow
#	define OpenWindow(nw)           (struct Window *)RT_Add(RTT_WINDOW,RTTO_OpenWindow,(nw))
#	undef OpenWindowTagList
#	define OpenWindowTagList(nw,tl) (struct Window *)RT_Add(RTT_WINDOW,RTTO_OpenWindowTagList,(nw),(tl))
#	undef OpenWindowTags
#	define OpenWindowTags(nw,tag...) (struct Window *)RT_Add(RTT_WINDOW,RTTO_OpenWindowTags,(nw),##tag)
#	undef CloseWindow
#	define CloseWindow(w)           (void)RT_Free(RTT_WINDOW,(w))
#	undef WindowToFront
#	define WindowToFront(w)         (void)RT_Check(RTT_WINDOW,RTTO_WindowToFront,(w))
#	undef WindowToBack
#	define WindowToBack(w)          (void)RT_Check(RTT_WINDOW,RTTO_WindowToBack,(w))
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
