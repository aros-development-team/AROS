#ifndef  CLIB_ALIB_PROTOS_H
#define  CLIB_ALIB_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for amiga.lib
    Lang: english
*/

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef LIBRARIES_COMMODITIES_H
#   include <libraries/commodities.h>
#endif
#ifdef AROS_SLOWSTACKTAGS
#   include <stdarg.h>
#   ifndef UTILITY_TAGITEM_H
#	include <utility/tagitem.h>
#   endif
#endif
#ifdef AROS_SLOWSTACKMETHODS
#   ifndef AROS_SLOWSTACKTAGS
#	include <stdarg.h>
#   endif
#endif
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

/*
    Prototypes
*/
IPTR DoMethodA (Object * obj, Msg message);
IPTR DoMethod (Object * obj, ULONG MethodID, ...);
IPTR DoGadgetMethod (struct Gadget * gad, struct Window * win,
		    struct Requester * req, ULONG MethodID, ...);
IPTR DoSuperMethodA (Class  * cl, Object * obj, Msg message);
IPTR DoSuperMethod (Class * cl, Object * obj, ULONG MethodID, ...);
IPTR CoerceMethod (Class * cl, Object * obj, ULONG MethodID, ...);
IPTR CoerceMethodA (Class * cl, Object * obj, Msg msg);
ULONG SetAttrs (Object * obj, ULONG tag1, ...);
ULONG SetSuperAttrs (Class * cl, Object * obj, ULONG tag1, ...);
APTR NewObject (struct IClass * classPtr, UBYTE * classID, ULONG tag1, ...);
IPTR CallHookA (struct Hook * hook, APTR obj, APTR param);
IPTR CallHook (struct Hook * hook, APTR obj, ...);

struct Window * OpenWindowTags (struct NewWindow * newWindow, ULONG tag1, ...);
struct Screen * OpenScreenTags (struct NewScreen * newScreen, ULONG tag1, ...);

/* Exec support */
struct IORequest * CreateExtIO (struct MsgPort * port, ULONG iosize);
struct IOStdReq * CreateStdIO (struct MsgPort * port);
void DeleteExtIO (struct IORequest * ioreq);
void DeleteStdIO (struct IOStdReq * ioreq);
struct MsgPort * CreatePort (STRPTR name, LONG pri);
void DeletePort (struct MsgPort * mp);
struct Task * CreateTask (STRPTR name, LONG pri, APTR initpc, ULONG stacksize);
void DeleteTask (struct Task * task);
void NewList (struct List *);

/* Extra */
ULONG RangeRand (ULONG maxValue);
ULONG FastRand (ULONG seed);
LONG TimeDelay (LONG unit, ULONG secs, ULONG microsecs);
void waitbeam (LONG pos);

/* Commodities */
CxObj * HotKey (STRPTR description, struct MsgPort *port, LONG id);
void FreeIEvents (volatile struct InputEvent *events);

/* Pools */
APTR LibCreatePool (ULONG requirements, ULONG puddleSize, ULONG threshSize);
void LibDeletePool (APTR poolHeader);
APTR LibAllocPooled (APTR poolHeader, ULONG memSize);
void LibFreePooled (APTR poolHeader, APTR memory, ULONG memSize);

AROS_UFH3(IPTR, HookEntry,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR,          obj,   A2),
    AROS_UFHA(APTR,          param, A1)
);

#ifndef AROS_METHODRETURNTYPE
#   define AROS_METHODRETURNTYPE IPTR
#endif
#ifdef AROS_SLOWSTACKMETHODS
    Msg  GetMsgFromStack  (ULONG MethodID, va_list args);
    void FreeMsgFromStack (Msg msg);

#   define AROS_SLOWSTACKMETHODS_PRE(arg)       \
    AROS_METHODRETURNTYPE retval;		\
						\
    va_list args;				\
    Msg     msg;				\
						\
    va_start (args, arg);                       \
						\
    if ((msg = GetMsgFromStack (arg, args)))    \
    {						\
#   define AROS_SLOWSTACKMETHODS_ARG(arg) msg
#   define AROS_SLOWSTACKMETHODS_POST		\
	FreeMsgFromStack (msg);                 \
    }						\
    else					\
	retval = (AROS_METHODRETURNTYPE)0L;     \
						\
    va_end (args);                              \
						\
    return retval;
#else
#   define AROS_SLOWSTACKMETHODS_PRE(arg)   AROS_METHODRETURNTYPE retval;
#   define AROS_SLOWSTACKMETHODS_ARG(arg)   ((Msg)&(arg))
#   define AROS_SLOWSTACKMETHODS_POST	    return retval;
#endif /* AROS_SLOWSTACKMETHODS */

#ifdef AROS_SLOWSTACKTAGS
    struct TagItem * GetTagsFromStack  (ULONG firstTag, va_list args);
    void	     FreeTagsFromStack (struct TagItem * tags);
#endif /* AROS_SLOWSTACKTAGS */

#endif /* CLIB_ALIB_PROTOS_H */
