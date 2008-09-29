/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__arosc_privdata.h"

#include <aros/symbolsets.h>
#include <sys/types.h>
#include <sys/stat.h>

#warning FIXME: Implement umask() properly

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
    struct arosc_privdata *privdata = __get_arosc_privdata();

    if (privdata->acpd_oldprivdata)
        privdata->acpd_umask = privdata->acpd_oldprivdata->acpd_umask;
    else
        privdata->acpd_umask = S_IWGRP|S_IWOTH;

    return 1;
}

ADD2INIT(__umask_init, 0);
