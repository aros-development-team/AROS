/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>
#include <proto/arp.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH2(ULONG, Printf,

/*  SYNOPSIS */ 
      AROS_LHA(char *, String, A0),
      AROS_LHA(LONG *, args  , A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 38, Arp)

/*  NAME
        Printf -- print formatted data on current output.

    SYNOPSIS
        count = Printf("String", *args)
          d0              A0      A1

    FUNCTION
        Print formatted data on current output stream.  This function
        is implemented as FPrintf( Output(), "String", *args), please
        see that page for more information.

    INPUTS
        See FPrintf entry for "String" *args.

    RESULT
        See FPrintf entry.

    BUGS
        None known.

    SEE ALSO
        FPrintf, Puts.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)
  
  ULONG count = FPrintf(Output(), String, args);
  Flush(Output());
  return count;
  AROS_LIBFUNC_EXIT
} /* Printf */
