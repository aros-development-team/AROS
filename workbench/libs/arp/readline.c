/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, ReadLine,

/*  SYNOPSIS */ 
      AROS_LHA(char *, address, A0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 41, Arp)

/*  NAME
        ReadLine -- Get a line from current input. (stdin)

    SYNOPSIS
        Count = ReadLine("Address")
          d0                A0

    FUNCTION
       This function reads a line of up to MaxInputBuf characters from
       stdin.  You must have a buffer available of this size, or you
       run the risk of overwriting innocent memory.  MaxInputBuf is
       defined in arpbase.[h|i].

       This function does no editing or echoing of the command line,
       although it does guarantee the returned string is null terminated.

    INPUTS
        Address - Pointer to a 256 byte buffer to store the input
                  string in.

    RESULT
        Count - The actual count of the characters returned.
        Address will contain a null terminated string of Count characters.

    BUGS
        None known - there may be problems using this function on other
                     than a CON: window (i.e., a file, or RAW:), but
                     nothing definite is known at this time.

    ADDITIONAL NOTES:
        For the convenience of assembly language programs, A0 is guaranteed
        to contain the same value on exit as it did on entry.

    SEE ALSO
        GADS, EscapeString.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  FGets(Input(), address, MaxInputBuf);
  return strlen(address);
  
  AROS_LIBFUNC_EXIT
} /* ReadLine */
