/*
    Copyright (C) 2002-2026, The AROS Development Team.
    All rights reserved.
    
*/

#define AROS_TAGRETURNTYPE BOOL
#include <utility/tagitem.h>
#include <proto/alib.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/muimaster.h>
extern struct Library * MUIMasterBase;

/*****************************************************************************

    NAME */
        BOOL MUI_AslRequestTags (

/*  SYNOPSIS */
        APTR requester,
        Tag tag1,
        ...)

/*  FUNCTION
        Run an ASL requester using a variable-length tag list.
        The call forwards its arguments to MUI_AslRequest().

    INPUTS
        requester - ASL requester allocated with MUI_AllocAslRequest()
                    or AllocAslRequest().
        tag1      - First tag in a sequence of tag/value pairs terminated
                    by TAG_DONE.

    RESULT
        Returns the BOOL result of MUI_AslRequest(): TRUE on success,
        FALSE otherwise.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    
    retval = MUI_AslRequest(requester, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* MUI_AslRequestTags */
