/*
 * credential.c --- handle credentials
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

#include "base.h"
#include "credential.h"
#include <proto/usergroup.h>

#include <exec/memory.h>
#include <exec/alerts.h>
#include <string.h>
#include <assert.h>

struct CredentialResource *CredentialBase;

static struct UserGroupCredentials creds[1];

#ifdef notyet
typedef struct CredentialResource *(*resident_init_fp)(void);

/*
 * Create credential resource
 */
struct CredentialResource *CredentialInit(const char *name)
{
    BPTR resseg = LoadSeg(_PATH_CREDENTIAL);

    D(bug("[UserGroup] %s()\n", __func__));

    if (resseg) {
        resident_init_fp initf = (resident_init_fp)((LONG *)(resseg << 2) + 1);
        struct CredentialResource *res = initf();

        if (res)
            return res;

        UnLoadSeg(resseg);
    }

    return NULL;
}
#endif


/*****************************************************************************

    NAME */
        AROS_LH0I(pid_t, setsid,

/*  SYNOPSIS */

/* LOCATION */
        struct Library *, UserGroupBase, 34, Usergroup)

/*  FUNCTION
        The setsid() function creates a new session when the calling
        process is not a process group leader.  The calling process
        then becomes the session leader of this session and the only
        process in the new session.

    RESULTS
        Upon successful completion, the value of the new session ID is
        returned.  Otherwise, a value of -1 is returned and an error
        code is  stored to global errno location.

    ERRORS
        [EPERM]   The calling process is already a session leader.

    SEE ALSO
         getpgrp()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

#ifdef notyet
    pid_t pid = (pid_t)FindTask(NULL);
    struct proc *p = procfind(pid);
    int error;

    if (p->p_session->s_leader == pid) {
        error = EPERM;
    } else {
        struct session *session;

        while (!(session = AllocMem(sizeof(*session), MEMF_CLEAR|MEMF_PUBLIC))) {
            Alert(AG_NoMemory | UG_CredRes);
            Wait(0L);
        }
        InitList((struct List *)session->s_pgrpl);
        Forbid();
        p = proccopy(p, pid);
        p->p_ucred->cr_ref++;
        p->p_ucred = crcopy(p->p_ucred);

        /* Remove from old process group */
        rempgrp(p);
        p->p_session = session;
        MinAddHead(session->s_pgrpl, p->p_pgrp);
        session->s_consoletask = GetConsoleTask();
        session->s_leader = pid;
        Permit();
        return pid;
    }

    ug_SetErrno((struct Library *)UserGroupBase, error);
    return -1;
#endif
    return 0;

    AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    NAME */
        AROS_LH0I(pid_t, getpgrp,

/*  SYNOPSIS */

/* LOCATION */
                struct Library *, UserGroupBase, 35, Usergroup)

/*  FUNCTION
        The getpgrp() function returns the process group id for the current
        process.  Currently, the process group ID is the same as the session
        ID. The 0 is valid process group ID for console session.

    RESULTS
        Upon successful completion, the value of the process group ID is
        returned.  Otherwise, a value of -1 is returned and an error code is
        stored to global errno location.

    ERRORS
        [ESRCH]   The calling process don't belong to any process group.

    SEE ALSO
         setsid(),  exec.library/FindTask()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct proc *p = procfind(0);

    D(bug("[UserGroup] %s()\n", __func__));

    return p->p_session->s_leader;

    AROS_LIBFUNC_EXIT
}

#ifdef notyet
/*
 * Remove a process from session's process group and
 * free session if last process is removed
 */
void
rempgrp(struct proc *p)
{
    Forbid();
    /* Free the session structure if the last task in process group dies */
    MinRemove((struct Node *)p->p_pgrp);
    if (IsMinListEmpty(p->p_session->s_pgrpl)) {
        FreeMem(p->p_session, sizeof(*p->p_session));
    } else if (p->p_session->s_leader == (pid_t)p->p_task) {

    }
    Permit();
}
#endif

/*
 * Check super-user
 */
int
suser(struct ucred *cred)
{
    if (cred->cr_uid == 0)
        return 0;
    return EPERM;
}

/*
 * Search credentials
 */
static struct pcred *crfind(pid_t pid)
{
    return procfind(pid)->p_cred;
}

/****** usergroup.library/getuid *********************************************

    NAME
        getuid, geteuid - get user process identification
        getgid, getegid - get group process identification

    SYNOPSIS
        ruid = getuid()
        D0

        uid_t getuid(void);

        euid = geteuid()
        D0

        uid_t geteuid(void);

        rgid = getgid()
        D0

        gid_t getgid(void);

        egid = getegid()
        D0

        gid_t getegid(void);

    FUNCTION
        The getuid() function returns the real user ID of the calling
        process, geteuid() returns the effective user ID of the calling
        process.

        The getgid() function returns the real group ID of the calling
        process, getegid() returns the effective group ID of the calling
        process.

        The real user ID and real group ID is specified at login time.

        The real ID is the ID of the user who invoked the program.  As the
        effective user and gourp ID gives the process additional permissions
        during the execution of `set-user-ID' or `set-group-ID' mode
        programs, functions getgid() and getuid () are used to determine the
        real-ids of the calling process.

    RESULT
        The getuid(), geteuid(), getgid(), and getegid() functions are
        always successful, and no return value is reserved to indicate an
        error.

    NOTES
        Any task can call these functions

    SEE ALSO
        getgroups(), setuid(), setreuid(), setgid(), setregid(), setgroups()

******************************************************************************
*/

AROS_LH0I(uid_t, getuid,
          struct Library *, UserGroupBase, 8, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    return crfind((pid_t)NULL)->p_ruid;

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(uid_t, geteuid,
          struct Library *, UserGroupBase, 9, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    return crfind((pid_t)NULL)->p_euid;

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(gid_t, getgid,
          struct Library *, UserGroupBase, 12, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    return crfind((pid_t)NULL)->p_rgid;

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(gid_t, getegid,
          struct Library *, UserGroupBase, 13, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    return crfind((pid_t)NULL)->p_egid;

    AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    NAME */

        AROS_LH2 (int, getgroups,

/*  SYNOPSIS */
        AROS_LHA(int, ngroups, D0),
        AROS_LHA(gid_t *, groups, A1),

/* LOCATION */
        struct UserGroupBase *, UserGroupBase, 16, Usergroup)

/*  FUNCTION
        Getgroups() gets the current group access list of the user process
        and stores it in the array gidset. The parameter gidsetlen indicates
        the number of entries that may be placed in gidset. The function
        getgroups() returns the actual number of groups returned in gidset.
        No more than NGROUPS, as defined in <libraries/usergroup.h>, will
        ever be returned.

     RESULT
        A successful call returns the number of groups in the group set.  A
        value of -1 indicates that the argument gidsetlen is smaller than
        the number of groups in the group set.

    ERRORS
        [EINVAL] The argument gidsetlen is smaller than the number of
                 groups in the group set.
        [EFAULT] The argument gidset specifies an invalid address.

    SEE ALSO
        setgroups(), initgroups(), getgid(), getegid()

    HISTORY
        The getgroups function call appeared in 4.2BSD.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct pcred *pc = crfind((pid_t)NULL);
    gid_t *getgroups;
    short error;

    D(bug("[UserGroup] %s()\n", __func__));

    if (ngroups == 0) {
        return pc->p_ngroups;
    } else if (ngroups < pc->p_ngroups) {
        error = EINVAL;
    } else if (groups == NULL || ((SIPTR)groups & 1) != 0) {
        error = EFAULT;
    } else {
        /* This does also UWORD -> gid_t conversion */
        getgroups = pc->p_groups;
        ngroups = pc->p_ngroups;
        while (ngroups-- > 0) {
            *groups++ = *getgroups++;
        }
        return pc->p_ngroups;
    }

    ug_SetErrno((struct Library *)UserGroupBase, error);
    return -1;

    AROS_LIBFUNC_EXIT
}

AROS_LH2 (int, setreuid,
          AROS_LHA(uid_t, ruid, D0),
          AROS_LHA(uid_t, euid, D1),
          struct UserGroupBase *, UserGroupBase, 10, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct pcred *pc = crfind((pid_t)NULL);
    short error;

    D(bug("[UserGroup] %s()\n", __func__));

    if (ruid == NOID)
        ruid = pc->p_ruid;
    if (euid == NOID)
        euid = pc->pc_ucred->cr_uid;

    /*
    * Allow setting real uid to previous effective, for swapping real and
    * effective.  This should be:
    *
    * if (ruid != pc->p_ruid &&
    *     (error = suser(pc->pc_ucred, &p->p_acflag)))
    */
#if 1
    /* allow setreuid(-1, 0) to emulate SUID */
    if ((ruid != pc->p_ruid && ruid != pc->pc_ucred->cr_uid /* XXX */ &&
            (error = suser(pc->pc_ucred))) ||
            (/* euid != pc->pc_ucred->cr_uid */ 0 && euid != pc->p_ruid &&
                    (error = suser(pc->pc_ucred))))
#else
    if ((ruid != pc->p_ruid && ruid != pc->pc_ucred->cr_uid /* XXX */ &&
            (error = suser(pc->pc_ucred))) ||
            (euid != pc->pc_ucred->cr_uid && euid != pc->p_ruid &&
             (error = suser(pc->pc_ucred))))
#endif
    {
        unlock(&credential_list);
        ug_SetErrno((struct Library *)UserGroupBase, error);
        return -1;
    } else {
        /*
         * Everything's okay, do it.  Copy credentials so other references do
         * not see our changes.
         */
        pc->pc_ucred = crcopy(pc->pc_ucred);
        pc->pc_ucred->cr_uid = euid;
        pc->p_ruid = ruid;

        return 0;
    }

    AROS_LIBFUNC_EXIT
}

/****** usergroup.library/setuid ***********************************************

    NAME
        setuid, setreuid - set real and effective user ID's
        setgid, setregid - set real and effective group ID's

    SYNOPSIS
        success = setuid(uid)
        D0               D0

        int setuid(uid_t);

        success = setreuid(ruid, euid);
        D0                 D0    D1

        int setreuid(uid_t, uid_t);

        success = setgid(gid)
        D0               D0

        int setgid(gid_t);

        success = setregid(ruid, euid)
        D0                 D0    D1

        int setregid(gid_t ruid, gid_t euid);

    FUNCTION
        The real and effective ID's of the current process are set according
        to the arguments.  If ruid or euid is -1, the current uid is filled
        in by the system.  Unprivileged users may change the real ID to the
        effective ID and vice-versa; only the super-user may make other
        changes.

    RETURN VALUES
        Upon successful completion, a value of 0 is returned. Otherwise, a
        value of -1 is returned and errno is set to indicate the error.

    ERRORS
        [EPERM]  The current process is not the super-user and a change
                 other than changing the effective id to the real id was
                 specified.

    SEE ALSO
        getuid(), getgid(), geteuid(), getegid()

    NOTES
        Any task can call these functions.

    HISTORY
        A setuid() and setgid() function calls appeared in Version 6 AT&T
        UNIX.  The setreuid() and setregid() function calls appeared in
        4.2BSD.

****************************************************************************
*/

AROS_LH1 (int, setuid,
          AROS_LHA(uid_t, uid, D0),
          struct UserGroupBase *, UserGroupBase, 11, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    return setreuid(uid, uid);

    AROS_LIBFUNC_EXIT
}

AROS_LH2 (int, setregid,
          AROS_LHA(gid_t, rgid, D0),
          AROS_LHA(gid_t, egid, D1),
          struct UserGroupBase *, UserGroupBase, 14, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct pcred *pc = crfind((pid_t)NULL);
    short error;

    D(bug("[UserGroup] %s()\n", __func__));

    if (rgid == NOID)
        rgid = pc->p_rgid;
    if (egid == NOID)
        egid = pc->pc_ucred->cr_gid;

    /*
    * Allow setting real gid to previous effective, for swapping real and
    * effective.  This should be:
    *
    * if (rgid != pc->p_rgid &&
    *     (error = suser(pc->pc_ucred, &p->p_acflag)))
    */
    if ((rgid != pc->p_rgid && rgid != pc->pc_ucred->cr_gid /* XXX */ &&
            (error = suser(pc->pc_ucred))) ||
            (egid != pc->pc_ucred->cr_gid && egid != pc->p_rgid &&
             (error = suser(pc->pc_ucred)))) {
        ug_SetErrno((struct Library *)UserGroupBase, error);
        return -1;
    } else {
        /*
         * Everything's okay, do it.  Copy credentials so other references do
         * not see our changes.
         */
        pc->pc_ucred = crcopy(pc->pc_ucred);
        pc->pc_ucred->cr_gid = egid;
        pc->p_rgid = rgid;

        return 0;
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1 (int, setgid,
          AROS_LHA(gid_t, gid, D0),
          struct UserGroupBase *, UserGroupBase, 15, Usergroup)
{
    AROS_LIBFUNC_INIT

    D(bug("[UserGroup] %s()\n", __func__));

    return setregid(gid, gid);

    AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    NAME */

        AROS_LH2(int, setgroups,

/*  SYNOPSIS */
        AROS_LHA(int, ngrp, D0),
        AROS_LHA(const gid_t *, groups, A1),

/* LOCATION */
        struct UserGroupBase *, UserGroupBase, 17, Usergroup)

/*  FUNCTION
        Setgroups() sets the group access list of the current user process
        according to the array gidset. The parameter ngroups indicates the
        number of entries in the array and must be no more than NGROUPS, as
        defined in <libraries/usergroup.h>.

        Only the super-user may set new groups. The super-user can not
        set illegal groups (-1).

    RESULT
        A 0 value is returned on success, -1 on error, with an error
        code stored in errno and available with ug_GetErr() function.

    ERRORS
        [EINVAL]    An illegal group id was specified.
        [EPERM]     The caller has got no necessary privileges.
        [EFAULT]    The address specified for gidset is illegal.

    NOTES
        Any task can call this function.

    SEE ALSO
        getgroups(), initgroups()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    register struct pcred *pc = crfind((pid_t)NULL);
    register gid_t *gp;
    register const gid_t *lp;
    short error;

    D(bug("[UserGroup] %s()\n", __func__));

    if (error = suser(pc->pc_ucred /*, &p->p_acflag */)) {

    } else if (ngrp > NGROUPS) {
        error = EINVAL;
    } else if (groups == NULL || ((SIPTR)groups & 1) != 0) {
        error = EFAULT;
    } else {
        pc->pc_ucred = crcopy(pc->pc_ucred);
        pc->pc_ucred->cr_ngroups = ngrp;
        for (gp = pc->pc_ucred->cr_groups, lp = groups; ngrp--; )
            *gp++ = *lp++;
        return 0;
    }

    ug_SetErrno((struct Library *)UserGroupBase, error);
    return -1;

    AROS_LIBFUNC_EXIT
}


/****** usergroup.library/MU2UG **********************************************

    NAME
        MU2ID - macro converting MultiUser id to usergroup id
        ID2MU - macro converting usergroup id to MultiUser id

    SYNOPSIS
        ug_id = MU2ID(mu_id)

        mu_id = ID2MU(ug_id)

    FUNCTION
        These macros are used to convert between different user ID
        formats.  The mu_id is in the format used by MultiUser filesystem
        and multiuser.library.  The ug_id is the format used by Unix and
        usergroup.library.

        Most of the id values are identical in usergroup.library and
        multiuser.library. However, these two exceptions have values as
        follows:

                    usergroup.library  multiuser.library
        super-user          0              65535
        nobody             -2                  0

    INPUTS AND RESULTS
        mu_id    - user ID in MultiUser format.
        ug_id    - user ID in usergroup format.

    BUGS
        The usergroup id values that won't fit into UWORD are truncated.

    SEE ALSO
******************************************************************************
*/

#if 0
ASM uid_t R_ug_mu2id(REG(d0) UWORD id)
{
    if (id == 0) {
        return -2;
    } else if (id == 65535) {
        return 0;
    } else {
        return id;
    }
}

ASM UWORD R_ug_id2mu(REG(d0) uid_t id)
{
    if (id == 0) {
        return 65535;
    } else if (id == -2) {
        return 0;
    } else {
        return id;
    }
}
#endif

/*****************************************************************************

    NAME */
        AROS_LH1I(mode_t, umask,

/*  SYNOPSIS */
        AROS_LHA(mode_t, newmask, D0),

/* LOCATION */
        struct Library *, UserGroupBase, 32, Usergroup)

/*  FUNCTION
        The umask() routine sets the process's file mode creation mask to
        numask and returns the previous value of the mask.  The 9 low-order
        access permission bits of numask are used by Unix-compatible
        filesystems, for examble by NFS, to turn off corresponding bits
        requested in file mode.  This clearing allows each user to restrict
        the default access to his files.

        The default mask value is 022 (write access for owner only).  Child
        processes should inherit the mask of the calling process.

    RESULT
        The previous value of the file mode mask is returned by the call.

    ERRORS
        The umask() function is always successful.

    SEE ALSO
        getumask()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct proc *p = procfind((pid_t)NULL);

    mode_t oldmask = p->p_umask;

    D(bug("[UserGroup] %s()\n", __func__));

    p->p_umask = newmask;

    return oldmask ;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH0I(mode_t, getumask,

/*  SYNOPSIS */

/* LOCATION */
                struct Library *, UserGroupBase, 33, Usergroup)

/*  FUNCTION
        The getumask() routine sets the process's file mode creation mask to
        numask and returns the previous value of the mask.  The 9 low-order
        access permission bits of numask are used by Unix-compatible
        filesystems, for examble by NFS, to turn off corresponding bits
        requested in file mode.

    RESULT
        The value of the file mode mask is returned by the call.

    ERRORS
        The getumask() function is always successful.

    SEE ALSO
        umask()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct proc *p = procfind((pid_t)NULL);

    D(bug("[UserGroup] %s()\n", __func__));

    return p->p_umask;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1I(struct UserGroupCredentials *, getcredentials,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, A0),

/* LOCATION */
        struct Library *, UserGroupBase, 43, Usergroup)

/*  FUNCTION
        The function getcredentials() returns all credentials of the given
        task.  The credentials include real and effective user and group IDs,
        umask, login name and session ID.  If the task pointer is NULL, the
        credentials of current task are returned.

    RESULT
        A getcredentials() function returns a valid pointer to structure
        UserGroupCredentials on success and a null pointer if an error
        occurs.

    ERRORS
        [EINVAL]    An illegal task pointer was specified.

    BUGS
        This function leave its result in an internal static object and
        return a pointer to that object. Subsequent calls to this function
        will modify the same object.

    SEE ALSO

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    register struct proc *p = procfind((pid_t)task);

    D(bug("[UserGroup] %s()\n", __func__));

    if (p != NULL) {
        register struct UserGroupCredentials *c = creds;

        c->cr_ruid  = p->p_cred->p_ruid;
        c->cr_rgid  = p->p_cred->p_rgid;
        c->cr_umask = p->p_umask;
        c->cr_euid  = p->p_cred->p_euid;
        c->cr_ngroups = p->p_cred->p_ngroups;
        c->cr_session = (struct Task *)p->p_session->s_leader;
        memcpy(c->cr_groups, p->p_cred->p_groups, sizeof(c->cr_groups));
        memcpy(c->cr_login, p->p_session->s_login, sizeof(c->cr_login));

        return c;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}
