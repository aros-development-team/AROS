/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

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
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    mode_t oumask = PosixCBase->umask;

    PosixCBase->umask = numask;

    return oumask;
}

static int __umask_init(struct PosixCIntBase *PosixCBase)
{
    struct PosixCIntBase *pPosixCBase;

    pPosixCBase = __GM_GetBaseParent(PosixCBase);

    /* TODO: Implement umask() properly
       Currently information is not used in any of the related functions
    */

     /* Child of exec*()/vfork() functions inherit umask of parent */
    if (pPosixCBase && (pPosixCBase->flags & (VFORK_PARENT | EXEC_PARENT)))
        PosixCBase->umask = pPosixCBase->umask;
    else
        PosixCBase->umask = S_IWGRP|S_IWOTH;

    return 1;
}

ADD2OPENLIB(__umask_init, 0);
