#ifndef  CLIB_ALIB_PROTOS_H
#define  CLIB_ALIB_PROTOS_H

/*
**	$VER: alib_protos.h 1.0 (26.10.95)
**
**	C prototypes for things in amiga.lib.
**
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
#ifndef  DOS_DOS_H
#   include <dos/dos.h>
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
ULONG DoMethodA (Object * obj, Msg message);
ULONG DoMethod (Object * obj, ULONG MethodID, ...);
ULONG DoGadgetMethod (struct Gadget * gad, struct Window * win,
		    struct Requester * req, ULONG MethodID, ...);
ULONG DoSuperMethodA (Class  * cl, Object * obj, Msg message);
ULONG DoSuperMethod (Class * cl, Object * obj, ULONG MethodID, ...);
ULONG CoerceMethod (Class * cl, Object * obj, ULONG MethodID, ...);
ULONG CoerceMethodA (Class * cl, Object * obj, Msg msg);
void SetAttrs (Object * obj, ULONG tag1, ...);
APTR NewObject (struct IClass * classPtr, UBYTE * classID, ULONG tag1, ...);

struct Window * OpenWindowTags (struct NewWindow * newWindow, ULONG tag1, ...);
struct Screen * OpenScreenTags (struct NewScreen * newScreen, ULONG tag1, ...);

BOOL ReadByte	 (BPTR fh, UBYTE  * dataptr);
BOOL ReadWord	 (BPTR fh, UWORD  * dataptr);
BOOL ReadLong	 (BPTR fh, ULONG  * dataptr);
BOOL ReadFloat	 (BPTR fh, FLOAT  * dataptr);
BOOL ReadDouble  (BPTR fh, DOUBLE * dataptr);
BOOL ReadString  (BPTR fh, STRPTR * dataptr);
BOOL ReadStruct  (BPTR fh, APTR   * dataptr, IPTR * desc);
BOOL WriteByte	 (BPTR fh, UBYTE  data);
BOOL WriteWord	 (BPTR fh, UWORD  data);
BOOL WriteLong	 (BPTR fh, ULONG  data);
BOOL WriteFloat  (BPTR fh, FLOAT  data);
BOOL WriteDouble (BPTR fh, DOUBLE data);
BOOL WriteString (BPTR fh, STRPTR data);
BOOL WriteStruct (BPTR fh, APTR   data, IPTR * desc);
void FreeStruct  (APTR s,  IPTR * desc);

AROS_UFH3(IPTR, HookEntry,
    AROS_UFHA(struct Hook *, hook,  A0),
    AROS_UFHA(APTR,          obj,   A2),
    AROS_UFHA(APTR,          param, A1)
);

#ifdef AROS_SLOWSTACKMETHODS
    Msg  GetMsgFromStack  (ULONG MethodID, va_list args);
    void FreeMsgFromStack (Msg msg);

#   define AROS_SLOWSTACKMETHODS_PRE(arg)       \
    ULONG   retval;				\
    va_list args;				\
    Msg     msg;				\
						\
    va_start (args, arg);                       \
						\
    if ((msg = GetMsgFromStack (arg, args)))    \
    {						\
	retval =
#   define AROS_SLOWSTACKMETHODS_ARG(arg) msg
#   define AROS_SLOWSTACKMETHODS_POST		\
	FreeMsgFromStack (msg);                 \
    }						\
    else					\
	retval = 0L; /* fail :-/ */		\
						\
    va_end (args);                              \
						\
    return retval;
#else
#   define AROS_SLOWSTACKMETHODS_PRE(arg) return
#   define AROS_SLOWSTACKMETHODS_ARG(arg) ((Msg)(arg))
#   define AROS_SLOWSTACKMETHODS_POST
#endif /* AROS_SLOWSTACKMETHODS */

#ifdef AROS_SLOWSTACKTAGS
    struct TagItem * GetTagsFromStack  (ULONG firstTag, va_list args);
    void	     FreeTagsFromStack (struct TagItem * tags);
#endif /* AROS_SLOWSTACKTAGS */

#endif /* CLIB_ALIB_PROTOS_H */
