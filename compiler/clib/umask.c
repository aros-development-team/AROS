/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
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
        umask is currently remembered but not used in any function

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __GM_GetBase();
    mode_t oumask = aroscbase->acb_umask;

    aroscbase->acb_umask = numask;

    return oumask;
}

static int __umask_init(struct aroscbase *aroscbase)
{
    struct aroscbase *paroscbase;

    paroscbase = __GM_GetBaseParent(aroscbase);

    /* TODO: Implement umask() properly
       Currently information is not used in any of the related functions
    */

     /* Child of exec*()/vfork() functions inherit umask of parent */
    if (paroscbase && (paroscbase->acb_flags & (VFORK_PARENT | EXEC_PARENT)))
        aroscbase->acb_umask = paroscbase->acb_umask;
    else
        aroscbase->acb_umask = S_IWGRP|S_IWOTH;

    return 1;
}

ADD2OPENLIB(__umask_init, 0);
