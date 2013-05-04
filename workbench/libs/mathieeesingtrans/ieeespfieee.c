/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, IEEESPFieee,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct Library *, MathIeeeSingTransBase, 18, MathIeeeSingTrans)

/*  FUNCTION
        Convert IEEE single to IEEE single
        It just returns the input parameter.

    INPUTS

    RESULT
        IEEE single precision floting point number

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return y;
    
    AROS_LIBFUNC_EXIT
}
