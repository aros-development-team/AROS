/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/


/*****************************************************************************

    NAME */

      AROS_LH2(LONG, LMult,

/*  SYNOPSIS */ 
      AROS_LHA(LONG, num1, D0),
      AROS_LHA(LONG, num2, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 100, Arp)

/*  NAME
	  LMult	- Perform a long multiply.
 
    FUNCTION
	  The 68000 instruction	set does not implement a full 32bit
	  multiply.  This function remedies that defect.

    INPUTS
	  num1 - A 32 bit signed number.

	  num2 - A 32 bit signed number.

    RESULT
	  LONG - The 32	bit result of num1*num2.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    return num1 * num2;

    AROS_LIBFUNC_EXIT
} 
