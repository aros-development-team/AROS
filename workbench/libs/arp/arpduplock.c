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

      AROS_LH1(BPTR, ArpDupLock,

/*  SYNOPSIS */ 
      AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 67, Arp)

/*  NAME
        ArpDupLock -- Duplicate a lock and track it.
 
    SYNOPSIS
        Lock = ArpDupLock( lock )
         d0                 d1
 
    FUNCTION
        This function is completely equivalent to the AmigaDOS DupLock(),
        except that ArpLib will remember that you have duped this lock,
        and will UnLock() for you when you CloseLibrary(ArpBase).
 
    INPUTS
        lock -- pointer to a lock
 
    RESULTS
        A lock or NULL, if no lock.
 
    BUGS
        None known.
 
    SEE ALSO
        DOS DOCUMENTATION, FreeTaskResList, ArpAlloc(), ArpAllocEntry(),
        ArpClose().

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  BPTR duplock = DupLock(lock);
  
  /* if we received a lock then track it! */
  if (NULL != duplock)
    intern_AddTrackedResource(ArpBase, TRAK_LOCK, (APTR)duplock);

  return duplock;
  
  AROS_LIBFUNC_EXIT
} /* ArpDupLock */
