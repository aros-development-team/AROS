/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, Atol,

/*  SYNOPSIS */ 
      AROS_LHA(char *, string  , A0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 43, Arp)

/*  NAME
        Atol -- Ascii string to long int.
  
    FUNCTION
        Convert "string" to a long integer.  This function will skip
        leading whitespace.
        This function returns an error (=0) if a non-whitespace 
        non-digit is encountered during processing.
 
    INPUTS
        string - pointer ascii string, which may be null terminated
                 or whitespace terminated.  The digits may have
                 leading whitespace.
 
    RESULT
        intval -- a long integer value.
                  If an non-numeric is encountered during processing 
                  other than surrounding whitespace or null
                  terminators), this function will return 0.
 
    BUGS
        Values which cannot be represented as a 32bit integer will
        not cause an error.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  ULONG count = 0;
  ULONG res = 0;
  /* skip leading whitespaces */
  while (0x20 == string[count])
    count++;
    
  /* this has to be the Ascii string to convert now */
  while (string[count] >= 0x30 && string[count] <= 0x39)
  {
    res = res * 10 + (string[count] - 0x30);   
    count++;
  }
  return res;

  AROS_LIBFUNC_EXIT
} /* atol */
