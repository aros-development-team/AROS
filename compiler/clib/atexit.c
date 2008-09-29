/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__exitfunc.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int atexit(
	
/*  SYNOPSIS */
	void (*func)(void))

/*  FUNCTION
	Registers the given function to be called at normal
	process termination.
	
    INPUTS
	func - function to be called.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	exit()

    INTERNALS

******************************************************************************/
{
    struct AtExitNode *aen = malloc(sizeof(*aen));

    if (!aen) return -1;

    aen->node.ln_Type = AEN_VOID;
    aen->func.fvoid = func;

    return __addexitfunc(aen);
}
