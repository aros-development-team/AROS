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

      AROS_LH1(void, DeletePort,

/*  SYNOPSIS */ 
      AROS_LHA(struct MsgPort *, port , A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 52, Arp)

/*    NAME
        DeletePort -- Deletes a message port.
 
    SYNOPSIS
        DeletePort(port)
                    A1
 
    FUNCTION
        Removes a message port.
 
    INPUTS
        port - pointer to a message port - may be private or public.
               port may also be NULL, in which case this function does nothing.
 
    RESULT
        None.
 
    ADDITIONAL CONSIDERATIONS
        Functionally equivalent to amiga.libs DeletePort, although the Amiga
        version does some extra stuff which is not done here, largely
        because it does not appear needed.
 
    BUGS
        None known.
 
    SEE ALSO
        CreatePort

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  if (NULL != port)
  {
    if (NULL != port->mp_Node.ln_Name)
	    RemPort (port);

    DeleteMsgPort (port);
  }

  AROS_LIBFUNC_EXIT
} /* DeletePort */
