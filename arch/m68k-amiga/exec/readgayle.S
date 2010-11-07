/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadGayle() - get the Gayle ID
    Lang: english
*/
/*****************************************************************************

    NAME
#include <proto/exec.h>

	AROS_LH0(ULONG, ReadGayle,

    LOCATION
	struct ExecBase *, SysBase, 136, Exec)

    FUNCTION
    Gets the Gayle ID

    INPUTS
	None.

    RESULT

    NOTES
	This function is special, because we need to call it
	from ROM code before the stack is set up.
	
	If %sp > 0xe000000, then we return via
	the value in %sp, not (%sp)+.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(ReadGayle,Exec)
AROS_SLIB_ENTRY(ReadGayle,Exec):
    /* NOTE: We don't need to check for custom chip mirroring,
     *       as BLTDDAT (at 0) is read only, and always returns
     *       either all 0 or all 1 as the ID via this algo.
     */
    move.l  #0xde1000, %a0      /* Gayle ID register */
    clr.l   %d0                 /* ID we will return */
    clr.l   %d1                 /* Gayle bit index */
    move.b  #0,%a0@             /* Reset ID register */
0:
    move.b  %a0@,%d0            /* Put reg into lower byte of %d0 */
    lsl.w   #1,%d0              /* Shift lower 16 bits of %d0 left by 1 */
    addq.l  #1,%d1              /* Increment bit index */
    cmp.b   #8,%d1              /* Last bit?*/
    ble.s   0b                  /* No - read more. */
    lsr.w   #8,%d0              /* Yes - move ID to lower 8 bits of %d0 */

    cmp.b   #0xff,%d0
    bne.s   1f
    clr.b   %d0
1:
    cmp.l   #_text_start,%sp
    bge.s   2f
    rts
2:
    jmp     (%sp)
