/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH3(ULONG, FPrintf,

/*  SYNOPSIS */ 
      AROS_LHA(BPTR  , File  , D0),
      AROS_LHA(char *, String, A0),
      AROS_LHA(LONG *, args  , A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 39, Arp)

/*  NAME
        FPrintf -- print formatted data on file.

    SYNOPSIS
        count = FPrintf(File, "String", *args)
 			 D0      A0       A1
    FUNCTION
        This function performs 'standard' C-style formatting of data on
        the specified output file. It uses the exec function
        RawDoFmt() to do the actual formatting of the data.  The %
        types supported by this function are quite standard, see any C
        reference for details.

    INPUTS
        File - A valid AmigaDOS output file handle, such as returned
               by Open() or Output().

        "String" - pointer to a C-style format string.

        *args - Pointer to start of argument stream.

    RESULT
        count - if all goes well, the total count of characters
                actually written will be returned.  If an error occured,
                this function will return -1.  If this function is
                passed a NULL output filehandle, this function will
                return zero.

    ADDITIONAL CONSIDERATIONS
        If your compiler uses a default int size of 32 bits (i.e., Lattice),
        you *must* specify %lx or %ld, for example, instead of %d
        or %x, since the compilers will promote all ints to longs
        before passing them to this function.

        Note also that this function very likely has a different idea
        of stdout then the support libraries for your compiler.

    BUGS
        None known.

    SEE ALSO
        Printf, Puts, exec.library/RawDoFmt, C language reference on printf.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)
  
  if (NULL == File)
    return 0;
  else
  {
    /* if this file can be locked then look for the startingposition in
       the file via Seek() */
    BPTR lock = DupLockFromFH(File);
    if (NULL != lock)
    { 
      ULONG Start;
      UnLock(lock);
      Start = Seek(File, 0, OFFSET_CURRENT);  
      VFWritef(File, String, (LONG *)args);
      return Seek(File, 0, OFFSET_CURRENT) - Start;
    }
    else /* no lock was possible, return the length of the formattingstring */
    {
      VFWritef(File, String, (LONG *)args);
      return strlen(String);
    }
  }

  AROS_LIBFUNC_EXIT
} /* FPrintf */
