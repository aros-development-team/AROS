/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
        Convert IEEE single to IEEE single
        It just returns the input parameter.

    RESULT
        IEEE single precision floting point number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH1(float, IEEESPTieee,
    AROS_LHA(float, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 17, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    return y;
    
    AROS_LIBFUNC_EXIT
}
