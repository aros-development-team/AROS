/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX function vfork()
    Lang: english
*/

/******************************************************************************

    NAME
#include <unistd.h>

	pid_t vfork ();

    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

    #include "aros/ppc/asm.h"

    .text
    _ALIGNMENT
    .globl AROS_CDEFNAME(vfork)
    _FUNCTION(AROS_CDEFNAME(vfork))
    .set    bufsize, 59*4
    .set    retaddr, 2*4
    .set    stack,   0*4

AROS_CDEFNAME(vfork):
    #warning implement vfork
    blr    
