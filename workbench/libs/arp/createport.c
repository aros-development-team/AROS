/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/exec.h>
#include <exec/ports.h>

/*****************************************************************************

    NAME */

      AROS_LH2(struct MsgPort *, CreatePort,

/*  SYNOPSIS */ 
      AROS_LHA(char *, name    , A0),
      AROS_LHA(LONG  , priority, D0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 51, Arp)

/*    NAME
        CreatePort -- Create a message port.
  
    FUNCTION
        Creates a message port with the given name and priority.  This
        function is equivalent to that in amiga.lib, but is reentrant, and
        may be called by assembly code.
 
     INPUTS
        Name - pointer to a null terminated character string, or NULL.  If NULL,
               this function will not attach the port to the system msgport
               list (it will be private to the caller).
 
        priority -- the priority of this msgport (-128..127).
 
    RESULT
        Pointer to an initialized message port, or NULL, if port could not
        be allocated.  If Name was non-null, then the port will have been
        added to the system's msgport list.
 
    BUGS
        None known.
 
    SEE ALSO
        DeletePort

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  struct MsgPort * mp = CreateMsgPort ();

  if (mp)
  {
	  mp->mp_Node.ln_Name = name;
	  mp->mp_Node.ln_Pri  = priority;
 
	  if (name)
	    AddPort (mp);
  }

  return mp;
 
  AROS_LIBFUNC_EXIT
} /* CreatePort */
