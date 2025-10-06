/*
 * User database functions
 *
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

/****** usergroup.library/getpwent *******************************************

   NAME
        getpwent, getpwnam, getpwuid, setpwent, endpwent
         - password database operations

   SYNOPSIS
        #include <pwd.h>

        pw = getpwuid(uid)
        D0            D0
        struct passwd *getpwuid(uid_t);


        pw = getpwnam(name)
        D0             A1
        struct passwd *getpwnam(const char *);

        pw = getpwent()
        D0
        struct passwd *getpwent(void);

        setpwent()
        void setpwent(void);

        endpwent()
        void endpwent(void);

   FUNCTION
        These functions operate on the user database via netinfo.device
        interface. They provide convenient unix-compatible interface to the
        password unit of the netinfo.device.

        The local password database is stored in the file AmiTCP:db/passwd,
        its format is described in netinfo.device/passwd.  The entry
        returned by each reading function is defined by the structure passwd
        found in the include file <pwd.h>:

               struct passwd
               {
                 char  *pw_name;         \* Username *\
                 char  *pw_passwd;       \* Encrypted password *\
                 pid_t  pw_uid;          \* User ID *\
                 gid_t  pw_gid;          \* Group ID *\
                 char  *pw_gecos;        \* Real name etc *\
                 char  *pw_dir;          \* Home directory *\
                 char  *pw_shell;        \* Shell *\
               };

        The functions getpwnam() and getpwuid() search the password database
        for the given login name or user uid, respectively, always returning
        the first one encountered.

        The getpwent() function sequentially reads the password database and
        is intended for programs that wish to process the complete list of
        users.

        All three routines will open the password unit of netinfo.device for
        reading, if necesssary.

        The setpwent() function opens the password unit of netinfo.device.
        The endpwent() function closes the password unit of netinfo.device.
        It is recommended to call endpwent() if the program won't access
        password database any more.

   RESULTS
        The functions getpwent(), getpwnam() and getpwuid() return a valid
        pointer to a passwd structure on success and a null pointer if end
        of database is reached or an error occurs. The functions endpwent()
        and setpwent() have no return value.

    ERRORS
        [ENOENT] -- the netinfo.device could not be opened.

        Other netinfo.device IO errors can be retrieved by ug_GetErr().

   FILES
        AmiTCP:db/passwd    The password database file

   SEE ALSO
        getgrent(), netinfo.device/passwd

   HISTORY
        The functions getpwent(), getpwnam(), getpwuid(), setpwent() and
        endpwent() functions appeared in Version 7 AT&T UNIX.

   BUGS
        These functions leave their results in an internal static object and
        return a pointer to that object. Subsequent calls to these function
        will modify the same object. If you need re-entrant operation, you
        should use directly the netinfo.device.

   COMPATIBILITY
        The BSD passwd database handling routines setpwfile() and
        setpassent() are fairly useless in a networked environment and they
        are not implemented.

*****************************************************************************
*
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include "base.h"
#include <proto/usergroup.h>

AROS_LH1(struct passwd *, getpwnam,
         AROS_LHA(const char *, name, A1),
         struct UserGroupBase *, UserGroupBase, 19, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct NetInfoReq *nreq;
    struct passwd *pw = NULL;

    D(bug("[UserGroup] %s('%s')\n", __func__, name));

    if (name == NULL) {
        ug_SetErrno((struct Library *)UserGroupBase, EFAULT);
        return NULL;
    }

    ObtainSemaphore(&UserGroupBase->ni_lock);
    if (nreq = ug_OpenUnit((struct Library *)UserGroupBase, NETINFO_PASSWD_UNIT)) {
        pw = (struct passwd *)nreq->io_Data;
        pw->pw_name = (char *)name;
        nreq->io_Command = NI_GETBYNAME;
        D(bug("[UserGroup] %s: sending NI_GETBYNAME to netinfo.device/%d...\n", __func__, NETINFO_PASSWD_UNIT));
        if (ug_DoIO(nreq) == 0) {
            D(bug("[UserGroup] %s:  -> %d\n", __func__, pw->pw_uid));
        } else {
            pw = NULL;
            SetDeviceErr((struct Library *)UserGroupBase);
        }
    } else {
        SetDeviceErr((struct Library *)UserGroupBase);
    }

    ReleaseSemaphore(&UserGroupBase->ni_lock);

    return pw;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct passwd *, getpwuid,
         AROS_LHA(uid_t, uid, D0),
         struct UserGroupBase *, UserGroupBase, 20, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct NetInfoReq *nreq;
    struct passwd *pw = NULL;

    D(bug("[UserGroup] %s(%d)\n", __func__, uid));

    ObtainSemaphore(&UserGroupBase->ni_lock);
    if (nreq = ug_OpenUnit((struct Library *)UserGroupBase, NETINFO_PASSWD_UNIT)) {
        pw = (struct passwd *)nreq->io_Data;            
        pw->pw_uid = uid;
        nreq->io_Command = NI_GETBYID;
        D(bug("[UserGroup] %s: sending NI_GETBYID to netinfo.device/%d...\n", __func__, NETINFO_PASSWD_UNIT));
        if (ug_DoIO(nreq) == 0) {
            D(bug("[UserGroup] %s:  -> '%s'\n", __func__, pw->pw_name));
        } else {
            pw = NULL;
            SetDeviceErr((struct Library *)UserGroupBase);
        }
    } else {
        D(bug("[UserGroup] %s: failed to open unit\n", __func__));
        SetDeviceErr((struct Library *)UserGroupBase);
    }

    ReleaseSemaphore(&UserGroupBase->ni_lock);

    D(bug("[UserGroup] %s: returning 0x%p\n", __func__, pw));
    return pw;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, setpwent,
         struct UserGroupBase *, UserGroupBase, 21, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct NetInfoReq *nreq;

    D(bug("[UserGroup] %s()\n", __func__));

    ObtainSemaphore(&UserGroupBase->ni_lock);

    if (nreq = ug_OpenUnit((struct Library *)UserGroupBase, NETINFO_PASSWD_UNIT)) {
        nreq->io_Command = CMD_RESET;
        D(bug("[UserGroup] %s: sending CMD_RESET to netinfo.device/%d...\n", __func__, NETINFO_PASSWD_UNIT));
        ug_DoIO(nreq);
        UserGroupBase->setent_done = 1;
    } else {
        SetDeviceErr((struct Library *)UserGroupBase);
    }

    ReleaseSemaphore(&UserGroupBase->ni_lock);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct passwd *, getpwent,
         struct UserGroupBase *, UserGroupBase, 22, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct NetInfoReq *nreq;
    struct passwd *pw = NULL;

    D(bug("[UserGroup] %s()\n", __func__));

    ObtainSemaphore(&UserGroupBase->ni_lock);
    if (nreq = ug_OpenUnit((struct Library *)UserGroupBase, NETINFO_PASSWD_UNIT)) {
        /* do setpwent() if necessary */
        if (!UserGroupBase->setent_done) {
            nreq->io_Command = CMD_RESET;
            D(bug("[UserGroup] %s: sending CMD_RESET to netinfo.device/%d...\n", __func__, NETINFO_PASSWD_UNIT));
            ug_DoIO(nreq);
            UserGroupBase->setent_done = 1;
        }
        D(bug("[UserGroup] %s: sending CMD_READ to netinfo.device/%d...\n", __func__, NETINFO_PASSWD_UNIT));
        nreq->io_Command = CMD_READ;
        if (ug_DoIO(nreq) == 0) {
            pw = (struct passwd *)nreq->io_Data;
        } else {
            SetDeviceErr((struct Library *)UserGroupBase);
        }
    } else {
        SetDeviceErr((struct Library *)UserGroupBase);
    }

    ReleaseSemaphore(&UserGroupBase->ni_lock);

    D(bug("[UserGroup] %s: returning 0x%p\n", __func__, pw));
    return pw;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, endpwent,
         struct UserGroupBase *, UserGroupBase, 23, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    ObtainSemaphore(&UserGroupBase->ni_lock);
    UserGroupBase->setent_done = 0;
    ug_CloseUnit((struct Library *)UserGroupBase, NETINFO_PASSWD_UNIT);
    ReleaseSemaphore(&UserGroupBase->ni_lock);

    AROS_LIBFUNC_EXIT
}
