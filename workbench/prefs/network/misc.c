/*
    Copyright (C) 2009, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/dos.h>

void ShowError(CONST_STRPTR message)
{
    Printf("ERROR: %s\n", message);
}
