/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#define NO_INLINE_STDARG        /* turn off inline def */

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

void NewList(struct List *list)
{
    NEWLIST(list);
}

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
}                               /* DoMethodA */

ULONG DoMethod(Object * obj, ULONG MethodID, ...)
{
    dbgDoMethodCount++;
    D(bug("*** DoMethod count: %d\n", dbgDoMethodCount));
    AROS_SLOWSTACKMETHODS_PRE(MethodID) ASSERT_VALID_PTR(obj);
    if (!obj)
    {
        dbgDoMethodNULLCount++;
        D(bug("*** DoMethodNULL count: %d\n", dbgDoMethodNULLCount));
        retval = 0L;
    }
    else
    {
        ASSERT_VALID_PTR(OCLASS(obj));
        retval =
            CallHookPkt((struct Hook *) OCLASS(obj), obj,
                        AROS_SLOWSTACKMETHODS_ARG(MethodID));
    }
AROS_SLOWSTACKMETHODS_POST}     /* DoMethod */

IPTR DoSuperMethodA(Class * cl, Object * obj, Msg message)
{
    if ((!obj) || (!cl))
        return 0L;
    return CallHookPkt((struct Hook *) cl->cl_Super, obj, message);
}                               /* DoSuperMethodA */

ULONG DoSuperMethod(Class * cl, Object * obj, ULONG MethodID, ...)
{
    AROS_SLOWSTACKMETHODS_PRE(MethodID) if ((!obj) || (!cl))
        retval = 0L;
    else
        retval =
            CallHookPkt((struct Hook *) cl->cl_Super, obj,
                        AROS_SLOWSTACKMETHODS_ARG(MethodID));
AROS_SLOWSTACKMETHODS_POST}     /* DoSuperMethod */

#define AROS_TAGRETURNTYPE APTR

APTR NewObject(struct IClass *classPtr, UBYTE * classID, ULONG tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
        retval = NewObjectA(classPtr, classID, AROS_SLOWSTACKTAGS_ARG(tag1));
AROS_SLOWSTACKTAGS_POST}        /* NewObject */

IPTR SetAttrs(
             /*
                SYNOPSIS 
              */
                 APTR object, ULONG tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
        retval = SetAttrsA(object, AROS_SLOWSTACKTAGS_ARG(tag1));
AROS_SLOWSTACKTAGS_POST}        /* SetAttrs */


Object         *MUI_NewObject(char *classname, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
        retval = MUI_NewObjectA(classname, AROS_SLOWSTACKTAGS_ARG(tag1));

AROS_SLOWSTACKTAGS_POST}        /* MUI_NewObject */


Object         *MUI_MakeObject(LONG type, ...)
{
    va_list         args;
    IPTR            param[4];
    WORD            i,
                    numparams = 0;

    switch (type)
    {
        case 2:                /* MUIO_Button */
        case 3:                /* MUIO_CheckMark */
        case 8:                /* MUIO_PopButton */
        case 9:                /* MUIO_HSpace */
        case 10:               /* MUIO_VSpace */
        case 11:               /* MUIO_HBar */
        case 12:               /* MUIO_VBar */
        case 15:               /* MUIO_BarTitle */
            numparams = 1;
            break;

        case 1:                /* MUIO_Label */
        case 4:                /* MUIO_Cycle */
        case 5:                /* MUIO_Radio */
        case 7:                /* MUIO_String */
        case 13:               /* MUIO_MenustripNM */
            numparams = 2;
            break;

        case 6:                /* MUIO_Slider */
        case 111:              /* MUIO_CoolButton */
            numparams = 3;
            break;

        case 14:               /* MUIO_MenuItem */
        case 16:               /* MUIO_NumericButton */
            numparams = 4;
            break;
    }

    if (numparams == 0)
        return NULL;

    va_start(args, type);

    for (i = 0; i < numparams; i++)
    {
        param[i] = va_arg(args, IPTR);
    }

    va_end(args);

    return MUI_MakeObjectA(type, param);

}                               /* MUI_MakeObject */



/*
   Putchar procedure needed by RawDoFmt() 
 */
AROS_UFH2(void, __putChr,
          AROS_UFHA(UBYTE, chr, D0), AROS_UFHA(STRPTR *, p, A3))
{
    AROS_LIBFUNC_INIT * (*p)++ = chr;
AROS_LIBFUNC_EXIT}

VOID __sprintf(UBYTE * buffer, UBYTE * format, ...)
{
    RawDoFmt(format, &format + 1, (VOID_FUNC) __putChr, &buffer);
}                               /* sprintf */
