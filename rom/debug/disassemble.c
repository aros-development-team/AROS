/*
    Copyright © 2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH3(APTR, InitDisassembleCtx,

/*  SYNOPSIS */
        AROS_LHA(APTR, start, A0),
        AROS_LHA(APTR, end, A1),
        AROS_LHA(APTR, pc, A2),

/*  LOCATION */
        struct Library *, DebugBase, 9, Debug)

/*  FUNCTION
        Initializes a disassembly context for the given inputs.

    INPUTS
        start	- Start address of disassembly
        end   	- End address of disassembly
        pc 		- Program Counter value.

    RESULT
        returns a disassembly context handle.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

	return NULL;

    AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH1(IPTR, DisassembleCtx,

/*  SYNOPSIS */
        AROS_LHA(APTR, ctx, A0),

/*  LOCATION */
        struct Library *, DebugBase, 10, Debug)

/*  FUNCTION
        Disassemble the next instruction for the given context handle.

    INPUTS
        ctx		- Disassembly context handle.

    RESULT
        returns the size of the disassembled instruction.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

	return 0;

    AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH2(APTR, GetCtxInstructionA,

/*  SYNOPSIS */
        AROS_LHA(APTR, ctx, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, DebugBase, 11, Debug)

/*  FUNCTION
        Get the requested attributes for the disasembled
		instruction, for the specified Disassembly context.

    INPUTS
        ctx		- Disassembly context handle.
        tags   	- Taglist of requested attributes.
				DCIT_Instruction_Offset - instructions offset relative to the PC.
				DCIT_Instruction_HexStr - instruction in hex format
				DCIT_Instruction_Asm    - disassembled instruction

    RESULT
        number of handled attributes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

	return 0;

    AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH1(void, FreeDisassembleCtx,

/*  SYNOPSIS */
        AROS_LHA(APTR, ctx, A0),

/*  LOCATION */
        struct Library *, DebugBase, 12, Debug)

/*  FUNCTION
        Free the disassemble context

    INPUTS
        ctx		- Disassembly context handle.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

	return;

    AROS_LIBFUNC_EXIT
}

