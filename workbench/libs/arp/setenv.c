/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>
#include <dos/var.h>

/*****************************************************************************

    NAME */

      AROS_LH2(BOOL, Setenv,

/*  SYNOPSIS */ 
      AROS_LHA(STRPTR, string , A0),
      AROS_LHA(STRPTR, buffer , A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 48, Arp)

/*  NAME
        Setenv -- Set the value of an environment variable
 
    FUNCTION
        This function provides an environment variable mechanism
        compatible with MANX.  Note that this call is more efficient
        than using the manx setenv() function call when arplibrary is
        installed.
 
    INPUTS
        string -- pointer to an environment variable name.
        buffer -- a user allocated area which will contains the values
                  to be associated with this environment variabnle.
 
    RESULT
        If the value was succesfully established, a TRUE result will
        be returned, otherwise, a FALSE value will be returned.
 
 
    ADDTIONAL CONSIDERATIONS
        MANX was the first to implement environment variables on the
        Amiga.  As a result, we are trying to be compatible with their
        implementation.
 
    BUGS
        None known.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  /* Oh, I hope the buffer contains a Null-terminated string! */
  return SetVar(string, buffer, -1, GVF_GLOBAL_ONLY);
    
  AROS_LIBFUNC_EXIT
} /* Setenv */
