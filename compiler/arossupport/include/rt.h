/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Resource Tracking
    Lang: english

    This file can be included more than once
*/

/* Resource Tracking currently disabled on native platforms */
#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
#   undef ENABLE_RT
#endif

#ifndef ENABLE_RT
#   define ENABLE_RT	0
#endif

#if ENABLE_RT || defined(RT_INTERNAL)
#   ifndef EXEC_TYPES_H
#	include <exec/types.h>
#   endif

#   ifndef AROS_RT_H
#   define AROS_RT_H
    /* Put code which must be defined only once here */

    /* Resources */
    enum
    {
	RTT_ALLOCMEM,
	RTT_ALLOCVEC,
	RTT_PORT,
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
	RTTO_GetMsg,
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

    BEGIN_EXTERN
    void RT_IntInitB (void);
    void RT_IntInitE (void);
    void RT_IntExitB (void);
    void RT_IntExitE (void);
    IPTR RT_IntAdd   (int rtt, const char * file, int line, ...); /* Add a resource for tracking */
    IPTR RT_IntCheck (int rtt, const char * file, int line, int op, ...); /* Check a resource before use */
    IPTR RT_IntFree  (int rtt, const char * file, int line, ...); /* Stop tracking of a resource */
    void RT_IntEnter (const char * functionname, const char * filename, int line);
    void RT_IntTrack (int rtt, const char * file, int line, APTR res, ...);
    void RT_Leave    (void);
    END_EXTERN

#	ifndef RT_INTERNAL
#	    define RT_Add(rtt, args...)       RT_IntAdd (rtt, __FILE__, __LINE__, ##args)
#	    define RT_Check(rtt, op, args...) RT_IntCheck (rtt, __FILE__, __LINE__, op, ##args)
#	    define RT_Free(rtt, args...)      RT_IntFree (rtt, __FILE__, __LINE__, ##args)
#	    define RT_Enter(fn)               RT_IntEnter (fn,__FILE__, __LINE__)
#	    define RT_Track(rtt, res...)      RT_IntTrack (rtt, __FILE__, __LINE__, ##res)
#	endif /* ENABLE_RT */
#   endif /* AROS_RT_H */

/* Add a resource for tracking which must not be freed. */

#   if ENABLE_RT
#	undef RT_INITEXEC
#	undef RT_EXITEXEC

#	if ENABLE_RT_EXEC
#	    ifndef PROTO_EXEC_H
#		include <proto/exec.h>
#	    endif

	    EXTERN void RT_InitExec (void);
	    EXTERN void RT_ExitExec (void);

#	    define RT_INITEXEC		    RT_InitExec(),
#	    define RT_EXITEXEC		    RT_ExitExec(),

#	    undef AllocMem
#	    define AllocMem(size,flags)     (APTR)RT_Add(RTT_ALLOCMEM,(size),(flags))
#	    undef FreeMem
#	    define FreeMem(ptr,size)        (void)RT_Free(RTT_ALLOCMEM,(ptr),(size))
#	    undef AllocVec
#	    define AllocVec(size,flags)     (APTR)RT_Add(RTT_ALLOCVEC,(size),(flags))
#	    undef FreeVec
#	    define FreeVec(ptr)             (void)RT_Free(RTT_ALLOCVEC,(ptr))
#	    undef OpenLibrary
#	    define OpenLibrary(name,ver)    (APTR)RT_Add(RTT_LIBRARY,(name),(ver))
#	    undef CloseLibrary
#	    define CloseLibrary(lib)        (void)RT_Free(RTT_LIBRARY,(lib))
#	    undef CreateMsgPort
#	    define CreateMsgPort()          (APTR)RT_Add(RTT_PORT,NULL,0)
#	    undef DeleteMsgPort
#	    define DeleteMsgPort(mp)        (void)RT_Free(RTT_PORT,(mp))
#	    undef CreatePort
#	    define CreatePort(name,pri)     (APTR)RT_Add(RTT_PORT,(name),(pri))
#	    undef DeletePort
#	    define DeletePort(mp)           (void)RT_Free(RTT_PORT,(mp))
#	    undef PutMsg
#	    define PutMsg(mp,msg)           (void)RT_Check(RTT_PORT,RTTO_PutMsg,(mp),(msg))
#	    undef GetMsg
#	    define GetMsg(mp)               (struct Message *)RT_Check(RTT_PORT,RTTO_GetMsg,(mp))
#	else
#	    define RT_INITEXEC
#	    define RT_EXITEXEC
#	endif /* ENABLE_RT_EXEC */

#	undef RT_INITDOS
#	undef RT_EXITDOS

#	if ENABLE_RT_DOS
#	    ifndef PROTO_DOS_H
#		include <proto/dos.h>
#	    endif

	    EXTERN void RT_InitDos (void);
	    EXTERN void RT_ExitDos (void);

#	    define RT_INITDOS		    RT_InitDos(),
#	    define RT_EXITDOS		    RT_ExitDos(),

#	    undef Open
#	    define Open(path,mode)          (BPTR)RT_Add(RTT_FILE,(path),(mode))
#	    undef Close
#	    define Close(fh)                (void)RT_Free(RTT_FILE,(fh))
#	    undef Read
#	    define Read(fh,buffer,length)   (LONG)RT_Check(RTT_FILE,RTTO_Read,(fh),(buffer),(length))
#	    undef Write
#	    define Write(fh,buffer,length)  (LONG)RT_Check(RTT_FILE,RTTO_Write,(fh),(buffer),(length))
#	else
#	    define RT_INITDOS
#	    define RT_EXITDOS
#	endif /* ENABLE_RT_DOS */

#	undef RT_INITINTUITION
#	undef RT_EXITINTUITION

#	if ENABLE_RT_INTUITION
#	    ifndef PROTO_INTUITION_H
#		include <proto/intuition.h>
#	    endif

	    EXTERN void RT_InitIntuition (void);
	    EXTERN void RT_ExitIntuition (void);

#	    define RT_INITINTUITION	    RT_InitIntuition(),
#	    define RT_EXITINTUITION	    RT_ExitIntuition(),

#	    undef OpenScreen
#	    define OpenScreen(ns)           (struct Screen *)RT_Add(RTT_SCREEN,RTTO_OpenScreen,(ns))
#	    undef OpenScreenTagList
#	    define OpenScreenTagList(ns,tl) (struct Screen *)RT_Add(RTT_SCREEN,RTTO_OpenScreenTagList,(ns),(tl))
#	    undef OpenScreenTags
#	    define OpenScreenTags(ns,tag...) (struct Screen *)RT_Add(RTT_SCREEN,RTTO_OpenScreenTags,(ns),##tag)
#	    undef CloseScreen
#	    define CloseScreen(s)           (void)RT_Free(RTT_SCREEN,(s))
#	    undef ScreenToFront
#	    define ScreenToFront(s)         (void)RT_Check(RTT_SCREEN,RTTO_ScreenToFront,(s))
#	    undef ScreenToBack
#	    define ScreenToBack(s)          (void)RT_Check(RTT_SCREEN,RTTO_ScreenToBack,(s))

#	    undef OpenWindow
#	    define OpenWindow(nw)           (struct Window *)RT_Add(RTT_WINDOW,RTTO_OpenWindow,(nw))
#	    undef OpenWindowTagList
#	    define OpenWindowTagList(nw,tl) (struct Window *)RT_Add(RTT_WINDOW,RTTO_OpenWindowTagList,(nw),(tl))
#	    undef OpenWindowTags
#	    define OpenWindowTags(nw,tag...) (struct Window *)RT_Add(RTT_WINDOW,RTTO_OpenWindowTags,(nw),##tag)
#	    undef CloseWindow
#	    define CloseWindow(w)           (void)RT_Free(RTT_WINDOW,(w))
#	    undef WindowToFront
#	    define WindowToFront(w)         (void)RT_Check(RTT_WINDOW,RTTO_WindowToFront,(w))
#	    undef WindowToBack
#	    define WindowToBack(w)          (void)RT_Check(RTT_WINDOW,RTTO_WindowToBack,(w))
#	else
#	    define RT_INITINTUITION
#	    define RT_EXITINTUITION
#	endif /* ENABLE_RT_INTUITION */
#   endif /* ENABLE_RT */

#   undef RT_Init
#   define RT_Init()    \
	RT_IntInitB(), \
	RT_INITEXEC \
	RT_INITDOS \
	RT_INITINTUITION \
	RT_IntInitE()

#   undef RT_Exit
#   define RT_Exit()    \
	RT_IntExitB(), \
	RT_EXITINTUITION \
	RT_EXITDOS \
	RT_EXITEXEC \
	RT_IntExitE()

#else /* ENABLE_RT || defined(RT_INTERNAL) */
#   ifndef RT_Init
#	define RT_Init()                /* eps */
#	define RT_Exit()                /* eps */
#	define RT_Add(rtt, args...)     /* eps */
#	define RT_Check(rtt, args...)   /* eps */
#	define RT_Free(rtt, args...)    /* eps */
#	define RT_Enter()               /* eps */
#	define RT_Leave()               /* eps */
#   endif
#endif /* ENABLE_RT || defined(RT_INTERNAL) */
