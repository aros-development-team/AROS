
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel.h"

/*****************************************************************************

    NAME */

	AROS_LH1(APTR, BusToPhys,

/*  SYNOPSIS */
	AROS_LHA(APTR, busAddress, A0),

/*  LOCATION */
	APTR, KernelBase, 1, Kernel)

/*  FUNCTION
	This function translates Bus address (address that device sees) to
	Physical address (address seen by CPU).

    INPUTS
	busAddress - Bus address as seen by device

    RESULT
	Physical address seen by CPU

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return((APTR)((ULONG)busAddress+_BUS_BASE));

    AROS_LIBFUNC_EXIT
}




/*****************************************************************************

    NAME */

	AROS_LH1(APTR, PhysToBus,

/*  SYNOPSIS */
	AROS_LHA(APTR, physAddress, A0),

/*  LOCATION */
	APTR, KernelBase, 2, Kernel)

/*  FUNCTION
	This function translates Physical address address (address seen by CPU)
	to Bus address (address seen by device).

    INPUTS
	physAddress - Physical address as seen by CPU

    RESULT
	Bus address seen by device

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return((APTR)((ULONG)physAddress+_BUS_BASE));

    AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    IO access functions

    These functions are the only allowable things used to communicate with IO
    address space. Please note that theirs implementation is platform specific.

    Also note, that allthough it might be possible to talk to hardware without
    them, it doesn't have to be possible any time. It may happen, that 
    self-made IO access will end with program killed.

    There is no description available for this functions, but there is no need
    for any. Function names and parameters are self-explaining.

*****************************************************************************/

AROS_LH2(void, OutB,
    AROS_LHA(UWORD, port, D0),
    AROS_LHA(UBYTE, val, D1),
    APTR, KernelBase, 3, Kernel)
{
    AROS_LIBFUNC_INIT

    asm volatile (
	"stbx %0,0,%1	\n\t"
	"sync		\n\t"
	:
	: "r"(val), "r"(port + _IO_BASE));

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, OutW,
    AROS_LHA(UWORD, port, D0),
    AROS_LHA(UWORD, val, D1),
    APTR, KernelBase, 4, Kernel)
{
    AROS_LIBFUNC_INIT

    asm volatile (
	"sthbrx %0,0,%1	\n\t"
	"sync		\n\t"
	:
	: "r"(val), "r"(port + _IO_BASE));

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, OutL,
    AROS_LHA(UWORD, port, D0),
    AROS_LHA(ULONG, val, D1),
    APTR, KernelBase, 5, Kernel)
{
    AROS_LIBFUNC_INIT

    asm volatile (
	"stwbrx %0,0,%1	\n\t"
	"sync		\n\t"
	:
	: "r"(val), "r"(port + _IO_BASE));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(UBYTE, InB,
    AROS_LHA(UWORD, port, D0),
    APTR, KernelBase, 6, Kernel)
{
    AROS_LIBFUNC_INIT

    UBYTE ret;

    asm volatile ("lbzx %0,0,%1\n\tisync\n\tnop"
	:"=r"(ret)
	:"r"(port + _IO_BASE));

    return(ret);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(UWORD, InW,
    AROS_LHA(UWORD, port, D0),
    APTR, KernelBase, 7, Kernel)
{
    AROS_LIBFUNC_INIT

    UWORD ret;

    asm volatile ("lhbrx %0,0,%1\n\tisync\n\tnop"
	:"=r"(ret)
	:"r"(port + _IO_BASE));

    return(ret);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, InL,
    AROS_LHA(UWORD, port, D0),
    APTR, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT

    ULONG ret;

    asm volatile ("lwbrx %0,0,%1\n\tisync\n\tnop"
	:"=r"(ret)
	:"r"(port + _IO_BASE));

    return(ret);

    AROS_LIBFUNC_EXIT
}

