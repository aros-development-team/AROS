/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH1(void, Puts,

/*  SYNOPSIS */ 
      AROS_LHA(char *, string  , A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 40, Arp)

/*  NAME
        Puts -- Print string with newline on stdout.

    SYNOPSIS
        BOOL = Puts("string")
         d0             a1

    FUNCTION
        Writes a string to stdout, and then writes a terminating newline.
        This is currently implemented as Printf("%s\n", string).

    INPUTS
        string - pointer to ascii string. (null terminated)
                 String may be null, in which case a newline only will
                 be displayed.

    RESULT
        See FPrintf page for more information - note that this
        function always trys to write a newline.

    BUGS
        None known.

    SEE ALSO
        Printf, FPrintf.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  PutStr(string);
  PutStr("\n");

  AROS_LIBFUNC_EXIT
} /* Puts */
