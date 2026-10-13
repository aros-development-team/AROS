/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_server.h"
#include "security_segment.h"
#include "security_monitor.h"
#include "security_memory.h"
#include "security_plugins.h"
#include "security_crypto.h"
#include "security_enforce.h"
#include "security_packetio.h"
#include "security_userinfo.h"
#include "security_groupinfo.h"
#include "security_login.h"
#include "security_support.h"

/*****************************************************************************

    NAME */
        AROS_LH0(struct secUserInfo *, secAllocUserInfo,

/*  SYNOPSIS */

/*  LOCATION */
        struct SecurityBase *, secBase, 9, Security)

/*
    FUNCTION
        Allocate a User Information Structure. Free it with secFreeUserInfo().

    RESULT
        info - the structure, or NULL.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (struct secUserInfo *)MAlloc(sizeof(struct secPrivUserInfo));

    AROS_LIBFUNC_EXIT
} /* secAllocUserInfo */
