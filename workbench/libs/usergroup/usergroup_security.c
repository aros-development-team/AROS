/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: usergroup.library on top of security.library, see usergroup_security.h
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <exec/resident.h>
#include <dos/dos.h>
#include <libraries/security.h>
#include <proto/exec.h>
#include <proto/security.h>
#include <sys/stat.h>
#include <string.h>

#include "base.h"
#include "usergroup_security.h"

#define UGB(b)   ((struct UserGroupBase *)(b))
#define secBase  (UGB(ugBase)->ugSecBase)

/*
 * usergroup ids: root is 0, nobody is -2; security.library ids: secROOT_UID
 * and secNOBODY_UID. Map symbolically so that a change of the constants in
 * <libraries/security.h> does not break this.
 */
static inline uid_t sec2ug_uid(UWORD u)
{
    if (u == secNOBODY_UID) return (uid_t)-2;
    if (u == secROOT_UID)   return 0;
    return u;
}
static inline gid_t sec2ug_gid(UWORD g)
{
    if (g == secNOBODY_GID) return (gid_t)-2;
    if (g == secROOT_GID)   return 0;
    return g;
}
static inline int ug2sec_uid(uid_t u)      /* -1 (NOID) passes through */
{
    if (u == (uid_t)-1) return -1;
    if (u == (uid_t)-2) return secNOBODY_UID;
    if (u == 0)         return secROOT_UID;
    return (UWORD)u;
}
static inline int ug2sec_gid(gid_t g)
{
    if (g == (gid_t)-1) return -1;
    if (g == (gid_t)-2) return secNOBODY_GID;
    if (g == 0)         return secROOT_GID;
    return (UWORD)g;
}

/*
 * The library is RTF_AFTERDOS; usergroup.library may be opened before it is
 * initialised, so keep trying while it is resident, give up when it is not.
 */
BOOL ugSecActive(struct Library *ugBase)
{
    if (secBase == NULL && !UGB(ugBase)->ugSecChecked)
    {
        if (FindResident(SECURITYNAME))
        {
            secBase = OpenLibrary(SECURITYNAME, 0);
            D(if (secBase) bug("[UserGroup] security.library @ %p\n", secBase);)
        }
        else
            UGB(ugBase)->ugSecChecked = TRUE;
    }
    return secBase != NULL;
}

void ugSecCleanup(struct Library *ugBase)
{
    if (secBase != NULL)
    {
        if (UGB(ugBase)->ugSecUI)  secFreeUserInfo(UGB(ugBase)->ugSecUI);
        if (UGB(ugBase)->ugSecUI2) secFreeUserInfo(UGB(ugBase)->ugSecUI2);
        if (UGB(ugBase)->ugSecGI)  secFreeGroupInfo(UGB(ugBase)->ugSecGI);
        UGB(ugBase)->ugSecUI = UGB(ugBase)->ugSecUI2 = NULL;
        UGB(ugBase)->ugSecGI = NULL;
        CloseLibrary(secBase);
        secBase = NULL;
    }
}

/* ---- credentials ------------------------------------------------------- */

uid_t ugSecGetUid(struct Library *ugBase, BOOL effective)
{
    if (effective)
        return sec2ug_uid(secGetTaskOwner(NULL) >> 16);
    return sec2ug_uid(secgetuid());
}

gid_t ugSecGetGid(struct Library *ugBase, BOOL effective)
{
    if (effective)
        return sec2ug_gid(secGetTaskOwner(NULL) & secMASK_GID);
    return sec2ug_gid(secgetgid());
}

int ugSecGetGroups(struct Library *ugBase, int ngroups, gid_t *groups)
{
    struct secExtOwner *owner = secGetTaskExtOwner(FindTask(NULL));
    int n, i, error = 0;

    if (owner == NULL)
    {
        ug_SetErrno(ugBase, ENOENT);
        return -1;
    }
    n = owner->NumSecGroups + 1;
    if (n > NGROUPS)
        n = NGROUPS;

    if (ngroups == 0)
        ;                                   /* just report the count */
    else if (ngroups < n)
        error = EINVAL;
    else if (groups == NULL || ((SIPTR)groups & 1) != 0)
        error = EFAULT;
    else
    {
        UWORD *sg = secSecGroups(owner);
        groups[0] = sec2ug_gid(owner->gid);
        for (i = 1; i < n; i++)
            groups[i] = sec2ug_gid(sg[i - 1]);
    }
    secFreeExtOwner(owner);

    if (error)
    {
        ug_SetErrno(ugBase, error);
        return -1;
    }
    return n;
}

int ugSecSetReUid(struct Library *ugBase, uid_t ruid, uid_t euid)
{
    if (secsetreuid(ug2sec_uid(ruid), ug2sec_uid(euid)) != 0)
    {
        ug_SetErrno(ugBase, EPERM);
        return -1;
    }
    return 0;
}

int ugSecSetUid(struct Library *ugBase, uid_t uid)
{
    if (secsetuid((UWORD)ug2sec_uid(uid)) != 0)
    {
        ug_SetErrno(ugBase, EPERM);
        return -1;
    }
    return 0;
}

int ugSecSetReGid(struct Library *ugBase, gid_t rgid, gid_t egid)
{
    int rc = 0;

    /* security.library keeps one real/effective gid pair per task through
       secsetgid()/secsetegid(); apply whichever was requested */
    if (rgid != (gid_t)-1 && secsetgid((UWORD)ug2sec_gid(rgid)) != 0)
        rc = -1;
    if (rc == 0 && egid != (gid_t)-1 && secsetegid((UWORD)ug2sec_gid(egid)) != 0)
        rc = -1;
    if (rc != 0)
        ug_SetErrno(ugBase, EPERM);
    return rc;
}

int ugSecSetGid(struct Library *ugBase, gid_t gid)
{
    if (secsetgid((UWORD)ug2sec_gid(gid)) != 0)
    {
        ug_SetErrno(ugBase, EPERM);
        return -1;
    }
    return 0;
}

/* Group membership is defined by the group database, not per process */
int ugSecSetGroups(struct Library *ugBase, int ngrp, const gid_t *groups)
{
    ug_SetErrno(ugBase, EPERM);
    return -1;
}

/* ---- umask <-> default protection -------------------------------------- */

/* Owner bits are active low (set = denied), group/other bits active high */
static mode_t Prot2Umask(ULONG prot)
{
    mode_t m = 0;

    if (prot & FIBF_READ)           m |= S_IRUSR;
    if (prot & FIBF_WRITE)          m |= S_IWUSR;
    if (prot & FIBF_EXECUTE)        m |= S_IXUSR;
    if (!(prot & FIBF_GRP_READ))    m |= S_IRGRP;
    if (!(prot & FIBF_GRP_WRITE))   m |= S_IWGRP;
    if (!(prot & FIBF_GRP_EXECUTE)) m |= S_IXGRP;
    if (!(prot & FIBF_OTR_READ))    m |= S_IROTH;
    if (!(prot & FIBF_OTR_WRITE))   m |= S_IWOTH;
    if (!(prot & FIBF_OTR_EXECUTE)) m |= S_IXOTH;
    return m;
}

static ULONG Umask2Prot(mode_t m)
{
    ULONG prot = 0;

    if (m & S_IRUSR)    prot |= FIBF_READ;
    if (m & S_IWUSR)    prot |= FIBF_WRITE | FIBF_DELETE;
    if (m & S_IXUSR)    prot |= FIBF_EXECUTE;
    if (!(m & S_IRGRP)) prot |= FIBF_GRP_READ;
    if (!(m & S_IWGRP)) prot |= FIBF_GRP_WRITE | FIBF_GRP_DELETE;
    if (!(m & S_IXGRP)) prot |= FIBF_GRP_EXECUTE;
    if (!(m & S_IROTH)) prot |= FIBF_OTR_READ;
    if (!(m & S_IWOTH)) prot |= FIBF_OTR_WRITE | FIBF_OTR_DELETE;
    if (!(m & S_IXOTH)) prot |= FIBF_OTR_EXECUTE;
    return prot;
}

mode_t ugSecUmask(struct Library *ugBase, mode_t newmask)
{
    mode_t old = Prot2Umask(secGetDefProtection(NULL));
    struct TagItem tags[] = { { secT_DefProtection, Umask2Prot(newmask & 0777) }, { TAG_DONE, 0 } };

    secSetDefProtectionA(tags);
    return old;
}

mode_t ugSecGetUmask(struct Library *ugBase)
{
    return Prot2Umask(secGetDefProtection(NULL));
}

/* ---- user database ----------------------------------------------------- */

static struct secUserInfo *UserInfo(struct Library *ugBase)
{
    if (UGB(ugBase)->ugSecUI == NULL)
        UGB(ugBase)->ugSecUI = secAllocUserInfo();
    return UGB(ugBase)->ugSecUI;
}

static struct secUserInfo *UserInfo2(struct Library *ugBase)
{
    if (UGB(ugBase)->ugSecUI2 == NULL)
        UGB(ugBase)->ugSecUI2 = secAllocUserInfo();
    return UGB(ugBase)->ugSecUI2;
}

static struct secGroupInfo *GroupInfo(struct Library *ugBase)
{
    if (UGB(ugBase)->ugSecGI == NULL)
        UGB(ugBase)->ugSecGI = secAllocGroupInfo();
    return UGB(ugBase)->ugSecGI;
}

/* Fill the passwd structure from a user info (strings point into the info) */
static struct passwd *MakePasswd(struct Library *ugBase, struct secUserInfo *ui)
{
    struct passwd *pw = &UGB(ugBase)->ugSecPw;

    pw->pw_name   = ui->UserID;
    pw->pw_uid    = sec2ug_uid(ui->uid);
    pw->pw_gid    = sec2ug_gid(ui->gid);
    pw->pw_dir    = ui->HomeDir;
    pw->pw_shell  = ui->Shell;
    pw->pw_passwd = "*";                  /* never disclosed, use secCheckPasswdA() */
    pw->pw_gecos  = ui->UserName;
    return pw;
}

struct passwd *ugSecGetPwNam(struct Library *ugBase, const char *name)
{
    struct secUserInfo *ui;

    if (name == NULL)
    {
        ug_SetErrno(ugBase, EFAULT);
        return NULL;
    }
    if ((ui = UserInfo(ugBase)) == NULL)
    {
        ug_SetErrno(ugBase, ENOMEM);
        return NULL;
    }
    strncpy(ui->UserID, name, secUSERIDSIZE - 1);
    ui->UserID[secUSERIDSIZE - 1] = '\0';
    if (secGetUserInfo(ui, secKeyType_UserID) == NULL)
    {
        ug_SetErrno(ugBase, ENOENT);
        return NULL;
    }
    return MakePasswd(ugBase, ui);
}

struct passwd *ugSecGetPwUid(struct Library *ugBase, uid_t uid)
{
    struct secUserInfo *ui;

    if ((ui = UserInfo(ugBase)) == NULL)
    {
        ug_SetErrno(ugBase, ENOMEM);
        return NULL;
    }
    ui->uid = (UWORD)ug2sec_uid(uid);
    if (secGetUserInfo(ui, secKeyType_uid) == NULL)
    {
        ug_SetErrno(ugBase, ENOENT);
        return NULL;
    }
    return MakePasswd(ugBase, ui);
}

void ugSecSetPwEnt(struct Library *ugBase)
{
    UGB(ugBase)->ugSecPwIter = FALSE;
}

struct passwd *ugSecGetPwEnt(struct Library *ugBase)
{
    struct secUserInfo *ui;
    ULONG key = UGB(ugBase)->ugSecPwIter ? secKeyType_Next : secKeyType_First;

    if ((ui = UserInfo(ugBase)) == NULL)
    {
        ug_SetErrno(ugBase, ENOMEM);
        return NULL;
    }
    UGB(ugBase)->ugSecPwIter = TRUE;
    if (secGetUserInfo(ui, key) == NULL)
        return NULL;
    return MakePasswd(ugBase, ui);
}

void ugSecEndPwEnt(struct Library *ugBase)
{
    UGB(ugBase)->ugSecPwIter = FALSE;
}

/* ---- group database ---------------------------------------------------- */

/* Members: users whose primary or secondary group is gid */
static void CollectMembers(struct Library *ugBase, UWORD gid)
{
    struct UserGroupBase *b = UGB(ugBase);
    struct secUserInfo *ui = UserInfo2(ugBase);
    ULONG key = secKeyType_First;
    int n = 0;

    while (ui != NULL && n < NGROUPS && secGetUserInfo(ui, key) != NULL)
    {
        BOOL member = (ui->gid == gid);
        int i;

        for (i = 0; !member && i < ui->NumSecGroups; i++)
            if (ui->SecGroups[i] == gid)
                member = TRUE;
        if (member)
        {
            strncpy(b->ugSecGrMemBuf[n], ui->UserID, secUSERIDSIZE - 1);
            b->ugSecGrMemBuf[n][secUSERIDSIZE - 1] = '\0';
            b->ugSecGrMem[n] = b->ugSecGrMemBuf[n];
            n++;
        }
        key = secKeyType_Next;
    }
    b->ugSecGrMem[n] = NULL;
}

static struct group *MakeGroup(struct Library *ugBase, struct secGroupInfo *gi)
{
    struct group *gr = &UGB(ugBase)->ugSecGr;

    gr->gr_name   = gi->GroupID;
    gr->gr_gid    = sec2ug_gid(gi->gid);
    gr->gr_passwd = "*";
    CollectMembers(ugBase, gi->gid);
    gr->gr_mem    = UGB(ugBase)->ugSecGrMem;
    return gr;
}

struct group *ugSecGetGrNam(struct Library *ugBase, const char *name)
{
    struct secGroupInfo *gi;

    if (name == NULL)
    {
        ug_SetErrno(ugBase, EFAULT);
        return NULL;
    }
    if ((gi = GroupInfo(ugBase)) == NULL)
    {
        ug_SetErrno(ugBase, ENOMEM);
        return NULL;
    }
    strncpy(gi->GroupID, name, secGROUPIDSIZE - 1);
    gi->GroupID[secGROUPIDSIZE - 1] = '\0';
    if (secGetGroupInfo(gi, secKeyType_GroupID) == NULL)
    {
        ug_SetErrno(ugBase, ENOENT);
        return NULL;
    }
    return MakeGroup(ugBase, gi);
}

struct group *ugSecGetGrGid(struct Library *ugBase, gid_t gid)
{
    struct secGroupInfo *gi;

    if ((gi = GroupInfo(ugBase)) == NULL)
    {
        ug_SetErrno(ugBase, ENOMEM);
        return NULL;
    }
    gi->gid = (UWORD)ug2sec_gid(gid);
    if (secGetGroupInfo(gi, secKeyType_gid) == NULL)
    {
        ug_SetErrno(ugBase, ENOENT);
        return NULL;
    }
    return MakeGroup(ugBase, gi);
}

void ugSecSetGrEnt(struct Library *ugBase)
{
    UGB(ugBase)->ugSecGrIter = FALSE;
}

struct group *ugSecGetGrEnt(struct Library *ugBase)
{
    struct secGroupInfo *gi;
    ULONG key = UGB(ugBase)->ugSecGrIter ? secKeyType_Next : secKeyType_First;

    if ((gi = GroupInfo(ugBase)) == NULL)
    {
        ug_SetErrno(ugBase, ENOMEM);
        return NULL;
    }
    UGB(ugBase)->ugSecGrIter = TRUE;
    if (secGetGroupInfo(gi, key) == NULL)
        return NULL;
    return MakeGroup(ugBase, gi);
}

void ugSecEndGrEnt(struct Library *ugBase)
{
    UGB(ugBase)->ugSecGrIter = FALSE;
}

/* ---- login name / credentials ------------------------------------------ */

/* The login name is the user id of the task's (session) owner */
static void LoginName(struct Library *ugBase, struct Task *task, char *buffer, ULONG size)
{
    struct secUserInfo *ui = UserInfo2(ugBase);

    buffer[0] = '\0';
    if (ui == NULL)
        return;
    ui->uid = secGetTaskOwner(task) >> 16;
    if (secGetUserInfo(ui, secKeyType_uid) != NULL)
    {
        strncpy(buffer, ui->UserID, size - 1);
        buffer[size - 1] = '\0';
    }
}

char *ugSecGetLogin(struct Library *ugBase, char *buffer, ULONG size)
{
    LoginName(ugBase, NULL, buffer, size);
    return buffer;
}

struct UserGroupCredentials *ugSecGetCredentials(struct Library *ugBase, struct Task *task,
                                                 struct UserGroupCredentials *c)
{
    struct secExtOwner *owner;
    UWORD *sg;
    int i, n;

    if (task == NULL)
        task = FindTask(NULL);
    if ((owner = secGetTaskExtOwner(task)) == NULL)
    {
        ug_SetErrno(ugBase, EINVAL);
        return NULL;
    }

    c->cr_euid = sec2ug_uid(owner->uid);
    /* the real ids are only tracked for the calling task */
    c->cr_ruid = (task == FindTask(NULL)) ? sec2ug_uid(secgetuid()) : c->cr_euid;
    c->cr_rgid = (task == FindTask(NULL)) ? sec2ug_gid(secgetgid()) : sec2ug_gid(owner->gid);
    c->cr_umask = Prot2Umask(secGetDefProtection(task));

    n = owner->NumSecGroups + 1;
    if (n > NGROUPS)
        n = NGROUPS;
    c->cr_ngroups = n;
    c->cr_groups[0] = sec2ug_gid(owner->gid);
    sg = secSecGroups(owner);
    for (i = 1; i < n; i++)
        c->cr_groups[i] = sec2ug_gid(sg[i - 1]);
    secFreeExtOwner(owner);

    c->cr_session = task;
    LoginName(ugBase, task, c->cr_login, MAXLOGNAME);
    return c;
}
