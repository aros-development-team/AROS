/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/


/*****************************************************************************

    NAME */

      AROS_LH2(LONG, LMod,

/*  SYNOPSIS */ 
      AROS_LHA(LONG, dividend, D0),
      AROS_LHA(LONG, divisor, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 102, Arp)

/*  NAME
	  LMod - Perform a long	modulus, doing canonical thing with
	  sign.

    FUNCTION
	  This returns the remainder of	dividend/divisor. The result
	  will have the	sign of	the dividend.

     INPUTS
	  dividend - A 32 bit signed number.

	  divisor - A 32 bit signed number.

     RESULT
	  LONG - The 32	bit result of dividend%divisor,	with sign of
	  the dividend.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    return dividend % divisor;

    AROS_LIBFUNC_EXIT
} 
