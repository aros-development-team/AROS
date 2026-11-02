/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"
#include "support_classes.h"

/*#define MYDEBUG 1*/
#include "debug.h"

/*****************************************************************************

    NAME */
        AROS_LH1(struct IClass *, MUI_GetClass,

/*  SYNOPSIS */
        AROS_LHA(ClassID, classid, A0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 13, MUIMaster)

/*  FUNCTION
        Look up a built-in MUI class or load an external class module.
        Built-in class names are compared case-sensitively. If no built-in
        class is found, the loader tries Zune/<classid>,
        Classes/Zune/<classid>, and MUI/<classid>, in that order, using
        exec.library/OpenLibrary().

    INPUTS
        classid - NUL-terminated class identifier, or NULL.

    RESULT
        The class pointer, or NULL for a NULL identifier or failed lookup.

    NOTES
        This legacy superclass lookup interface is deprecated in favor of
        MUI_CreateCustomClass(). Pair a successful lookup with
        MUI_FreeClass() when the class is no longer needed.
        Loading an external class can perform file I/O and execute the
        module's initialization and MCC_Query entrypoint. Do not call this
        interface from an interrupt handler.

    SEE ALSO
        MUI_FreeClass(), MUI_CreateCustomClass(), MUI_DeleteCustomClass()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    Class *cl;

    if (!classid)
        return NULL;

    cl = ZUNE_GetBuiltinClass(classid, MUIMasterBase);

    if (!cl)
        cl = ZUNE_GetExternalClass(classid, MUIMasterBase);

    return cl;

    AROS_LIBFUNC_EXIT

} /* MUIA_GetClass */

