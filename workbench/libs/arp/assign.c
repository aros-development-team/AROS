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

      AROS_LH2(ULONG, Assign,

/*  SYNOPSIS */ 
      AROS_LHA(char *, name    , A0),
      AROS_LHA(char *, physical, A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 56, Arp)

/*  NAME
        Assign -- Assigns a logical device name

    SYNOPSIS
        Result = Assign("name:","physical")
          D0              ao        a1

    FUNCTION
        This function performs the guts of an AmigaDOS "assign"
        function.  The arguments are similar to the arguments of the
        ADOS program "Assign logicaldev: directory".


    INPUTS
        "name" -- Name to create a Devinfo assigned name for.

        "physical" -- Name of file or directory to get a Lock from.
 		  NOTE - if physical is NULL, remove existing name.

    RESULT
        A Devinfo entry is created for the requested name. Any prior
        assignment for that name is removed.

        Result -- an error code return which may be one of:

            ASSIGN_OK       everything worked this time.
            ASSIGN_NODEV    "physical" did not represent a valid directory.
            ASSIGN_FATAL    Something is really rotten somewhere.
            ASSIGN_CANCEL   Attempt to cancel something (like a
                            volume) that can't be canceled.

    BUGS
        None known.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  /* if physical == NULL cancel any utstanding locks for this name */
  if (NULL == physical)
  {
    if (DOSTRUE == AssignLock(name, (BPTR)NULL))
      return ASSIGN_OK;
    else
      return ASSIGN_CANCEL;
  }
  else
  {
    BPTR lock = Lock(physical, ACCESS_READ);
  
    /* did we get the lock? */
    if (NULL != lock)
    {
      if (DOSTRUE == AssignLock(name, lock))
        return ASSIGN_OK; /* everything worked this time */
      else
      {
        /* free the lock as we couldn't create the assign */
        UnLock(lock); 
        return ASSIGN_FATAL; /* something is really rotten somewhere */
      }
    }
    else
      return ASSIGN_NODEV; /* "physical" did not represent a valid dir. */
  }
  
  AROS_LIBFUNC_EXIT
} /* Assign */
