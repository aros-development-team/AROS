
#include <aros/debug.h>
#include <intuition/classes.h>
#include "alib_intern.h"

#include <intuition/classusr.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <stdarg.h>
#include "desktop_intern.h"

#include <intuition/classusr.h>

#include <clib/alib_protos.h>

/*
All of the functions here have been copied from amiga.lib.
This means we can convenient things like DoMethod() in
desktop.library.

*/

void NewList(struct List * list)
{
    NEWLIST(list);
}

IPTR DoMethodA (Object * obj, Msg message)
{

    ASSERT_VALID_PTR(obj);
    if (!obj)
	return 0L;

    ASSERT_VALID_PTR(OCLASS(obj));
    ASSERT_VALID_PTR(message);

    return (CallHookPkt ((struct Hook *)OCLASS(obj), obj, message));
} /* DoMethodA */

ULONG DoMethod(Object * obj, ULONG MethodID, ...)
{
	AROS_SLOWSTACKMETHODS_PRE(MethodID)

	ASSERT_VALID_PTR(obj);
    if (!obj)
    {
    	retval = 0L;
    }
    else
    {
		ASSERT_VALID_PTR(OCLASS(obj));
		retval = CallHookPkt ((struct Hook *)OCLASS(obj), obj, AROS_SLOWSTACKMETHODS_ARG(MethodID));
    }
    AROS_SLOWSTACKMETHODS_POST
} /* DoMethod */

IPTR DoSuperMethodA (Class  * cl, Object * obj, Msg	 message)
{
    if ((!obj) || (!cl))
        return 0L;
    return CallHookPkt ((struct Hook *)cl->cl_Super, obj, message);
} /* DoSuperMethodA */

ULONG DoSuperMethod(Class * cl, Object * obj, ULONG MethodID, ...)
{
    AROS_SLOWSTACKMETHODS_PRE(MethodID)
    if ((!obj) || (!cl))
        retval = 0L;
    else
        retval = CallHookPkt ((struct Hook *)cl->cl_Super
            , obj
            , AROS_SLOWSTACKMETHODS_ARG(MethodID)
        );
    AROS_SLOWSTACKMETHODS_POST
} /* DoSuperMethod */

#define AROS_TAGRETURNTYPE APTR
#define NO_INLINE_STDARG /* turn off inline def */

APTR NewObject(struct IClass *classPtr, UBYTE *classID, ULONG tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = NewObjectA (classPtr, classID, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* NewObject */

IPTR SetAttrs (

/*  SYNOPSIS */
	APTR  object,
	ULONG tag1,
	...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SetAttrsA (object, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SetAttrs */


Object * MUI_NewObject(char * classname, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = MUI_NewObjectA(classname, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST

} /* MUI_NewObject */


