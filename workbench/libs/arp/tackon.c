/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH2(VOID, TackOn,

/*  SYNOPSIS */ 
      AROS_LHA(STRPTR, pathname, A0),
      AROS_LHA(CONST_STRPTR, filename, A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 104, Arp)

/*  NAME
	  TackOn -- Correctly add a filename to	an existing pathname.

    FUNCTION
	  This function	correctly Tacks	a filename onto	an existing
	  PathName, inserting the appropriate separator	character (':'
	  or '/') as appropriate. The user is responsible for
	  allocating a large enough buffer.

    INPUTS
	  PathName - Pointer to	the pathname to	be augmented.

	  FileName - Pointer to	the filename to	augment	pathname with.

    RESULT
	  None.


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    AddPart(pathname, filename, 32000);

    AROS_LIBFUNC_EXIT
} 
