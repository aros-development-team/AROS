/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH3(ULONG, PathName,

/*  SYNOPSIS */ 
      AROS_LHA(BPTR,   lock       , D0),
      AROS_LHA(STRPTR, Destination, A0),
      AROS_LHA(ULONG,  NumberNames, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 55, Arp)

/*    NAME
        PathName - Find complete pathname of file/directory.
 
    SYNOPSIS
        Length = PathName(Lock, Destination, NumberNames)
          D0               D0       A0          D1
 
    FUNCTION
        This function builds a path name which completely describes the
        path from the root of the filing system to the file on which the
        Lock has been obtained.
 
        The name returned is of the form:
 
            Volume:Dir1/Dir2/Dir3/.../Name\x00
 
    INPUTS
        Lock -- Thisis a lock on the file or directory obtained from Lock()
                or DupLock(), or some such function.
 
        Destination -- This is the area of memory to place the filename in.
 
        NumberNames -- This is the number of names that can be placed in
                       the destination area.  You should reserve 31 bytes
                       for each pathname component.  The minimum buffer
                       size would be 32 bytes, which would allow room for
                       one name.
 
    RESULT
        The resulting pathname will be placed in Destination.
 
        If everything goes well, then you get the total length of the
        pathname, in characters accumulated.  If there is a problem (either
        the buffer is too small, or there was a disk error), you will get a
        zero.
 
    BUGS
        None known.
 
    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  ULONG len, success;
  
  if (1 == NumberNames)
    len = 32;
  else
    len = NumberNames * 31;
    
  success = NameFromLock(lock, Destination, len);
  
  /* if there was a problem: return 0*/
  if (DOSFALSE == success)
    return 0;

  /* there was no problem acquiring the pathname */
  return strlen(Destination);
    
  AROS_LIBFUNC_EXIT
} /* PathName */
