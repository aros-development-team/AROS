/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Register a program as the Workbench Application.
    Lang: english
*/

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH1(BOOL   , RegisterWorkbench,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, msgport, A0),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 23, Workbench)

/*  FUNCTION
        The Workbench Application uses this function to register itself with
        the library and intuition. When it has done this, the library and
        intuition will send notification messages to this message port about
        actions the Workbench Application is supposed to carry out.

        All messages sent to the message port are of struct IntuiMessage
        with the Class field set to WBENCHMESSAGE and other fields more or
        less abused to send information. The Code field of the message is used
        to identify the different actions and what the other fields contain.

        The following Codes can be used:

            WBENCHOPEN
                Intuition has (re)opened the Workbench Screen, and request that
                you open all your windows. When the message is replied, Intuition
                assumes that the windows have been opened.

            WBENCHCLOSE
                Intuition is about to close the Workbench Screen, and request that
                you close all your windows. When the message is replied, Intuition
                assumes that the windows have been closed and will try to close the
                screen.

    INPUTS
        msgport - The MsgPort of the Workbench Application to send the notification
                  messages to.

    RESULT
        TRUE if the message port was successfully registered, FALSE otherwise.
        The registration will fail if an other message port has already been
        registered earlier or if a NULL pointer was passed in.

    NOTES
        As you can read above, only one Workbench Application can be registered
        at a time. This is intentional. Note that "Workbench Application" in
        this context means the program that is the file manager and handles
        the GUI of Workbench, not a program that is started from Workbench!

        Also, before the Workbench Application terminates, you *must* call
        UnregisterWorkbench()!

    EXAMPLE

    BUGS

    SEE ALSO
        UnregisterWorkbench()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    if( (msgport != NULL) && (WorkbenchBase->wb_AppPort == NULL) ) {
        AlohaWorkbench( msgport );
        WorkbenchBase->wb_AppPort = msgport;

        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* RegisterWorkbench */
