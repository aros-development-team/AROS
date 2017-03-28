/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/* The main purpose is to test alert address detection and stack trace */

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>

/*
 * SoftInt is the only way to simulate supervisor mode on hosted AROS.
 * Of course there's no real privilege level difference, however interrupts
 * in hosted AROS are running on a simulated supervisor level for internal
 * purposes (in order to avoid nesting interrupts).
 */
static AROS_INTH1(superAlert, APTR, interruptData)
{
    AROS_INTFUNC_INIT

    D(bug("Supervisor code called\n"));

    Alert((IPTR)interruptData);

    return 0;

    AROS_INTFUNC_EXIT
}

struct Interrupt MyInt;

int main(int argc, char **argv)
{
    IPTR n = AN_InitAPtr;
    BOOL super = FALSE;
    int i;

    for (i = 1; i < argc; i++)
    {
    	if (!stricmp(argv[i], "deadend"))
	    n = AN_BogusExcpt;
	else if (!stricmp(argv[i], "supervisor"))
	    super = TRUE;
    }

    if (super)
    {

	D(bug("Calling supervisor alert...\n"));

	MyInt.is_Data = (APTR)n;
	MyInt.is_Code = (APTR)superAlert;

	Cause(&MyInt);
    }
    else
    {
    	Alert(n);
    }

    return 0;
}
