/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <dos/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH3(ULONG, BtoCStr,

/*  SYNOPSIS */ 
      AROS_LHA(char *, CString  , A0),
      AROS_LHA(BPTR  , BSTR     , D0),
      AROS_LHA(ULONG , MaxLength, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 59, Arp)

/*  NAME
        BtoCStr -- Copy a BSTR to a C null terminated string
  
    FUNCTION
        This function copies a BSTR into a null terminated string.
        The BCPL string is passed as an unconverted icky BCPL ptr.
 
    INPUTS
        CString - Pointer to a buffer area for the resulting string.
 
        BSTR - BPTR to the BSTR to convert.
 
        MaxLength - Maximum number of characters in buffer (not
                    including NULL terminator).
 
    RESULT
        Count -- Actual number of characters copied.
        A0 has START of C string for assembly convenience.
 
    ADDITIONAL NOTES:
        This function does NOT require A6 to be ArpBase.  This info
        if for internal (library) use only!  Within the Arp library
        you can bsr CtoBStr without setting A6 = ArpBase.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  char * Source = BADDR(BSTR);
  ULONG Length = Source[0];
  ULONG Count = 0;
  
  if (Length > MaxLength)
    Length = MaxLength;
  
  while (Count < Length)
  {
    CString[Count] = Source[Count+1];
    Count++;
  }
  CString[Count] = 0x0;
   
  return Count;

  AROS_LIBFUNC_EXIT
} /* BtoCStr */
