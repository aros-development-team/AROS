#ifndef  CLIB_ALIB_PROTOS_H
#define  CLIB_ALIB_PROTOS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Prototypes for amiga.lib
    Lang: english
*/

#if defined(RT_ENABLE) && RT_ENABLE
#include <aros/rt.h>
#endif

#include <aros/asmcall.h>
#include <devices/inputevent.h>
#include <devices/keymap.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/commodities.h>
#include <libraries/gadtools.h>
#include <rexx/storage.h>
#include <utility/hooks.h>

__BEGIN_DECLS

struct MsgPort;
struct IORequest;
struct Task;
struct InputEvent;
struct Locale;
/*
    Prototypes
*/
IPTR CallHookA (struct Hook * hook, APTR obj, APTR param);
IPTR CallHook (struct Hook * hook, APTR obj, ...) __stackparm;
UBYTE * ACrypt(UBYTE * buffer, const UBYTE * password, const UBYTE * user);

/* Dos support */
BPTR ErrorOutput(void);
BPTR SelectErrorOutput(BPTR fh);

/* Exec support */
VOID BeginIO (struct IORequest *ioReq);
struct IORequest * CreateExtIO (struct MsgPort * port, ULONG iosize);
struct IOStdReq * CreateStdIO (struct MsgPort * port);
void DeleteExtIO (struct IORequest * ioreq);
void DeleteStdIO (struct IOStdReq * ioreq);
struct Task * CreateTask (STRPTR name, LONG pri, APTR initpc, ULONG stacksize);
void DeleteTask (struct Task * task);
void NewList (struct List *);
#if !defined(ENABLE_RT) || !ENABLE_RT
struct MsgPort * CreatePort (STRPTR name, LONG pri);
void DeletePort (struct MsgPort * mp);
#endif

/* Extra */
ULONG RangeRand (ULONG maxValue);
ULONG FastRand (ULONG seed);
LONG TimeDelay (LONG unit, ULONG secs, ULONG microsecs);
void waitbeam (LONG pos);
void __sprintf(UBYTE *buffer, const UBYTE *format, ...);
STRPTR StrDup(CONST_STRPTR str);
APTR ReAllocVec(APTR oldmem, ULONG size, ULONG requirements);

void MergeSortList(struct MinList *l, int (*compare)(struct MinNode *n1, struct MinNode *n2, void *data), void *data);

/* Commodities */
CxObj  *HotKey (STRPTR description, struct MsgPort *port, LONG id);
VOID    FreeIEvents (volatile struct InputEvent *events);
UBYTE **ArgArrayInit(ULONG argc, UBYTE **argv);
VOID    ArgArrayDone(VOID);
LONG    ArgInt(UBYTE **tt, STRPTR entry, LONG defaultVal);
STRPTR  ArgString(UBYTE **tt, STRPTR entry, STRPTR defaultstring);
struct  InputEvent *InvertString(STRPTR str, struct KeyMap *km);
struct  InputEvent *InvertStringForwd(STRPTR str, struct KeyMap *km);

/* Graphics */
#ifndef ObtainBestPen
LONG ObtainBestPen( struct ColorMap * cm, LONG R, LONG G, LONG B, ULONG tag1, ...) __stackparm;
#endif

#ifndef GetRPAttrs
void GetRPAttrs( struct RastPort * rp, Tag tag1, ...) __stackparm;
#endif

BOOL AndRectRect(struct Rectangle *rect1, struct Rectangle *rect2, struct Rectangle *intersect);
struct Region *CopyRegion(struct Region *region);
struct Region *NewRectRegion(WORD MinX,	WORD MinY, WORD MaxX, WORD MaxY);

/* Intuition */
#ifndef SetWindowPointer 
void SetWindowPointer( struct Window * window, ULONG tag1, ...) __stackparm;
#endif

/* BOOPSI */
IPTR DoMethodA (Object * obj, Msg message);
IPTR DoMethod (Object * obj, STACKULONG MethodID, ...) __stackparm;
IPTR DoSuperMethodA (Class * cl, Object * obj, Msg message);
IPTR DoSuperMethod (Class * cl, Object * obj, STACKULONG MethodID, ...) __stackparm;
IPTR CoerceMethodA (Class * cl, Object * obj, Msg message);
IPTR CoerceMethod (Class * cl, Object * obj, STACKULONG MethodID, ...) __stackparm;
IPTR DoSuperNewTagList(Class *CLASS, Object *object, struct GadgetInfo *gadgetInfo, struct TagItem *tags);
IPTR DoSuperNewTags(Class *CLASS, Object *object, struct GadgetInfo *gadgetInfo, Tag tag1, ...) __stackparm;
IPTR SetSuperAttrs (Class * cl, Object * obj, Tag tag1, ...) __stackparm;

/* Locale */
#ifndef OpenCatalog
struct Catalog *OpenCatalog(const struct Locale * locale,
				CONST_STRPTR name,
				Tag tag1,
				...) __stackparm;
#endif

/* Pools */
APTR LibCreatePool (ULONG requirements, ULONG puddleSize, ULONG threshSize);
void LibDeletePool (APTR poolHeader);
APTR LibAllocPooled (APTR poolHeader, ULONG memSize);
void LibFreePooled (APTR poolHeader, APTR memory, ULONG memSize);

/* Aligned */
APTR LibAllocAligned (ULONG memSize, ULONG requirements, IPTR alignBytes);

/* Hook Support */
AROS_UFP3(IPTR, HookEntry,
    AROS_UFPA(struct Hook *, hook,  A0),
    AROS_UFPA(APTR,          obj,   A2),
    AROS_UFPA(APTR,          param, A1)
);

#ifndef AROS_METHODRETURNTYPE
#   define AROS_METHODRETURNTYPE IPTR
#endif

#ifdef AROS_SLOWSTACKMETHODS
    Msg  GetMsgFromStack  (IPTR MethodID, va_list args);
    void FreeMsgFromStack (Msg msg);

#   define AROS_NR_SLOWSTACKMETHODS_PRE(arg)    \
    va_list args;				\
    Msg     msg;				\
						\
    va_start (args, arg);                       \
						\
    if ((msg = GetMsgFromStack (arg, args)))    \
    {

#   define AROS_SLOWSTACKMETHODS_PRE_AS(arg, type) \
    type retval;				\
						\
    va_list args;				\
    Msg     msg;				\
						\
    va_start (args, arg);                       \
						\
    if ((msg = GetMsgFromStack (arg, args)))    \
    {

#   define AROS_SLOWSTACKMETHODS_PRE(arg)       \
	AROS_SLOWSTACKMETHODS_PRE_AS(arg, AROS_METHODRETURNTYPE)

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

#   define AROS_NR_SLOWSTACKMETHODS_POST	\
	FreeMsgFromStack (msg);                 \
    }						\
						\
    va_end (args);
#else
#   define AROS_NR_SLOWSTACKMETHODS_PRE(arg)
#   define AROS_SLOWSTACKMETHODS_PRE(arg)   AROS_METHODRETURNTYPE retval;
#   define AROS_SLOWSTACKMETHODS_PRE_AS(arg, type)   type retval;
#   define AROS_SLOWSTACKMETHODS_ARG(arg)   ((Msg)&(arg))
#   define AROS_SLOWSTACKMETHODS_POST	    return retval;
#   define AROS_NR_SLOWSTACKMETHODS_POST
#endif /* AROS_SLOWSTACKMETHODS */

#ifdef AROS_SLOWSTACKTAGS
    struct TagItem * GetTagsFromStack  (IPTR firstTag, va_list args);
    void	     FreeTagsFromStack (struct TagItem * tags);
#endif /* AROS_SLOWSTACKTAGS */

#ifndef AROS_HOOKRETURNTYPE
#   define AROS_HOOKRETURNTYPE IPTR
#endif

#ifdef AROS_SLOWSTACKHOOKS
    APTR  GetParamsFromStack  (va_list args);
    void FreeParamsFromStack (APTR params);

#   define AROS_NR_SLOWSTACKHOOKS_PRE(arg)    \
    va_list args;				\
    APTR     params;				\
						\
    va_start (args, arg);                       \
						\
    if ((params = GetParamsFromStack (args)))    \
    {

#   define AROS_SLOWSTACKHOOKS_PRE(arg)       \
    AROS_HOOKRETURNTYPE retval;		\
						\
    va_list args;				\
    APTR     params;				\
						\
    va_start (args, arg);                       \
						\
    if ((params = GetParamsFromStack (args)))    \
    {

#   define AROS_SLOWSTACKHOOKS_ARG(arg) params

#   define AROS_SLOWSTACKHOOKS_POST		\
	FreeParamsFromStack (params);                 \
    }						\
    else					\
	retval = (AROS_HOOKRETURNTYPE)0L;     \
						\
    va_end (args);                              \
						\
    return retval;

#   define AROS_NR_SLOWSTACKHOOKS_POST	\
	FreeParamsFromStack (params);                 \
    }						\
						\
    va_end (args);
#else
#   define AROS_NR_SLOWSTACKHOOKS_PRE(arg)
#   define AROS_SLOWSTACKHOOKS_PRE(arg)   AROS_HOOKRETURNTYPE retval;
#   define AROS_SLOWSTACKHOOKS_ARG(arg)   ((IPTR*)&(arg)+1)
#   define AROS_SLOWSTACKHOOKS_POST	    return retval;
#   define AROS_NR_SLOWSTACKHOOKS_POST
#endif /* AROS_SLOWSTACKHOOKS */

/* Rexx support */
BOOL CheckRexxMsg(struct RexxMsg *);
LONG SetRexxVar(struct RexxMsg *, CONST_STRPTR var, char *value, ULONG length);
LONG GetRexxVar(struct RexxMsg *, CONST_STRPTR var, char **value);

/* Inline versions of varargs functions */
#if !defined(ALIB_NO_INLINE_STDARG) && !defined(NO_INLINE_STDARG)

#    define SetSuperAttrsA(cl, object, attrs)          	  \
     ({                                                   \
         struct opSet __ops;                              \
                                                          \
         __ops.MethodID     = OM_SET;                     \
         __ops.ops_AttrList = (attrs);                    \
         __ops.ops_GInfo    = NULL;                       \
                                                          \
         DoSuperMethodA((cl), (object), (Msg) &__ops.MethodID); \
     })
#    define SetSuperAttrs(cl, object, args...)                         \
     ({                                                                \
         IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(args) };         \
         SetSuperAttrsA((cl), (object), (struct TagItem *) __args);    \
     })
#    define DoMethodA(object, message)                                 \
     ({                                                                \
         (object) != NULL ?                                            \
         ({                                                            \
             CALLHOOKPKT                                               \
             (                                                         \
                 (struct Hook *) OCLASS((object)), (object), (message) \
             );                                                        \
         })                                                            \
         :                                                             \
             0                                                         \
         ;                                                             \
    })
#   define DoMethod(object, methodid, args...)                        \
    ({                                                                \
        IPTR __args[] = {methodid, AROS_PP_VARIADIC_CAST2IPTR(args)}; \
        DoMethodA((object), __args);                                  \
    })
            
#   define DoSuperMethodA(cl, object, message)                        \
    ({                                                                \
        ((cl) != NULL && (object) != NULL) ?                          \
            CALLHOOKPKT                                               \
            (                                                         \
                (struct Hook *) ((Class *) (cl))->cl_Super,           \
                (object), (message)                                   \
            )                                                         \
        :                                                             \
            0                                                         \
        ;                                                             \
    })
#   define DoSuperMethod(cl, object, methodid, args...)               \
    ({                                                                \
        IPTR __args[] = {methodid, AROS_PP_VARIADIC_CAST2IPTR(args)}; \
        DoSuperMethodA((cl), (object), __args);                       \
    })
            
#   define CoerceMethodA(cl, object, message)                         \
    ({                                                                \
        ((cl) != NULL && (object) != NULL) ?                          \
            CALLHOOKPKT((struct Hook *) (cl), (object), (message))    \
        :                                                             \
            0                                                         \
        ;                                                             \
    })
#   define CoerceMethod(cl, object, methodid, args...)                 \
    ({                                                                 \
         IPTR __args[] = {methodid, AROS_PP_VARIADIC_CAST2IPTR(args)}; \
         CoerceMethodA((cl), (object), __args);                        \
    })  
#   define DoSuperNewTagList(cl, object, gadgetinfo, tags)                 \
    ({                                                                     \
        struct opSet __ops;                                                \
                                                                           \
        __ops.MethodID     = OM_NEW;                                       \
        __ops.ops_AttrList = (tags);                                       \
        __ops.ops_GInfo    = (gadgetinfo);                                 \
                                                                           \
        (cl) != NULL && (object) != NULL ?                                 \
            DoSuperMethodA((cl), (object), (Msg)&__ops.MethodID)           \
        :                                                                  \
            0                                                              \
        ;                                                                  \
    })
#   define DoSuperNewTags(cl, object, gadgetinfo, args...)                 \
    ({                                                                     \
        IPTR __args[] = {AROS_PP_VARIADIC_CAST2IPTR(args)};                \
        DoSuperNewTagList                                                  \
        (                                                                  \
            (cl), (object), (gadgetinfo), (struct TagItem *) __args        \
        );                                                                 \
    })

#define CallHook(hook, object, args...)					\
    ({									\
    	IPTR __args[] = {AROS_PP_VARIADIC_CAST2IPTR(args)};		\
    	CallHookA((hook), (object), __args);				\
    })

#endif /* !ALIB_NO_INLINE_STDARG && !NO_INLINE_STDARG */

__END_DECLS

#endif /* CLIB_ALIB_PROTOS_H */
