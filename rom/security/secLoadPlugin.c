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
        AROS_LH1(BOOL, secLoadPlugin,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 50, Security)

/*
    FUNCTION
        Load and initialise a plugin from the configuration directory.
        Root only.

    INPUTS
        name - the plugin name without the .secplugin suffix.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!name || !CallerIsRoot(secBase))
        return FALSE;
    if (FindTask(NULL) == (struct Task *)secBase->Server)
        return loadPlugin(secBase, name);
    return (BOOL)SendServerPacket(secBase, secSAction_LoadPlugin, (SIPTR)name, 0, 0, 0);

    AROS_LIBFUNC_EXIT
} /* secLoadPlugin */
