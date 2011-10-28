/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__arosc_privdata.h"

#include <aros/symbolsets.h>
#include <sys/types.h>
#include <sys/stat.h>

#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */

	mode_t umask(

/*  SYNOPSIS */
	mode_t numask)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    mode_t oumask = __umask;

    __umask = numask;

    return oumask;
}

static int __umask_init(void)
{
    struct aroscbase *aroscbase = __get_aroscbase(),
                     *paroscbase;

    paroscbase = __GM_GetBaseParent(aroscbase);

    /* FIXME: Implement umask() properly */

    if (paroscbase && (paroscbase->acb_flags & (VFORK_PARENT | EXEC_PARENT)))
        __umask = paroscbase->acb_umask;
    else
        __umask = S_IWGRP|S_IWOTH;

    return 1;
}

ADD2OPENLIB(__umask_init, 0);
