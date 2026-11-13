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
        AROS_LH1(ULONG, secRegisterHandler,

/*  SYNOPSIS */
        AROS_LHA(struct plugin_ops *, ops, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 47, Security)

/*
    FUNCTION
        Plugin API: register an operation table with the subsystem named
        by its HandlerType.

    RESULT
        secpiTRUE on success.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return regHandler(secBase, TRUE, ops);

    AROS_LIBFUNC_EXIT
} /* secRegisterHandler */
