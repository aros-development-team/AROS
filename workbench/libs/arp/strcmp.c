/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/utility.h>

/*****************************************************************************

    NAME */

      AROS_LH2(ULONG, strcmp,

/*  SYNOPSIS */ 
      AROS_LHA(char *, s1, A0),
      AROS_LHA(char *, s2, A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 87, Arp)

/*  NAME
        Strcmp -- Compare two strings, ignoring case.
  
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

  return Stricmp(s1,s2);

  AROS_LIBFUNC_EXIT
} /* Strcmp */
