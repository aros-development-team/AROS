/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <exec/memory.h>

/*****************************************************************************

    NAME */

      AROS_LH2(BPTR, ArpOpen,

/*  SYNOPSIS */ 
      AROS_LHA(char *, name      , D1),
      AROS_LHA(ULONG , accessmode, D2),

/*  LOCATION */
      struct ArpBase *, ArpBase, 66, Arp)

/*  NAME
        ArpOpen -- Open a file and track it.
 
    SYNOPSIS
        FileHandle = ArpOpen("name", accessmode)
            d0                 d1        d2
 
    FUNCTION
        This function is equivalent to the AmigaDOS Open(), except
        that this function will remember that the file is open when
        you do a CloseLibrary(ArpBase) and close it for you.
 
        This is part of the Resource tracking of ArpLib, which also
        tracks memory allocations and Locks.
 
    INPUTS
        name - pointer to a null terminated string.
 
        accessmode -- pointer to a valid AmigaDOS access mode.
 
    RESULT
        A BPTR to a filehandle, or NULL, if an error occurred.
 
    BUGS
        None known.
 
    SEE ALSO
        ArpAllocEntry(), ArpAlloc(), ArpLock(), FreeTaskResList().
        dos.doc/Open

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  BPTR FH = Open(name,accessmode);
  
  /* if we received a lock then track it! */
  if (NULL != FH)
    intern_AddTrackedResource(ArpBase, TRAK_FILE, (APTR)FH);

  return FH;
  
  AROS_LIBFUNC_EXIT
} /* ArpLock */
