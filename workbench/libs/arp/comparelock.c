/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH2(ULONG, CompareLock,

/*  SYNOPSIS */ 
      AROS_LHA(BPTR, Lock1, D0),
      AROS_LHA(BPTR, Lock2, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 76, Arp)

/*    NAME
        CompareLock -- Compares two filesystem Locks to see if they
   			belong to the same object.
 
 
    FUNCTION
      This function may be used to determine if two file Locks obtained
      with the DOS Lock() function belong to the same file/object.
      Using this library call allows an application to avoid using  
      private information; this call may be updated if/when a DOS
      Packet ACTION_COMPARE_LOCK is implemented.
 
    INPUTS
      D0 and D1 are Locks obtained with DOS Lock()
 
    RESULTS
      Return is in D0.  If D0 is:
 
      0 - Locks are identical
      1 - Locks are on same Volume
      2 - Locks are on different Volumes (dn_Task?)
      3 - Locks are on different Volumes
 
      Z-Flag reflects return status
 
    BUGS
      Caveat, this function now uses information that is considered
      "private" to each filehandler; thus, it is possible it will
      give an erroneous result if somebody implements a funny
      filehandler.
      This function was included primarily for the Rename program,
      to prevent Rename from creating directory loops that cause the
      directory to be lost in BCPL-space.
 
    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  LONG value = SameLock(Lock1, Lock2);
  switch (value) 
  {
    case LOCK_SAME       : return 0; break;
    case LOCK_SAME_VOLUME: return 1; break;
    case LOCK_DIFFERENT  : return 3; break;
  };

  return 0;  
  AROS_LIBFUNC_EXIT
} /* CompareLock */
