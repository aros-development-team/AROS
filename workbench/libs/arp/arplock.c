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

      AROS_LH2(BPTR, ArpLock,

/*  SYNOPSIS */ 
      AROS_LHA(char *, name      , D1),
      AROS_LHA(ULONG , accessmode, D2),

/*  LOCATION */
      struct ArpBase *, ArpBase, 68, Arp)

/*  NAME
        ArpLock -- Get a lock and track it.
 
    SYNOPSIS
        Lock = ArpLock("name", accessmode)
         d0              d1       d2
 
    FUNCTION
        This function is completely equivalent to the AmigaDOS Lock(),
        except that ArpLib will remember that you have opened this lock,
        and will UnLock() for you when you CloseLibrary(ArpBase).
 
    INPUTS
        name -- pointer to a null terminated string
 
        accessmode -- a valid AmigaDOS access value.
 
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

  BPTR lock = Lock(name,accessmode);
  
  /* if we received a lock then track it! */
  if (NULL != lock)
    intern_AddTrackedResource(ArpBase, TRAK_LOCK, (APTR)lock);

  return lock;
  
  AROS_LIBFUNC_EXIT
} /* ArpLock */
