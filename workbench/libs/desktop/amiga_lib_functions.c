/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$ 
*/

#define NO_INLINE_STDARG   /* turn off inline def */

#include <aros/asmcall.h>

#include <intuition/classusr.h>
#include <intuition/classes.h>


#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <stdarg.h>
#include "alib_intern.h"
#include "desktop_intern.h"

#include <clib/alib_protos.h>

#define DEBUG 1
#include <aros/debug.h>

/*
    All of the functions here have been copied from amiga.lib. This means we
    can convenient things like DoMethod() in desktop.library. 
*/

static ULONG dbgDoMethodCount     = 0;
static ULONG dbgDoMethodNULLCount = 0;

IPTR DoMethodA(Object * obj, Msg message)
{
    dbgDoMethodCount++;
    D(bug("*** DoMethod count: %d\n", dbgDoMethodCount));
    ASSERT_VALID_PTR(obj);
    if (!obj)
    {
        dbgDoMethodNULLCount++;
        D(bug("*** DoMethodNULL count: %d\n", dbgDoMethodNULLCount));
        return 0L;
    }
    ASSERT_VALID_PTR(OCLASS(obj));
    ASSERT_VALID_PTR(message);

    return (CallHookPkt((struct Hook *) OCLASS(obj), obj, message));
} /* DoMethodA */

ULONG DoMethod(Object * obj, ULONG MethodID, ...)
{
    AROS_SLOWSTACKMETHODS_PRE(MethodID)
    
    dbgDoMethodCount++;
    D(bug("*** DoMethod count: %d\n", dbgDoMethodCount));
    
    ASSERT_VALID_PTR(obj);
    
    if (!obj)
    {
        dbgDoMethodNULLCount++;
        D(bug("*** DoMethodNULL count: %d\n", dbgDoMethodNULLCount));
        retval = 0L;
    }
    else
    {
        ASSERT_VALID_PTR(OCLASS(obj));
        retval = CallHookPkt
        (
            (struct Hook *) OCLASS(obj), obj, 
            AROS_SLOWSTACKMETHODS_ARG(MethodID)
        );
    }
    
    AROS_SLOWSTACKMETHODS_POST
} /* DoMethod */

IPTR DoSuperMethodA(Class * cl, Object * obj, Msg message)
{
    if ((!obj) || (!cl))
        return 0L;
    return CallHookPkt((struct Hook *) cl->cl_Super, obj, message);
} /* DoSuperMethodA */

ULONG DoSuperMethod(Class * cl, Object * obj, ULONG MethodID, ...)
{
    AROS_SLOWSTACKMETHODS_PRE(MethodID)
    
    if ((!obj) || (!cl))
        retval = 0L;
    else
        retval = CallHookPkt
        (
            (struct Hook *) cl->cl_Super, obj,
            AROS_SLOWSTACKMETHODS_ARG(MethodID)
        );
        
    AROS_SLOWSTACKMETHODS_POST
} /* DoSuperMethod */

/* Putchar procedure needed by RawDoFmt() */
AROS_UFH2(void, __putChr,
    AROS_UFHA(UBYTE, chr, D0), 
    AROS_UFHA(STRPTR *, p, A3)
)
{
    AROS_USERFUNC_INIT 
    
    *(*p)++ = chr;
    
    AROS_USERFUNC_EXIT
}

VOID __sprintf(UBYTE * buffer, UBYTE * format, ...)
{
    RawDoFmt(format, &format + 1, (VOID_FUNC) __putChr, &buffer);
} /* sprintf */
