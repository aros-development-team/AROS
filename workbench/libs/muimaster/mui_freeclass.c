/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
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
        Release a class obtained through MUI_GetClass(). For built-in
        classes, update the internal class accounting and attempt to free
        the class when that accounting reaches zero. For external classes,
        close the library stored in the class dispatcher's data field.

    INPUTS
        cl - Non-NULL class pointer obtained through MUI_GetClass().

    RESULT
        None. The function does not report whether a built-in class was
        actually freed.

    NOTES
        Do not use the class after releasing it. This legacy interface
        is not the disposal function for a MUI_CustomClass descriptor;
        use MUI_DeleteCustomClass() for that descriptor.
        The implementation obtains the MUI class semaphore and can wait.

    BUGS
        After FreeClass() successfully frees a built-in class, this
        implementation writes to that class's dispatcher data field.
        This is a use-after-free defect, not a valid class-lifetime rule.

    SEE ALSO
        MUI_GetClass(), MUI_CreateCustomClass(), MUI_DeleteCustomClass()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    /* CLF_INLIST tells us that this class is a builtin class */
    if (cl->cl_Flags & CLF_INLIST)
    {
        Class *super = cl->cl_Super;
        char *count = (char *)cl->cl_Dispatcher.h_Data;

        if (--count == 0)
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
                count++;
            }
        }
        cl->cl_Dispatcher.h_Data = count;

        ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    }
    else
    {
        ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

        CloseLibrary((struct Library *)cl->cl_Dispatcher.h_Data);
    }

    AROS_LIBFUNC_EXIT

} /* MUIA_FreeClass */
