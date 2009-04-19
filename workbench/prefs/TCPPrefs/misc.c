/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id: misc.c 17839 2003-06-02 21:57:54Z chodorowski $
 */

#include <exec/types.h>
#include <proto/dos.h>

void ShowError(CONST_STRPTR message)
{
	Printf("ERROR: %s\n", message);
}
