/*
    Copyright (C) 2002-2026, The AROS Development Team.
    All rights reserved.
    
*/

#define AROS_TAGRETURNTYPE APTR
#include <utility/tagitem.h>
#include <proto/alib.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/muimaster.h>
extern struct Library * MUIMasterBase;

/*****************************************************************************

    NAME */
        APTR MUI_AllocAslRequestTags (

/*  SYNOPSIS */
        ULONG reqType,
        Tag tag1,
        ...)

/*  FUNCTION
        Allocate an ASL requester using a variable-length tag list.
        This is the varargs form of MUI_AllocAslRequest().

    INPUTS
        reqType - Requester type: ASL_FileRequest, ASL_FontRequest or
                  ASL_ScreenModeRequest.
        tag1    - First tag in a sequence of tag/value pairs terminated
                  by TAG_DONE.

    RESULT
        Pointer to the allocated requester, or NULL if allocation fails.

    NOTES
        Release the requester with MUI_FreeAslRequest().

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = MUI_AllocAslRequest(reqType, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* MUI_AllocAslRequestTags */
