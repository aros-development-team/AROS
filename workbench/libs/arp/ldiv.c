/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/


/*****************************************************************************

    NAME */

      AROS_LH2(LONG, LDiv,

/*  SYNOPSIS */ 
      AROS_LHA(LONG, dividend, D0),
      AROS_LHA(LONG, divisor, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 101, Arp)

/*  NAME
 	  LDiv - Perform a long	divide.

    FUNCTION
	  The 68000 instruction	set does not implement a full 32bit
	  divide.  This	function corrects that defect.

    INPUTS
	  dividend - A 32 bit signed number.

	  divisor - A 32 bit signed number.

    RESULT
	  LONG - The 32	bit result of dividend/divisor.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    return dividend / divisor;

    AROS_LIBFUNC_EXIT
} 
