/*
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

/****** usergroup.library/--background-- ***********************************

    WARNING
        Unfortunately, this experimental release of usergroup.library is not
        compatible with multiuser.library.  There are some problems with
        multiuser.library, eg. the multiuser.library does not support the
        real ids.  Also the password format is different, multiuser.library
        uses the AS225r2 password format, which is very simple encoding.
        The usergroup.library uses the standard Unix password encryption.

        The current implementation of this model is very simple.  All tasks
        belong to one session and they share common credentials.  The
        setsid() function call does nothing.  You are supposed to log in
        using "login -f login-name" when the machine is booted.

    PURPOSE
        When the AmiTCP/IP was originally released, a little attention was
        paid to the security aspects.  Since the AmigaOS is basically a
        single user operating system with little or no provisions for
        multiple users, there was no standard how accounts, password
        checking and access control should be implemented.

    USERGROUP.LIBRARAY SEMANTICS
        The usergroup.library provides a BSD-stylish interface to the user
        and group identification, the account database, the group databases,
        password checking and login information.  Since it is a shared
        library instead of link library, the underlying security mechanisms
        can be changed according future standards and needs.  The
        usergroup.library provides quite clean basic model. Each process has
        credentials, which consist of real used ID, real group ID, effective
        user ID and up to 32 effective group IDs. The process credentials
        can be changed with setuid()/setgid()/setgroups() functions.

        Each process belongs also to an session.  A new session will created
        with setsid() function call, which is typically executed before you
        call command or when you create a new connection.  A session
        contains the login name of the user and possibly some other
        information.

        The information about users logging in and out is typically stored
        into a file in Unix systems.  These files (in BSD Net2 release they
        are /var/run/utmp and /var/log/wtmp) are usually very long and
        contain holes.  Since the AmigaDOS files cannot contain holes, this
        approach is not practical.  The usergroup.library provides an
        loosely HP-UX-stylish interface to the utmp and lastlogin databases.
        The utmp database contains an entry for each session, it is searched
        in linear manner qith getutent().  The lastlogin database contains
        an entry for each user and getlastlogin() returns an entry for given
        UID.

        The usergroup.library does not directly depend on AmiTCP/IP.  It can
        be used with any program needing user identification, account and
        group databases.

    USING USERGROUP.LIBRARY
        Each time the usergroup.library is opened, it creates an new
        instance of the library base.  The library base contains the static
        data buffers used by many library functions.  The usergroup.library
        functions behave exactly like they were in link library. The
        functions allocate all resources for you, the library also frees the
        resources when they are no more needed.

        Since each library contains static data and resources allocated in
        the context of calling task (ie. signals), only the task which
        opened the library is allowed to call most library functions.
        However, any task whatsoever can call following functions:

            getuid() geteuid() getgid() getegid() getsid()

        These functions return the credentials of calling task.

        It is also possible to call following functions from any task.
        However, note that a non-owning tasks cannot recover error codes:

            getgroups() setreuid() setuid() setregid() setgid() setgroups()
            setsid() setlogin()

        It is possible to give the library instance to another task.  Only
        the current owner can close the library.

        The user and group information is provided by netinfo.device.
        It is more convenient interface to user and group databases
        for multitasking applications.

    EXAMPLE PROGRAMS
        There are a few utilities provided as examples.  The finger programs
        deals with user (password), utmp and lastlog database, the id and
        whoami with user and group identification, login and passwd with
        password checking and password changing.

    SEE ALSO
        netinfo.device/--background--, ug_SetupContextTags(),
        SAS C Manual, libinit.c and libinitr.o

    COPYRIGHT
        Copyright © 1980--1991 The Regents of the University of California.
        Copyright © 1993, 1994 AmiTCP/IP-Group, <AmiTCP-Group@hut.fi>,
        Helsinki University of Technology, Finland.

****************************************************************************
*/

/****** usergroup.library/--Licence-- **************************************

   USERGROUP.LIBRARY LICENCE
        The usergroup.library is Copyright © 1993, 1994 AmiTCP/IP-Group,
        <AmiTCP-Group@hut.fi>, Helsinki University of Technology, Finland.

        The usergroup.library contains source code from 4.3BSD Net2 release.
        The 4.3BSD Net2 release is copyright © 1980 --- 1991 The Regents of
        the University of California.  The following licence apply to the
        usergroup.library and its documentation:

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions
        are met:

        1. Redistributions of source code must retain the above copyright
           notice, this list of conditions and the following disclaimer.
        2. Redistributions in binary form must reproduce the above copyright
           notice, this list of conditions and the following disclaimer in
           the documentation and/or other materials provided with the
           distribution.
        3. All advertising materials mentioning features or use of this
           software must display the following acknowledgement: This product
           includes software developed by the University of California,
           Berkeley and its contributors.
        4. Neither the name of the University nor the names of its
           contributors may be used to endorse or promote products derived
           from this software without specific prior written permission.

        THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
        AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
        TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
        PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
        CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
        SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
        LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
        USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
        ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
        OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
        OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
        SUCH DAMAGE.

****************************************************************************
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <dos/dos.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <sys/time.h>
#include "base.h"
#include <proto/usergroup.h>

#include <exec/memory.h>

#include <assert.h>

#include <sys/errno.h>
#include <exec/errors.h>
#include "credential.h"

struct DosLibrary *DOSBase;
#if !defined(__AROS__)
struct Library *UtilityBase;
#else
struct UtilityBase *UtilityBase;
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#include <proto/utility.h>

#include "assert.h"

#if defined(LC_LIBDEFS_FILE)
#include LC_LIBDEFS_FILE

/*
 * AROS Library initialization & cleanup
 */

static void CleanupNIO(struct Library *);

static int UserGroup__Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[UserGroup] %s()\n", __func__));

    CleanupUTMP((struct Library *)LIBBASE);
    CleanupNIO((struct Library *)LIBBASE);
    TimeCleanup((struct Library *)LIBBASE);
    if (DOSBase)
        CloseLibrary((void *)DOSBase);
    if (UtilityBase)
        CloseLibrary((struct Library *)UtilityBase);

    return TRUE;
}

static int UserGroup__Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[UserGroup] %s()\n", __func__));

    InitSemaphore(&LIBBASE->ni_lock);

    if ((DOSBase = (void *)OpenLibrary("dos.library", 37L)) &&
            (UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37L)) &&
            TimeInit((struct Library *)LIBBASE) == 0 && LRandomInit() == 0) {
        LIBBASE->owner = FindTask(NULL);
        Forbid();
        if (!(CredentialBase = OpenResource(CREDENTIALNAME))) {
            CredentialBase = CredentialInit("root");
        }
        Permit();
        if (CredentialBase)
            return TRUE;
    }
    UserGroup__Expunge(LIBBASE);
    return FALSE;
}

ADD2INITLIB(UserGroup__Init, 0)
ADD2EXPUNGELIB(UserGroup__Expunge, 0)
#endif

/*
 * Handle the netinfo device
 */

struct NetInfoReq *ug_OpenUnit(struct Library *ugBase, ULONG unit)
{
    struct UserGroupBase *UserGroupBase = (struct UserGroupBase *)ugBase;

    D(bug("[UserGroup] %s()\n", __func__));

    /* Check ownership */
    if (UserGroupBase->owner != FindTask(NULL)) {
        ug_SetErrno(ugBase, EDEADLK);
        return NULL;
    }

    /* Allocate port */
    if (UserGroupBase->niport == NULL) {
        UserGroupBase->niport = CreateMsgPort();
        if (UserGroupBase->niport == NULL)
            return NULL;
    }
    if (UserGroupBase->nireq == NULL) {
        UserGroupBase->nireq = CreateIORequest(UserGroupBase->niport, sizeof(*UserGroupBase->nireq));
        if (UserGroupBase->nireq == NULL)
            return NULL;
    }

    if (UserGroupBase->nidevice[unit]) {
        /* Already opened */
        UserGroupBase->nireq->io_Device = UserGroupBase->nidevice[unit];
        UserGroupBase->nireq->io_Unit = UserGroupBase->niunit[unit];
    } else {
        if (OpenDevice(NETINFONAME, unit, (struct IORequest *)UserGroupBase->nireq, 0L)) {
            return NULL;
        }

        UserGroupBase->nidevice[unit] = UserGroupBase->nireq->io_Device;
        UserGroupBase->niunit[unit] = UserGroupBase->nireq->io_Unit;
    }

    if (UserGroupBase->nibuffer[unit] == NULL) {
        UserGroupBase->nibuffer[unit] = AllocVec(MAXLINELENGTH, MEMF_PUBLIC);
        if (UserGroupBase->nibuffer[unit] == NULL)
            return NULL;
    }

    UserGroupBase->nireq->io_Length = MAXLINELENGTH;
    UserGroupBase->nireq->io_Data = UserGroupBase->nibuffer[unit];

    return UserGroupBase->nireq;
}

void ug_CloseUnit(struct Library *ugBase, ULONG unit)
{
    struct UserGroupBase *UserGroupBase = (struct UserGroupBase *)ugBase;

    D(bug("[UserGroup] %s()\n", __func__));

    ObtainSemaphore(&UserGroupBase->ni_lock);

    if (UserGroupBase->nidevice[unit]) {
        assert(UserGroupBase->nireq != NULL);

        UserGroupBase->nireq->io_Device = UserGroupBase->nidevice[unit];
        UserGroupBase->nireq->io_Unit = UserGroupBase->niunit[unit];

        CloseDevice((struct IORequest *)UserGroupBase->nireq);

        UserGroupBase->nidevice[unit] = NULL;
        UserGroupBase->niunit[unit] = NULL;
    }

    ReleaseSemaphore(&UserGroupBase->ni_lock);
}

static void CleanupNIO(struct Library *ugBase)
{
    struct UserGroupBase *UserGroupBase = (struct UserGroupBase *)ugBase;

    D(bug("[UserGroup] %s()\n", __func__));

    endpwent();
    endgrent();

    if (UserGroupBase->nibuffer[NETINFO_PASSWD_UNIT] != NULL)
        FreeVec(UserGroupBase->nibuffer[NETINFO_PASSWD_UNIT]);
    UserGroupBase->nibuffer[NETINFO_PASSWD_UNIT] = NULL;

    if (UserGroupBase->nibuffer[NETINFO_GROUP_UNIT] != NULL)
        FreeVec(UserGroupBase->nibuffer[NETINFO_GROUP_UNIT]);
    UserGroupBase->nibuffer[NETINFO_GROUP_UNIT] = NULL;

    if (UserGroupBase->nireq)
        DeleteIORequest(UserGroupBase->nireq), UserGroupBase->nireq = NULL;

    if (UserGroupBase->niport)
        DeleteMsgPort(UserGroupBase->niport), UserGroupBase->niport = NULL;
}

BYTE ug_DoIO(struct NetInfoReq *req)
{
    D(bug("[UserGroup] %s()\n", __func__));

    DoIO((struct IORequest *)req);
    return req->io_Error;
}

/****** usergroup.library/ug_SetupContextTags ********************************

    NAME
        ug_SetupContextTagList - Set up the caller context
        ug_SetupContextTags    -  varargs stub for ug_SetupContextTagList

    SYNOPSIS
        success = ug_SetupContextTagList(taglist)
          D0                               A1

        ULONG ug_SetupContextTagList(struct TagItem *);

        success = ug_SetupContextTags(...)

        ULONG ug_SetupContextTags(LONG tag, ...);


    FUNCTION
        The function ug_SetupContextTags() will prepare the library caller
        context.

    INPUTS
        taglist - pointer to taglist

        Currently, there are defined tags as follows:

        UGT_ERRNOPTR  - gives the pointer to the errno variable.  The error
                        variable is redirected to the scope of the task.  If
                        the pointer is NULL, no redirection is done anymore.

        UGT_ERRNOSIZE - specifies the size of the errno variable. Legal
                        values are 1, 2 and 4.  The UGT_ERRNOSIZE must be
                        given with same call if the UGT_ERRNOPTR is given a
                        non-NULL value.

        UGT_INTRMASK  - specifies the interrupt signal mask. All blocking
                        library calls will be interrrupted when a signal in
                        the break mask is received. The signals in the
                        `mask' are not cleared when a library call is
                        interrupted.  The signals in INTRMASK should be
                        allocated in the context of the owning task.

        UGT_OWNER     - changes the owner of this library instance.  The
                        UGT_OWNET tagData must be a valid task pointer or
                        NULL. If the pointer is NULL, the library will have
                        no owner and any task can become owner by calling
                        ug_SetupContextTagList(UGT_OWNER, FindTask(NULL),
                        TAG_END) ;

                        Most of the library calls are allowed only for the
                        owner of library.  Only the owner can CloseLibrary()
                        this library.

    RESULT
        If the call is successfull, value of 0 is returned. Otherwise the
        value -1 is returned. Old context is cleared, if an error occurs.
        The error code can be retrieved with function ug_GetErr().

    ERRORS
        [EINVAL]    An illegal input value was specified.

    BUGS
        Strange and unusual things will happen if the signal allocated for
        the use of the library is included in the mask.

    SEE ALSO
        ug_GetErr(), --background--

******************************************************************************
*/

AROS_LH2 (int, ug_SetupContextTagList,
          AROS_LHA(UBYTE *, name, A0),
          AROS_LHA(struct TagItem *, tagargs, A1),
          struct UserGroupBase *, UserGroupBase, 5, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    struct TagItem *tag;
    struct Task *caller = FindTask(NULL);

    if (UserGroupBase->owner != NULL && UserGroupBase->owner != caller) {
        /* We should */
        InMsg("ug_SetupContextTags: not an UserGroupBase->owner (%lx)", caller);
        return -1;
    }

    if (tagargs == NULL || name == NULL) {
        ug_SetErrno((struct Library *)UserGroupBase, EINVAL);
        return -1;
    }

    UserGroupBase->_ProgramName = name;

    if (tag = FindTagItem(UGT_OWNER, tagargs)) {
        short error;

        ObtainSemaphore(&UserGroupBase->ni_lock);

        if (UserGroupBase->owner != NULL && UserGroupBase->owner != caller) {
            if (tag->ti_Data != 0) {
                UserGroupBase->owner = (void *)tag->ti_Data;
                error = 0;
            } else {
                error = EINVAL;
            }
        } else {
            error = EPERM;
        }

        ReleaseSemaphore(&UserGroupBase->ni_lock);

        if (error) {
            ug_SetErrno((struct Library *)UserGroupBase, error);
            return -1;
        }
    }

    if (tag = FindTagItem(UGT_ERRNOLPTR, tagargs)) {
        UserGroupBase->errnop = (void *)tag->ti_Data;
        UserGroupBase->errnosize = es_long;
    }
    if (tag = FindTagItem(UGT_ERRNOWPTR, tagargs)) {
        UserGroupBase->errnop = (void *)tag->ti_Data;
        UserGroupBase->errnosize = es_word;
    }
    if (tag = FindTagItem(UGT_ERRNOBPTR, tagargs)) {
        UserGroupBase->errnop = (void *)tag->ti_Data;
        UserGroupBase->errnosize = es_byte;
    }
#if (0)
    if (tag = FindTagItem(UGT_INTRMASK, tagargs)) {
        break_mask = tag->ti_Data;
    }
#endif

    return 0;

    AROS_LIBFUNC_EXIT
}
