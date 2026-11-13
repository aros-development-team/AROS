/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Debug
*/

/*****************************************************************************

    NAME
        Debug

    FORMAT
        Debug

    SYNOPSIS

    LOCATION
        C:

    FUNCTION
        Activates built-in AROS debugger (SAD)

    EXAMPLE

    SEE ALSO

******************************************************************************/

#include <proto/exec.h>
#include <aros/shcommands.h>

AROS_SH0(Debug, 1.0)
{
    AROS_SHCOMMAND_INIT

    Debug(0);
    return 0;
    
    AROS_SHCOMMAND_EXIT
}
