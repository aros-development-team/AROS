/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH1(STRPTR, BaseName,

/*  SYNOPSIS */ 
      AROS_LHA(STRPTR, pathname, A0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 105, Arp)

/*  NAME
	  BaseName - return basename of	pathname

    FUNCTION
	  The basename of a pathname is	its last component. For
	  example, the basename	of DF1:FOOBAR is FOOBAR, but the
	  basename of DF1:Foobar/Doobar/Shobar is Shobar.  This
	  function returns a pointer to	the start of the basename
	  given	a valid	AmigaDOS path.

    INPUTS
	  PathName - Pointer to	a valid	pathname.

    RESULT
	  basename - Pointer to	start of basename.  Note that the
		  basename ptr may point to the	start of PathName.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    return FilePart(pathname);

    AROS_LIBFUNC_EXIT
} 
