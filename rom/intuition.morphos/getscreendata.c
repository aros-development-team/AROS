/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Get infos about a screen. *OBSOLETE*
*/

#include "intuition_intern.h"
#include <string.h>
#include <proto/exec.h>

/*****************************************************************************
 
    NAME */
#include <intuition/screens.h>
#include <proto/intuition.h>

AROS_LH4(LONG, GetScreenData,

         /*  SYNOPSIS */
         AROS_LHA(APTR           , buffer, A0),
         AROS_LHA(ULONG          , size, D0),
         AROS_LHA(ULONG          , type, D1),
         AROS_LHA(struct Screen *, screen, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 71, Intuition)

/*  FUNCTION
    Copy part or all infos about a screen into a private buffer.
 
    To copy the Workbench, one would call
 
        GetScreenData (buffer, sizeof(struct Screen), WBENCHSCREEN, NULL)
 
    If the screen is not open, this call will open it. You can use
    this function for these purposes:
 
    1) Get information about the workbench in order to open a window
       on it (eg. size).
    2) Clone a screen.
 
    INPUTS
    buffer - The data gets copied here
    size - The size of the buffer in bytes
    type - The type of the screen as in OpenWindow().
    screen - Ignored unless type is CUSTOMSCREEN.
 
    RESULT
    TRUE if successful, FALSE if the screen could not be opened.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_GETSCREENDATA(dprintf("GetScreenData(buffer 0x%lx size 0x%lx type 0x%lx screen 0x%lx)\n",buffer,size,type,screen));

    EXTENDUWORD(size);
    EXTENDUWORD(type);

    if (type == WBENCHSCREEN)
    {
        screen = GetPrivIBase(IntuitionBase)->WorkBench;
    }
    else if (type != CUSTOMSCREEN)
    {
#warning TODO:
        screen = NULL;
    }
    
    if (screen)
        CopyMem (screen, buffer, size);

    return (screen != NULL);
    
    AROS_LIBFUNC_EXIT
} /* GetScreenData */
