#ifndef MUIMASTER_INTERN_H
#define MUIMASTER_INTERN_H

/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Internal definitions for muimaster.library
    Lang: English
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifdef _AROS
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#else
#define AROS_LIBFUNC_INIT
#define AROS_LIBBASE_EXT_DECL(a,b) extern a b;
#define AROS_LIBFUNC_EXIT

typedef unsigned long IPTR;
typedef long STACKLONG;
typedef unsigned long STACKULONG;

#endif

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

/****************************************************************************************/

struct MUIMasterBase_intern
{
    struct Library		library;
    struct ExecBase		*sysbase;
    BPTR			seglist;

    struct UtilityBase		*utilitybase;

    struct SignalSemaphore ClassSempahore;
    struct IClass **Classes;
    int     ClassCount;
    int     ClassSpace;
};

/****************************************************************************************/

#undef MUIMB
#define MUIMB(b)		((struct MUIMasterBase_intern *)b)

#undef SysBase
#define SysBase     (((struct MUIMasterBase_intern*)MUIMasterBase)->sysbase)

#undef UtilityBase
#define UtilityBase	(((struct MUIMasterBase_intern*)MUIMasterBase)->utilitybase)

/****************************************************************************************/

#ifndef _AROS
struct __MUIBuiltinClass {
    CONST_STRPTR name;
    CONST_STRPTR supername;
    ULONG        datasize;
    ULONG	   (*dispatcher)();
};

#else
struct __MUIBuiltinClass {
    CONST_STRPTR name;
    CONST_STRPTR supername;
    ULONG        datasize;

    AROS_UFP3(IPTR, (*dispatcher),
        AROS_UFPA(Class  *,  cl, A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg     , msg, A1));
};
#endif

#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright (obj)) \
                          && _between(_mtop(obj) ,(y),_mbottom(obj)))

/* add mask in flags if tag is true, else sub mask */
#define _handle_bool_tag(flags, tag, mask) \
((tag) ? ((flags) |= (mask)) : ((flags) &= ~(mask)))

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#endif /* MUIMASTER_INTERN_H */
