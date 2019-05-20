/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <stdio.h>

#include "security_intern.h"
#include "security_packetio.h"

#include <libraries/mufs.h>

/*****************************************************************************

    NAME */
	AROS_LH1(LONG, secGetPktDefProtection,

/*  SYNOPSIS */
	/* (pkt) */
	AROS_LHA(struct DosPacket *, pkt, A1),

/*  LOCATION */
	struct SecurityBase *, secBase, 35, Security)

/*  FUNCTION

    INPUTS


    RESULT


    NOTES


    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    return pkt == NULL ? DEFPROTECTION : GetPktDefProtection(secBase, pkt);

    AROS_LIBFUNC_EXIT

} /* secGetPktDefProtection */

