/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate a trap
    Lang: english
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, AllocTrap,

/*  SYNOPSIS */
	AROS_LHA(long, trapNum, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 57, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Very similar to AllocSignal()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *ThisTask;
    ULONG mask;
    ULONG mask1;
    
    ThisTask = FindTask(NULL);
    mask = GetTrapAlloc(ThisTask);

    /* Will any trap do? */
    if(trapNum < 0)
    {
	/*
	 * To get the last nonzero bit in a number I use a&~a+1:
	 * Given a number that ends with a row of zeros  xxxx1000
	 * I first toggle all bits in that number	 XXXX0111
	 * then add 1 to toggle all but the last 0 again XXXX1000
	 * and AND this with the original number	 00001000
	 *
	 * And since ~a+1=-a I can use a&-a instead.
	 *
	 * And to get the last zero bit I finally use ~a&-~a.
	 */
	mask1 = ~mask & - ~mask;

	/* Is the bit already allocated? */
	if(mask1 == 0)
	    return -1;

	/* And get the bit number */
	trapNum=(mask1&0xffff0000?16:0)+(mask1&0xff00ff00?8:0)+
		  (mask1&0xf0f0f0f0? 4:0)+(mask1&0xcccccccc?2:0)+
		  (mask1&0xaaaaaaaa? 1:0);
    }
    else
    {
	mask1 = 1L << trapNum;

	/* If trap bit is already allocated, return. */
	if(mask & mask1)
	    return -1;
    }

    if (ThisTask->tc_Flags & TF_ETASK) {
        struct ETask *et = ThisTask->tc_UnionETask.tc_ETask;
	
	et->et_TrapAlloc |= mask1;
    } else
        ThisTask->tc_TrapAlloc |= mask1;

    return trapNum;
    AROS_LIBFUNC_EXIT
} /* AllocTrap() */
