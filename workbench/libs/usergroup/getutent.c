/*
 * Login database functions
 *
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

/****** usergroup.library/getutent *******************************************

    NAME
        getutsid, getutent, setutent, endutent - utmp database operations

    SYNOPSIS
        #include <utmp.h>

        utmpent = getutsid(sid)
           D0              D0
        struct utmp *getutsid(long);

        utmpent = getutent()
           D0
        struct utmp *getutent(void);

        setutent()
        void setutent(void);

        endutent()
        void endutent(void);

    FUNCTION
        These functions operate on the utmp database.  There is an utmp
        entry for each active session.  A session is started with login
        command and finished with logout command.

        The entry returned by each reading function is defined by the
        structure utmp found in the include file <utmp.h>:

            struct utmp {
              long    ut_time;              \* the login time *\
              long    ut_sid;               \* session ID *\
              char    ut_name[UT_NAMESIZE]; \* the login name *\
              char    ut_line[UT_LINESIZE]; \* the name of login device *\
              char    ut_host[UT_HOSTSIZE]; \* where the login originated *\
            };

        The getutsid() function search the utmp database for the given
        session id, returning the first one encountered.  The getutent()
        function sequentially reads the utmp database.  Both functions also
        open the utmp database, if necessary.

        The setutent() function opens the utmp database.  The
        endutent() function closes the utmp database.  It is
        recommended to call endutent() if the program won't access
        utmp database any more.

    RESULTS
        The functions getutsid() and getutent() return a pointer to the utmp
        entry if successful; if the end of database is reached or an error
        occurs a null pointer is returned.  The functions endutent() and
        setutent() have no return value.

    ERRORS
        [ENOENT] -- no utmp entries were available.

    SEE ALSO

    BUGS
        The getutent() and getutsid() function leave their result in an
        internal static object and return a pointer to that object.
        Subsequent calls to the same function will modify the same object.

        Current implementation allows only one user to be logged in
        concurrently.

*****************************************************************************
*
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <sys/time.h>

#include "base.h"

#include <proto/usergroup.h>

#include "credential.h"

#include <utmp.h>
#include <string.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <proto/dos.h>

static void lock_for_writing(void)
{
}

static void unlock_for_writing(void)
{
}

#define UTENTS 2

void CleanupUTMP(struct Library *ugBase)
{
    struct UserGroupBase *UserGroupBase = (struct UserGroupBase *)ugBase;

    D(bug("[UserGroup] %s()\n", __func__));

    endutent();

    if (UserGroupBase->utentbuf != NULL) {
        FreeVec(UserGroupBase->utentbuf);
        UserGroupBase->utentbuf = NULL;
    }
}

AROS_LH0(void, setutent,
         struct UserGroupBase *, UserGroupBase, 38, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    if (!FindResident("security.library")) {
        // Single-User ...
        UserGroupBase->already_read = 0;
        if (UserGroupBase->utentbuf == NULL)
            UserGroupBase->utentbuf = AllocVec(sizeof(*UserGroupBase->utent), MEMF_CLEAR | MEMF_PUBLIC);
    } else {
        // Multi-User ...
        if (UserGroupBase->utentbuf == NULL)
            UserGroupBase->utentbuf = AllocVec(sizeof(*UserGroupBase->utent) * UTENTS, MEMF_CLEAR | MEMF_PUBLIC);
        if (UserGroupBase->utentbuf != NULL) {
            UserGroupBase->utent_left = 0;
            if (UserGroupBase->utfile == NULL) {
                UserGroupBase->utfile = Open(_PATH_UTMP, MODE_READWRITE);
            } else {
                Seek(UserGroupBase->utfile, 0, OFFSET_BEGINNING);
            }
        }
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct utmp *, getutent,
         struct UserGroupBase *, UserGroupBase, 39, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    if (!FindResident("security.library")) {
        // Single-User ...
        if (!UserGroupBase->already_read) {
            if (UserGroupBase->utentbuf == NULL) {
                setutent();
            }
            if (UserGroupBase->utentbuf) {
                UserGroupBase->already_read = 1;
                CopyMem(CredentialBase->r_utmp, UserGroupBase->utentbuf, sizeof(*CredentialBase->r_utmp));
                return UserGroupBase->utentbuf;
            }
        }
        ug_SetErrno((struct Library *)UserGroupBase, ENOENT);
    } else {
        // Multi-User ...

        if (UserGroupBase->utentbuf == NULL) {
            setutent();
        }

        if (UserGroupBase->utfile) {
            for (;;) {
                if (UserGroupBase->utent_left == 0) {
                    LONG read;

                    UserGroupBase->uttell = Seek(UserGroupBase->utfile, 0, OFFSET_CURRENT);
                    read = Read(UserGroupBase->utfile, UserGroupBase->utentbuf, sizeof(*UserGroupBase->utent) * UTENTS);

                    UserGroupBase->utent = UserGroupBase->utentbuf;
                    if (read >= sizeof(*UserGroupBase->utent)) {
                        UserGroupBase->utent_left = read / sizeof(*UserGroupBase->utent);
                    } else {
                        break;
                    }
                } else {
                    UserGroupBase->uttell += sizeof(*UserGroupBase->utent);
                }

                UserGroupBase->utent_left--;
                if (UserGroupBase->utent->ut_time != 0)
                    return UserGroupBase->utent++;
                UserGroupBase->utent++;
            }

            ug_SetErrno((struct Library *)UserGroupBase, ENOENT);
        } else {
            if (UserGroupBase->utentbuf)
                ug_SetErrno((struct Library *)UserGroupBase, ENOENT);
            else
                ug_SetErrno((struct Library *)UserGroupBase, ENOMEM);
        }
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, endutent,
         struct UserGroupBase *, UserGroupBase, 40, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    if (!FindResident("security.library")) {
        // Single-User ...
        UserGroupBase->already_read = 0;
    } else {
        // Multi-User ...

        if (UserGroupBase->utfile != NULL) {
            Close(UserGroupBase->utfile), UserGroupBase->utfile = NULL;
        }
    }

    AROS_LIBFUNC_EXIT
}

#if (0)
AROS_LH1(struct utmp *, getutsid,
         AROS_LHA(pid_t, sid, D0),
         struct UserGroupBase *, UserGroupBase, 40, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct utmp *ut;

    for (setutent(); ut = getutent();) {
        if (ut->ut_sid == sid)
            return ut;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}
#endif

/*****************************************************************************

    NAME */
#include <utmp.h>
        AROS_LH1(struct lastlog *, getlastlog,

/*  SYNOPSIS */
        AROS_LHA(uid_t, uid, D0),

/* LOCATION */
        struct UserGroupBase *, UserGroupBase, 41, Usergroup)

/*  FUNCTION

        The getlastlog() function search the lastlog database for the given
        user id. There should be an lastlog entry for each user.  A lastlog
        entry with ll_time being zero means that user has never logged in
        this system.

        The entry returned by getlastlog is defined by the structure lastlog
        found in the include file <utmp.h>:

              struct lastlog {
                long  ll_time;              \* the login time *\
                uid_t ll_uid;               \* user ID *\
                char  ll_name[UT_NAMESIZE]; \* the login name *\
                char  ll_line[UT_LINESIZE]; \* the name of login device *\
                char  ll_host[UT_HOSTSIZE]; \* where the login originated *\
              };

    RESULTS
        The function getlastlog() returns a pointer to the lastlog entry if
        successful; if an error occurs a null pointer is returned.

    ERRORS
        [ENOENT] -- no lastlog entry was found
        [EINVAL] -- the user ID was illegal

    SEE ALSO
        setlastlog()

    BUGS
        The getlastlog() function leaves its result in an internal static
        object and return a pointer to that object.  Subsequent calls to the
        same function will modify the same object.

        Current implementation stores only the lastlog data of the latest
        user logged in.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    static struct lastlog ll[1];

    D(bug("[UserGroup] %s()\n", __func__));

    if (!FindResident("security.library")) {
        // Single-User ...
        if (uid == CredentialBase->r_lastlog->ll_uid) {
            *ll = *CredentialBase->r_lastlog;
        } else {
            bzero(ll, sizeof(*ll));
            ll->ll_uid = uid;
        }
    } else {
        BPTR llfile = Open(_PATH_LASTLOG, MODE_OLDFILE);
        BOOL never = TRUE;

        // Multi-User ...

        UserGroupBase->lltell = 0;

        if (llfile) {
            while (Read(llfile, ll, sizeof(*ll)) == sizeof(*ll)) {
                UserGroupBase->lltell += sizeof(*ll);
                if (ll->ll_uid == uid) {
                    never = FALSE;
                    break;
                }
            }
            Close(llfile);
        }

        if (never) {
            ll->ll_time = 0;
            ll->ll_uid = uid;
            ll->ll_name[0] = '\0';
            ll->ll_line[0] = '\0';
            ll->ll_host[0] = '\0';
            UserGroupBase->lltell = -1;
        }
    }

    return ll;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        #include <utmp.h>
        AROS_LH4(int, setlastlog,

/*  SYNOPSIS */
        AROS_LHA(uid_t, uid, D0),
        AROS_LHA(char *, name, A0),
        AROS_LHA(char *, con, A1),
        AROS_LHA(char *, host, A2),

/* LOCATION */
        struct UserGroupBase *, UserGroupBase, 42, Usergroup)

/*  FUNCTION
        The setlastlog function is used to register user logging in.  Each
        time a user is logging in, the function setlastlog() should be
        called to register that event.

    INPUTS
        uid     -- the uid of user logging in
        name    -- the user login name
        console -- the console handler name (from ug_GetConsole())
        host    -- the host which the user is logging in from

    RESULTS
        The setlastlog() function returns an success indicator, 0 if the
        call was successful, -1 otherwise.  The error code is set if an
        error occurs.

    ERRORS
        The setlastlog() can have following error codes:
        [EFAULT] -- the utmp entry cannot be accessed
        [ENOMEM] -- the memory has been exhausted
        [ENOENT] -- cannot access utmp database

    BUGS
        Current implementation stores only the lastlog data of the latest
        user logged in.

    FILES

    SEE ALSO
        getutent(), getlastlog()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    short error = 0;
    struct proc *p = procfind((pid_t)NULL);
    pid_t sid = p->p_session->s_leader;

    D(bug("[UserGroup] %s()\n", __func__));

    if (name == NULL || con == NULL) {
        error = EFAULT;
    } else {
        if (!FindResident("security.library")) {
            struct utmp utmp[1];
            struct timeval now[1];

            // Single-User ...

            bzero((void *)utmp, sizeof(*utmp));

            gettimeofday(now, NULL);
            utmp->ut_time = now->tv_sec;
            utmp->ut_sid = (long)sid;

            strncpy(utmp->ut_name, name, sizeof(utmp->ut_name));
            if (host)
                strncpy(utmp->ut_host, host, sizeof(utmp->ut_host));

            *CredentialBase->r_utmp = *utmp;

            /*
            * This trick requires that
            * lastlog and utmp are essentially identical..
            */
            utmp->ut_sid = (long) uid;
            *CredentialBase->r_lastlog = *(struct lastlog *)utmp;
        } else {
            struct utmp *oldut, utmp[1];
            struct timeval now[1];

            // Multi-User ...

            bzero((void *)utmp, sizeof(*utmp));
            gettimeofday(now, NULL);
            utmp->ut_time = now->tv_sec;
            utmp->ut_sid = sid;
            strncpy(utmp->ut_name, name, sizeof(utmp->ut_name));
            if (sid != 0)
                strncpy(utmp->ut_line, con, sizeof(utmp->ut_line));
            else
                strcpy(utmp->ut_line, "ALL");
            if (host)
                strncpy(utmp->ut_host, host, sizeof(utmp->ut_host));

            setutent();

            /* Writing lock */
            lock_for_writing();

            if (UserGroupBase->utentbuf == NULL) {
                error = ENOMEM;
            } else if (UserGroupBase->utfile == NULL) {
                error = ENOENT;
            } else {
                LONG utslot = -1;

                while (oldut = getutent()) {
                    if (oldut->ut_sid == sid) {
                        utslot = UserGroupBase->uttell;
                        break;
                    }
                    if (oldut->ut_time == 0 && utslot == -1) {
                        utslot = UserGroupBase->uttell;
                    }
                }
                if (oldut == NULL)
                    utslot = UserGroupBase->uttell;

                Seek(UserGroupBase->utfile, UserGroupBase->uttell, OFFSET_BEGINNING);
                Write(UserGroupBase->utfile, utmp, sizeof(*utmp));
            }

            endutent();

            {
                BPTR llfile = Open(_PATH_LASTLOG, MODE_READWRITE);

                utmp->ut_sid = (long)uid;

                if (llfile) {
                    if (UserGroupBase->lltell > 0) {
                        Seek(llfile, UserGroupBase->lltell, OFFSET_BEGINNING);
                    } else {
                        LONG tell;
                        Seek(llfile, 0, OFFSET_END);
                        tell = Seek(llfile, 0, OFFSET_CURRENT);
                        tell = (tell / sizeof(*utmp)) * sizeof(*utmp);
                        Seek(llfile, tell, OFFSET_BEGINNING);
                    }
                    Write(llfile, utmp, sizeof(*utmp));
                    Close(llfile);
                }
            }
            unlock_for_writing();
        }
    }

    if (error != 0) {
        ug_SetErrno((struct Library *)UserGroupBase, error);
        return -1;
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
