/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "datatypes_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

        AROS_LH1(ULONG, GetDTTriggerMethodDataFlags,

/*  SYNOPSIS */
	AROS_LHA(ULONG, method, A0),


/*  LOCATION */
	struct Library *, DataTypesBase, 48, DataTypes)

/*  FUNCTION

    Get the kind of data that may be attached to the stt_Data field in the
    dtTrigger method body. The data type can be specified by or:ing the
    method id (within the STMF_METHOD_MASK value) with one of the STMD_
    identifiers.

    STMD_VOID     --  stt_Data must be NULL
    STMD_ULONG    --  stt_Data contains an unsigned value
    STMD_STRPTR   --  stt_Data is a pointer to a string
    STMD_TAGLIST  --  stt_Data points to an array of struct TagItem terminated
                      with TAG_DONE

    The trigger methods below STM_USER are explicitly handled as described in
    <datatypes/datatypesclass.h>.

    INPUTS

    method  --  dtt_Method ID from struct DTMethod

    RESULT

    One of the STMD_ identifiers defined in <datatypes/datatypesclass.h>

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    CopyDTTriggerMethods(), FindTriggerMethod()

    INTERNALS

    HISTORY

    5.8.99   SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    switch(method)
    {
    case  STM_PAUSE:
    case  STM_PLAY:
    case  STM_CONTENTS:
    case  STM_INDEX:
    case  STM_RETRACE:
    case  STM_BROWSE_PREV:
    case  STM_BROWSE_NEXT:
    case  STM_NEXT_FIELD:
    case  STM_PREV_FIELD:
    case  STM_ACTIVATE_FIELD:
    case  STM_REWIND:
    case  STM_FASTFORWARD:
    case  STM_STOP:
    case  STM_RESUME:
	return STMD_VOID;
    
    case  STM_COMMAND:
	return STMD_STRPTR;
	    
    case  STM_LOCATE:
	return STMD_ULONG;

    default:
	break;
    }

    /* User defined */
    return (method & STMF_DATA_MASK) != 0 ? method & STMF_DATA_MASK : STMD_VOID;

    AROS_LIBFUNC_EXIT
} /* GetDTTriggerMethodDataFlags */
