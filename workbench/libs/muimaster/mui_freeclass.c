/*
    Copyright � 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/exec.h>

#include "muimaster_intern.h"
#include "support_classes.h"
#include "debug.h"

/*****************************************************************************

    NAME */
        AROS_LH1(VOID, MUI_FreeClass,

/*  SYNOPSIS */
        AROS_LHA(Class *, cl, A0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 14, MUIMaster)

/*  FUNCTION
        Frees a class returned by MUI_GetClass(). This function is
        obsolete. Use MUI_DeleteCustomClass() instead.

    INPUTS
        cl - The pointer to the class.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        MUI_GetClass(), MUI_CreateCustomClass(), MUI_DeleteCustomClass()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    /* CLF_INLIST tells us that this class is a builtin class */
    if (cl->cl_Flags & CLF_INLIST)
    {
        Class *super = cl->cl_Super;

        if (--cl->cl_Dispatcher.h_Data == 0)
        {
              ZUNE_RemoveBuiltinClass(cl, MUIMasterBase);

            if (FreeClass(cl))
            {
                CloseLibrary(MUIMasterBase);
                if (strcmp(super->cl_ID, ROOTCLASS) != 0)
                    MUI_FreeClass(super);
            }
            else
            {
                /* Re-add the class to the list since freeing it failed */
                ZUNE_AddBuiltinClass(cl, MUIMasterBase);

                /* And also increase the reference counter again */
                cl->cl_Dispatcher.h_Data++;
            }
        }

        ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    }
    else
    {
        ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

        CloseLibrary((struct Library *)cl->cl_Dispatcher.h_Data);
    }

    AROS_LIBFUNC_EXIT

} /* MUIA_FreeClass */
