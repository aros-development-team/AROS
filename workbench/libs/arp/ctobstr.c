/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <dos/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH3(ULONG, CtoBStr,

/*  SYNOPSIS */ 
      AROS_LHA(char *, CString  , A0),
      AROS_LHA(BPTR  , BSTR     , D0),
      AROS_LHA(ULONG , MaxLength, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 60, Arp)

/*  NAME
        CtoBStr -- Copy a C null terminated string to a BSTR
 
 
    FUNCTION
        This function copies a null terminated string into a BCPL
        string.  The BCPL string is passed as an unconverted icky
        pointer.
 
    INPUTS
        CString - pointer to the string to convert.
 
        BSTR - BPTR to the destination string.
 
        MaxLength -- Number of characters available in BSTR (not
                     including initial count byte.)
 
    RESULT
        Count -- actual number of characters transferred.
        A0 and A1 are zapped.
 
    ADDITIONAL NOTES:
        This function does NOT require A6 to be ArpBase.  This info
        is for internal (library) use only!  Within the Arp library
        you can bsr CtoBStr without setting A6 = ArpBase.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  char * Dest = BADDR(BSTR);
  ULONG Count = 0;
  
  MaxLength &= 0xff;
  
  while (0 != CString[Count] && Count < MaxLength )
  {
    Dest[Count+1] = CString[Count];
    Count++;
  }
  Dest[0]=Count;

  return Count;

  AROS_LIBFUNC_EXIT
} /* CtoBStr */
