/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/utility.h>

/*****************************************************************************

    NAME */

      AROS_LH3(ULONG, strncmp,

/*  SYNOPSIS */ 
      AROS_LHA(char *, s1, A0),
      AROS_LHA(char *, s2, A1),
      AROS_LHA(ULONG , n , D0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 88, Arp)

/*  NAME
        Strncmp -- Compare two strings for n bytes, ignoring case.
  
    FUNCTION
        returns <0 =0 or >0
 
    INPUTS
 
    RESULT

    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  return Strnicmp(s1,s2,n);

  AROS_LIBFUNC_EXIT
} /* Strncmp */
