/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
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
        AROS_LH1(BOOL, secCheckPasswdA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 17, Security)

/*
    FUNCTION
        Check the password of the owner of the calling task; asks for it
        unless secT_Password is given.

    TAGS
        secT_Input, secT_Output, secT_Graphical, secT_PubScrName - as for
        secLoginA(). secT_Password - (STRPTR) the password to check.

    RESULT
        valid - TRUE if the password is correct.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTags tags;
    struct LocaleInfo li;
    char pwdbuf[secPASSWORDSIZE];
    STRPTR password;
    BOOL res = FALSE;

    if (!InterpretTagList(secBase, taglist, &tags))
        return FALSE;

    if (tags.Password)
        password = tags.Password;
    else
    {
        memset(pwdbuf, 0, sizeof(pwdbuf));
        OpenLoc(secBase, &li);
        if (!ReadPasswordCon(secBase, tags.Input, tags.Output, pwdbuf, sizeof(pwdbuf), &li))
        {
            CloseLoc(secBase, &li);
            return FALSE;
        }
        CloseLoc(secBase, &li);
        password = pwdbuf;
    }
    res = (BOOL)SendServerPacket(secBase, secSAction_CheckPasswd, (SIPTR)password, 0, 0, 0);
    memset(pwdbuf, 0, sizeof(pwdbuf));
    return res;

    AROS_LIBFUNC_EXIT
} /* secCheckPasswdA */
