/*
 * loginname.c - Login name handling
 *
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <sys/time.h>
#include "base.h"
#include "credential.h"
#include <proto/usergroup.h>
#include <string.h>
#include <exec/memory.h>

/****** usergroup.library/getlogin *******************************************

    NAME
        getlogin - get login name

    SYNOPSIS
        name = getlogin()
        D0

        char *getlogin(void)

    FUNCTION
        The getlogin() routine returns the login name of the user associated
        with the current session, as previously set by setlogin().  The name
        is normally associated with a console at the time a session is
        created, and is inherited by all processes descended from the login
        process.  (This is true even if some of those processes assume
        another user ID, for example when su is used.)

    INPUTS

    RESULT
        name   - pointer to login name

    SEE ALSO
        setlogin()

****************************************************************************
*/

AROS_LH0(char *, getlogin,
         struct UserGroupBase *, UserGroupBase, 36, Usergroup)
{
    AROS_LIBFUNC_INIT

    static char buffer[MAXLOGNAME];
    struct proc *p;

    D(bug("[UserGroup] %s()\n", __func__));

    lock(proc_list);
    p = procfind((pid_t)NULL);
    memcpy(buffer, p->p_session->s_login, MAXLOGNAME);
    unlock(proc_list);

    return buffer;

    AROS_LIBFUNC_EXIT
}

/****** usergroup.library/setlogin *******************************************

    NAME
        setlogin - set login name

    SYNOPSIS
        success = setlogin(name)
        D0                 A1

        int setlogin(const char *);

    FUNCTION
        The function setlogin() sets the login name of the user associated
        with the current session to name. This call is restricted to the
        super-user, and is normally used only when a new session is being
        created on behalf of the named user (for example, at login time, or
        when a remote shell is invoked).

    INPUTS
        name     - Buffer to hold login name

    RESULT
        If a call to setlogin() succeeds, a value of 0 is returned.  If
        setlogin() fails, a value of -1 is returned and an error code is
        placed into global errno location.

    ERRORS
        [EPERM]  - The caller has got no necessary privileges.
        [EFAULT] - The name parameter gave an invalid address.
        [EINVAL] - The name parameter pointed to a string that was too long.
                   Login names are limited to MAXLOGNAME (from <sys/param.h>)
                   characters, currently 16.

    BUGS

    SEE ALSO
        getlogin()

**********************************************************************
*/

AROS_LH1(int, setlogin,
         AROS_LHA(const char *, buffer, A1),
         struct UserGroupBase *, UserGroupBase, 37, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct proc *p;
    int error;

    D(bug("[UserGroup] %s()\n", __func__));

    if (buffer == NULL) {
        error = EFAULT;
    } else if (strlen(buffer) > MAXLOGNAME - 1) {
        error = EINVAL;
    } else {
        p = procfind((pid_t)NULL);

        error = suser(p->p_cred->pc_ucred);
        if (!error) {
            strncpy(p->p_session->s_login, buffer, MAXLOGNAME);
            p->p_session->s_login[MAXLOGNAME - 1] = '\0';
            return 0;
        }
    }

    ug_SetErrno((struct Library *)UserGroupBase, error);
    return -1;

    AROS_LIBFUNC_EXIT
}
