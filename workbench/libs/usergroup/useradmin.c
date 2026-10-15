/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: usergroup.library - ug_UserAdminA(): account administration

          One call for the account tools (SYS:System/Accounts) and the
          password module of pam.library, whichever database the system
          uses:

          - security.library resident and configured: secUserAdminA() (and
            secAuthControlA() to lock), which only root may use; the
            library writes its own password file.
          - otherwise (also when security.library is in ROM but not set
            up, which is a single-user system): the netinfo.device database
            (AmiTCP:db/passwd, ENV:SYS/userdb), changed with CMD_WRITE /
            NI_REMOVE and saved with CMD_UPDATE. Passwords are crypt()
            hashes; a locked account has '!' before its hash, which no
            password matches.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/security.h>
#include <libraries/security.h>
#include <utility/tagitem.h>
#include <exec/errors.h>
#include <sys/errno.h>
#include <string.h>

#include "base.h"
#include "usergroup_security.h"

#include <proto/usergroup.h>

#define FIELDMAX    256

/* A field may not break the "a|b|c" line format (paths hold ':') */
static BOOL FieldOK(const char *s, BOOL empty)
{
    ULONG n = 0;

    if (!s)
        return empty;
    for (; s[n]; n++)
    {
        UBYTE c = (UBYTE)s[n];
        if (c < 0x20 || c == 0x7f || c == '|' || (c >= 0x80 && c < 0xa0))
            return FALSE;
    }
    return n < FIELDMAX && (empty || n > 0);
}

/* A user name: a field without ':' or spaces either */
static BOOL NameOK(const char *s)
{
    return FieldOK(s, FALSE) && !strchr(s, ':') && !strchr(s, ' ');
}

/* ------------------------------------------------------------------------ */
/* security.library                                                         */

static int SecErrno(ULONG r)
{
    switch (r)
    {
    case secUAERR_OK:           return 0;
    case secUAERR_NOTROOT:      return EPERM;
    case secUAERR_NOTFOUND:     return ENOENT;
    case secUAERR_EXISTS:       return EEXIST;
    case secUAERR_PROTECTED:    return EPERM;
    case secUAERR_WRITE:        return EIO;
    case secUAERR_UNAVAILABLE:  return ENOSYS;
    default:                    return EINVAL;
    }
}

/* usergroup ids are POSIX ones: root is 0 (usergroup_security.c) */
static IPTR ToSecId(IPTR id, UWORD root)
{
    return (id == 0) ? root : id;
}

static int SecAdmin(struct Library *ugBase, struct TagItem *tags)
{
    struct Library *secBase = ((struct UserGroupBase *)ugBase)->ugSecBase;
    struct TagItem *lock;
    ULONG op = GetTagData(UGA_Operation, 0, tags), sop;
    const char *name = (const char *)GetTagData(UGA_Name, 0, tags);
    int err = 0;

    if (!secBase || secBase->lib_Version < 45 || (secBase->lib_Version == 45 && secBase->lib_Revision < 14))
        return ENOSYS;

    switch (op)
    {
    case UGOP_ADD:      sop = secUA_ADD;      break;
    case UGOP_MODIFY:   sop = secUA_MODIFY;   break;
    case UGOP_DELETE:   sop = secUA_DELETE;   break;
    case UGOP_PASSWORD: sop = secUA_PASSWORD; break;
    default:            return EINVAL;
    }

    {
        struct TagItem *t = FindTagItem(UGA_UID, tags), *g = FindTagItem(UGA_GID, tags);
        const char *nn = (const char *)GetTagData(UGA_NewName, 0, tags);
        const char *rn = (const char *)GetTagData(UGA_RealName, 0, tags);
        const char *hd = (const char *)GetTagData(UGA_HomeDir, 0, tags);
        const char *sh = (const char *)GetTagData(UGA_Shell, 0, tags);
        const char *pw = (const char *)GetTagData(UGA_Password, 0, tags);
        struct TagItem st[] =
        {
            { secT_UAOperation,                     sop                       },
            { secT_UserID,                          (IPTR)name                },
            { nn ? secT_NewUserID : TAG_IGNORE,     (IPTR)nn                  },
            { t ? secT_UID : TAG_IGNORE,            t ? ToSecId(t->ti_Data, secROOT_UID) : 0 },
            { g ? secT_GID : TAG_IGNORE,            g ? ToSecId(g->ti_Data, secROOT_GID) : 0 },
            { rn ? secT_UserName : TAG_IGNORE,      (IPTR)rn                  },
            { hd ? secT_HomeDir : TAG_IGNORE,       (IPTR)hd                  },
            { sh ? secT_Shell : TAG_IGNORE,         (IPTR)sh                  },
            { pw ? secT_Password : TAG_IGNORE,      (IPTR)pw                  },
            { TAG_DONE,                             0                         }
        };

        /* a lock-only change needs no secUserAdminA() call */
        lock = FindTagItem(UGA_Lock, tags);
        if (!(op == UGOP_MODIFY && lock && !nn && !t && !g && !rn && !hd && !sh && !pw))
            err = SecErrno(secUserAdminA(st));
    }
    if (!err && op == UGOP_MODIFY && lock)
    {
        const char *acct = (const char *)GetTagData(UGA_NewName, (IPTR)name, tags);
        struct TagItem lt[] =
        {
            { lock->ti_Data ? secT_LockAccount : secT_UnlockAccount, (IPTR)acct },
            { TAG_DONE, 0 }
        };
        if (!secAuthControlA(lt))
            err = EPERM;
    }
    return err;
}

/* ------------------------------------------------------------------------ */
/* netinfo.device                                                           */

/* A copy of one passwd entry */
struct PwCopy
{
    LONG    uid, gid;
    char    name[FIELDMAX], passwd[FIELDMAX], gecos[FIELDMAX], dir[FIELDMAX], shell[FIELDMAX];
};

static void Copy(char *to, const char *from)
{
    strncpy(to, from ? from : "", FIELDMAX - 1);
    to[FIELDMAX - 1] = '\0';
}

/* Look an entry up by name (into *out) or uid; 0, ENOENT or another errno */
static int NiGet(struct Library *ugBase, ULONG unit, const char *name, LONG id, struct PwCopy *out)
{
    struct NetInfoReq *nreq;
    struct NetInfoPasswd *pw;

    if (!(nreq = ug_OpenUnit(ugBase, unit)))
        return ENXIO;
    pw = (struct NetInfoPasswd *)nreq->io_Data;
    if (name)
    {
        pw->pw_name = (UBYTE *)name;
        nreq->io_Command = NI_GETBYNAME;
    }
    else
    {
        pw->pw_uid = id;     /* gr_gid of struct NetInfoGroup is at the same place */
        nreq->io_Command = NI_GETBYID;
    }
    if (ug_DoIO(nreq) != 0)
        return (nreq->io_Error == NIERR_NOTFOUND) ? ENOENT : EIO;
    if (out && unit == NETINFO_PASSWD_UNIT)
    {
        out->uid = pw->pw_uid;
        out->gid = pw->pw_gid;
        Copy(out->name, pw->pw_name);
        Copy(out->passwd, pw->pw_passwd);
        Copy(out->gecos, pw->pw_gecos);
        Copy(out->dir, pw->pw_dir);
        Copy(out->shell, pw->pw_shell);
    }
    return 0;
}

static int NiPut(struct Library *ugBase, struct PwCopy *e)
{
    struct NetInfoReq *nreq;
    struct NetInfoPasswd pw;
    APTR buffer;
    ULONG len;
    BYTE r;

    if (!(nreq = ug_OpenUnit(ugBase, NETINFO_PASSWD_UNIT)))
        return ENXIO;
    pw.pw_name = (UBYTE *)e->name;
    pw.pw_passwd = (UBYTE *)e->passwd;
    pw.pw_uid = e->uid;
    pw.pw_gid = e->gid;
    pw.pw_gecos = (UBYTE *)e->gecos;
    pw.pw_dir = (UBYTE *)e->dir;
    pw.pw_shell = (UBYTE *)e->shell;
    buffer = nreq->io_Data;
    len = nreq->io_Length;
    nreq->io_Data = &pw;
    nreq->io_Length = sizeof(pw);
    nreq->io_Command = CMD_WRITE;
    r = ug_DoIO(nreq);
    nreq->io_Data = buffer;
    nreq->io_Length = len;
    return r ? EIO : 0;
}

static int NiRemove(struct Library *ugBase, const char *name)
{
    struct NetInfoReq *nreq;
    struct NetInfoPasswd pw;
    APTR buffer;
    ULONG len;
    BYTE r;

    if (!(nreq = ug_OpenUnit(ugBase, NETINFO_PASSWD_UNIT)))
        return ENXIO;
    memset(&pw, 0, sizeof(pw));
    pw.pw_name = (UBYTE *)name;
    buffer = nreq->io_Data;
    len = nreq->io_Length;
    nreq->io_Data = &pw;
    nreq->io_Length = sizeof(pw);
    nreq->io_Command = NI_REMOVE;
    r = ug_DoIO(nreq);
    nreq->io_Data = buffer;
    nreq->io_Length = len;
    if (r)
        return (nreq->io_Error == NIERR_NOTFOUND) ? ENOENT : (nreq->io_Error == IOERR_NOCMD ? ENOSYS : EIO);
    return 0;
}

static int NiUpdate(struct Library *ugBase)
{
    struct NetInfoReq *nreq;

    if (!(nreq = ug_OpenUnit(ugBase, NETINFO_PASSWD_UNIT)))
        return ENXIO;
    nreq->io_Command = CMD_UPDATE;
    return ug_DoIO(nreq) ? EIO : 0;
}

/* The crypt() hash of a password, "" for none; FALSE if it cannot be made */
static BOOL Hash(struct Library *ugBase, const char *pwd, char *out)
{
    struct Library *UserGroupBase = ugBase;
    char salt[16];
    char *h;

    out[0] = '\0';
    if (!pwd || !pwd[0])
        return TRUE;
    if (!ug_GetSalt(NULL, salt, sizeof(salt)) || !(h = crypt(pwd, salt)))
        return FALSE;
    Copy(out, h);
    return TRUE;
}

static int NiAdmin(struct Library *ugBase, struct TagItem *tags)
{
    ULONG op = GetTagData(UGA_Operation, 0, tags);
    const char *name = (const char *)GetTagData(UGA_Name, 0, tags);
    const char *nn = (const char *)GetTagData(UGA_NewName, 0, tags);
    const char *pw = (const char *)GetTagData(UGA_Password, 0, tags);
    struct TagItem *t = FindTagItem(UGA_UID, tags), *g = FindTagItem(UGA_GID, tags), *lock = FindTagItem(UGA_Lock, tags);
    struct PwCopy *e, *other;
    int err;

    if (!NameOK(name))
        return EINVAL;
    if (!(e = AllocVec(2 * sizeof(struct PwCopy), MEMF_CLEAR)))
        return ENOMEM;
    other = e + 1;

    switch (op)
    {
    case UGOP_ADD:
        err = EINVAL;
        if (!t || !g || !FieldOK((char *)GetTagData(UGA_RealName, 0, tags), TRUE) ||
            !FieldOK((char *)GetTagData(UGA_HomeDir, 0, tags), TRUE) || !FieldOK((char *)GetTagData(UGA_Shell, 0, tags), TRUE))
            break;
        if ((err = NiGet(ugBase, NETINFO_PASSWD_UNIT, name, 0, NULL)) != ENOENT ||
            (err = NiGet(ugBase, NETINFO_PASSWD_UNIT, NULL, (LONG)t->ti_Data, NULL)) != ENOENT)
        {
            err = err ? err : EEXIST;
            break;
        }
        if ((err = NiGet(ugBase, NETINFO_GROUP_UNIT, NULL, (LONG)g->ti_Data, NULL)))
        {
            err = (err == ENOENT) ? EINVAL : err;
            break;
        }
        Copy(e->name, name);
        e->uid = (LONG)t->ti_Data;
        e->gid = (LONG)g->ti_Data;
        Copy(e->gecos, (char *)GetTagData(UGA_RealName, 0, tags));
        Copy(e->dir, (char *)GetTagData(UGA_HomeDir, 0, tags));
        Copy(e->shell, (char *)GetTagData(UGA_Shell, 0, tags));
        err = Hash(ugBase, pw, e->passwd) ? NiPut(ugBase, e) : EINVAL;
        break;

    case UGOP_MODIFY:
    case UGOP_PASSWORD:
        if ((err = NiGet(ugBase, NETINFO_PASSWD_UNIT, name, 0, e)))
            break;
        err = EINVAL;
        if (op == UGOP_PASSWORD)
        {
            if (!pw || !Hash(ugBase, pw, e->passwd))
                break;
            err = NiPut(ugBase, e);
            break;
        }
        if ((nn && !NameOK(nn)) ||
            !FieldOK((char *)GetTagData(UGA_RealName, 0, tags), TRUE) ||
            !FieldOK((char *)GetTagData(UGA_HomeDir, 0, tags), TRUE) || !FieldOK((char *)GetTagData(UGA_Shell, 0, tags), TRUE))
            break;
        /* the super user keeps its name and number */
        if (e->uid == 0 && ((nn && strcmp(nn, name)) || (t && t->ti_Data != 0)))
        {
            err = EPERM;
            break;
        }
        if (nn && strcmp(nn, name) && NiGet(ugBase, NETINFO_PASSWD_UNIT, nn, 0, NULL) != ENOENT)
        {
            err = EEXIST;
            break;
        }
        if (t && (LONG)t->ti_Data != e->uid && NiGet(ugBase, NETINFO_PASSWD_UNIT, NULL, (LONG)t->ti_Data, NULL) != ENOENT)
        {
            err = EEXIST;
            break;
        }
        if (g && NiGet(ugBase, NETINFO_GROUP_UNIT, NULL, (LONG)g->ti_Data, NULL))
            break;
        if (t)
            e->uid = (LONG)t->ti_Data;
        if (g)
            e->gid = (LONG)g->ti_Data;
        if (FindTagItem(UGA_RealName, tags))
            Copy(e->gecos, (char *)GetTagData(UGA_RealName, 0, tags));
        if (FindTagItem(UGA_HomeDir, tags))
            Copy(e->dir, (char *)GetTagData(UGA_HomeDir, 0, tags));
        if (FindTagItem(UGA_Shell, tags))
            Copy(e->shell, (char *)GetTagData(UGA_Shell, 0, tags));
        if (pw && !Hash(ugBase, pw, e->passwd))
            break;
        if (lock)
        {
            BOOL locked = (e->passwd[0] == '!');
            if (lock->ti_Data && !locked)
            {
                memmove(e->passwd + 1, e->passwd, FIELDMAX - 2);
                e->passwd[0] = '!';
                e->passwd[FIELDMAX - 1] = '\0';
            }
            else if (!lock->ti_Data && locked)
                memmove(e->passwd, e->passwd + 1, FIELDMAX - 1);
        }
        err = 0;
        if (nn && strcmp(nn, name))
        {
            /* write the new entry first: a failure then leaves the old one */
            Copy(e->name, nn);
            if (!(err = NiPut(ugBase, e)))
                err = NiRemove(ugBase, name);
        }
        else
            err = NiPut(ugBase, e);
        break;

    case UGOP_DELETE:
        if ((err = NiGet(ugBase, NETINFO_PASSWD_UNIT, name, 0, e)))
            break;
        if (e->uid == 0)
        {
            err = EPERM;
            break;
        }
        err = NiRemove(ugBase, name);
        break;

    default:
        err = EINVAL;
        break;
    }

    if (!err)
        err = NiUpdate(ugBase);
    memset(e, 0, 2 * sizeof(struct PwCopy));
    FreeVec(e);
    return err;
}

/****** usergroup.library/ug_UserAdminA ************************************

    NAME
        ug_UserAdminA - add, change or remove an account

    SYNOPSIS
        error = ug_UserAdminA(tags)
          D0                   A0

        int ug_UserAdminA(struct TagItem *);

    FUNCTION
        Changes the user database of the system. With security.library the
        change is made by it (only root may do it); otherwise in the
        netinfo.device password database. The change is saved at once.

    INPUTS
        Ids are those usergroup.library reports (the super user is 0, also
        with security.library).

        tags - UGA_Operation (UGOP_ADD, UGOP_MODIFY, UGOP_DELETE,
               UGOP_PASSWORD) and UGA_Name are required. UGOP_ADD needs
               UGA_UID and UGA_GID; UGA_RealName, UGA_HomeDir, UGA_Shell
               and UGA_Password are optional. UGOP_MODIFY changes the
               fields given, UGA_NewName renames, UGA_Lock (TRUE/FALSE)
               locks or unlocks the account. UGOP_PASSWORD sets
               UGA_Password ("" for none).

    RESULT
        0 on success, -1 on error with errno set: EPERM (not allowed, or
        the super user's name or number), ENOENT (no such user), EEXIST
        (name or uid taken), EINVAL (bad field, unknown group), EIO (the
        database could not be written), ENOSYS (no database to change).

    NOTES
        Fields may not contain '|' or control characters; user names
        neither ':' nor spaces. With security.library an account with a password must
        get a new one when it is renamed.

    SEE ALSO
        getpwent(), crypt(), security.library/secUserAdminA()

****************************************************************************
*/

AROS_LH1(int, ug_UserAdminA,
         AROS_LHA(struct TagItem *, tags, A0),
         struct UserGroupBase *, UserGroupBase, 44, Usergroup)
{
    AROS_LIBFUNC_INIT

    struct Library *ugBase = (struct Library *)UserGroupBase;
    int err;

    if (!tags || !GetTagData(UGA_Name, 0, tags))
    {
        ug_SetErrno(ugBase, EINVAL);
        return -1;
    }

    if (ugSecDbActive(ugBase))
        err = SecAdmin(ugBase, tags);
    else
    {
        ObtainSemaphore(&UserGroupBase->ni_lock);
        err = NiAdmin(ugBase, tags);
        ReleaseSemaphore(&UserGroupBase->ni_lock);
    }
    if (err)
    {
        ug_SetErrno(ugBase, err);
        return -1;
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
