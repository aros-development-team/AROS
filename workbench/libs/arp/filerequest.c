/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/asl.h>
#include <libraries/asl.h>
#include "arp_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1(char *, FileRequest,

/*  SYNOPSIS */ 
      AROS_LHA(struct FileRequester *, filereqstruct, A0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 49, Arp)

/*    NAME
        FileRequest -- Get filename from user

    SYNOPSIS
	      retstr = FileRequest( filereqstruct )
        D0			    A0

    FUNCTION
        Prompts the user for a filename.  See struct FR_struct in arpbase.i,

    NOTE
        V1.0 of Arp does not support the FR_Flags, FR_Wildfunc,
        or FR_MsgFunc entries in the struct, but you MUST allocate these
        and pass NULL values if you want to work with V1.1 of arplib!

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)
 
  RequestFile(filereqstruct);
  return (char *)(filereqstruct -> fr_File); 
  
  AROS_LIBFUNC_EXIT
} /* FileRequest */
