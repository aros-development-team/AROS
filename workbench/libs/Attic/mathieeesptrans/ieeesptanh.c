/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/


#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/mathieeesingtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingtrans_intern.h"


/*****************************************************************************

    NAME */

      AROS_LH1(LONG, IEEESPTanh,

/*  SYNOPSIS */

      AROS_LHA(LONG, y, D0),

/*  LOCATION */

      struct Library *, MathIeeeSingTransBase, 12, Mathieeesingtrans)

/*  FUNCTION

      Calculate hyperbolic tangens of the IEEE single precision number

    INPUTS

      y - IEEE single precision floating point number

    RESULT

      IEEE single precision floating point number


      flags:
	zero	 : result is zero
	negative : result is negative
	overflow : (not possible)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

		( e^x - e^(-x) )
     tanh(x) =  ----------------
		( e^x + e^(-x) )

     tanh( |x| >= 9 ) = 1

    HISTORY

******************************************************************************/

{
LONG Res;
LONG y2 = y & (IEEESPMantisse_Mask + IEEESPExponent_Mask );
LONG tmp;

  if ( y2 >= 0x41100000 )
  /* tanh( x > 9 ) =  1
  ** tanh( x <-9 ) = -1
  */
    return (one | ( y & IEEESPSign_Mask ));

  /* tanh(-x) = -tanh(x) */
  Res = IEEESPExp(y2);
  tmp = IEEESPDiv(one, Res);
  Res = IEEESPDiv( IEEESPAdd(Res, (tmp | IEEESPSign_Mask) ),
		   IEEESPAdd(Res, tmp) );

  /* Result is zero */
  if (0 == Res )
  {
    if (y < 0)
      SetSR(Zero_Bit | Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    else
      SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return (y & IEEESPSign_Mask);
  }

  /* Argument is negative -> result is negative */
  if ( y < 0)
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return (Res | IEEESPSign_Mask );
  }

  return Res;
} /* SPTanh */
