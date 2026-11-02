/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"

/* #define MYDEBUG 1 */
#include "debug.h"

/*****************************************************************************

    NAME */
        AROS_LH2(Object *, MUI_NewObjectA,

/*  SYNOPSIS */
        AROS_LHA(ClassID, classid,     A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, MUIMasterBase, 5, MUIMaster)

/*  FUNCTION
        Create an instance of the named MUI class with the supplied attributes.
        The class is looked up through MUI_GetClass(), then instantiated
        through intuition.library/NewObjectA().

    INPUTS
        classid - NUL-terminated class identifier. Built-in class names are
                  case-sensitive. Use the MUIC_* identifiers where available.
        tags - Attribute/value pairs terminated by TAG_DONE, or NULL for
               the class defaults. Attribute types and ownership are defined
               by the selected class.

    RESULT
        The new object, or NULL if the class lookup or construction fails.

    NOTES
        MUI_NewObject() is the variadic tag-list wrapper.

        Release an independently owned object with MUI_DisposeObject(). If
        ownership is transferred to a parent, follow that parent's disposal
        contract instead of disposing of the child twice.
        This function does not establish a separate MUI error code.

    SEE ALSO
        MUI_DisposeObject(), MUI_GetClass(),
        intuition.library/NewObjectA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    Class  *cl;

    cl = MUI_GetClass(classid);
    if (cl)
    {
        Object *obj = NewObjectA(cl, NULL, tags);

        if (obj) return obj;

        bug("*** Could not create object of %s\n", classid);
        MUI_FreeClass(cl);
    }

    bug("*** Couldn't find %s\n", classid);

    return NULL;

    AROS_LIBFUNC_EXIT
} /* MUIA_NewObjectA */
