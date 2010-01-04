/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/dos.h>

VOID ShowError(CONST_STRPTR message)
{
    Printf("ERROR: %s\n", message);
}
