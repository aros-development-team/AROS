/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "datatypes_intern.h"
#include <proto/alib.h>
#include <datatypes/datatypesclass.h>

/*****************************************************************************

    NAME */

	AROS_LH2(VOID, ReleaseDTDrawInfo,

/*  SYNOPSIS */
	AROS_LHA(Object *, o     , A0),
	AROS_LHA(APTR    , handle, A1),

/*  LOCATION */
	struct Library *, DTBase, 22, DataTypes)

/*  FUNCTION

    Release the handle obtained from ObtainDTDrawInfoA(); invokes the object's
    DTM_RELEASEDRAWINFO method sending the dtReleaseDrawInfo message.

    INPUTS

    o       --  pointer to the data type object the drawinfo of which to
                release
    handle  --  handle got from ObtainDTDrawInfoA()

    RESULT

    A private handle that must be passed to ReleaseDTDrawInfo when the
    application is done drawing the object, or NULL if failure.

    TAGS

    PDTA_Screen     --  for pictureclass objects
    ADTA_Screen     --  for animationclass objects

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DrawDTObjectA(), ObtainDTDrawInfo()

    INTERNALS

    HISTORY

    29.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct dtReleaseDrawInfo rdi;

    rdi.MethodID   = DTM_RELEASEDRAWINFO;
    rdi.dtr_Handle = handle;

    DoMethodA(o, (Msg)&rdi);

    AROS_LIBFUNC_EXIT
} /* ReleaseDTDrawInfoA */
