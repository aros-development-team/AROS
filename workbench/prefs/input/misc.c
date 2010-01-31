/*
    Copyright © 2003-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/dos.h>

void ShowError(CONST_STRPTR message)
{
    Printf("ERROR: %s\n", message);
}
