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
        AROS_LH4(APTR, secContextLocate,

/*  SYNOPSIS */
        AROS_LHA(secPluginModule *, module, A0),
        AROS_LHA(ULONG, id, D0),
        AROS_LHA(struct Task *, caller, A1),
        AROS_LHA(ULONG, size, D1),

/*  LOCATION */
        struct SecurityBase *, secBase, 52, Security)

/*
    FUNCTION
        Plugin API: locate (or allocate) the context memory of size 'size'
        that a module keeps for a caller under the given id. The memory is
        freed automatically when the caller closes the library or exits.

    RESULT
        The context memory, or NULL.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    APTR res = NULL;
    struct secTaskNode *node;

    if (!caller)
        caller = FindTask(NULL);

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindContextOwner(secBase, caller)))
    {
        if (!(res = FindContext(node, module, id)))
            res = AllocateContext(node, module, id, size);
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return res;

    AROS_LIBFUNC_EXIT
} /* secContextLocate */
