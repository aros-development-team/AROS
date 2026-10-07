/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Accounts Server - account store (see acc_store.h).

          Envoy flags are derived, never stored: root has AdminAll and
          AdminGroups; a user with a password has NeedsPassword; a user who
          may change their password (PASSWDUIDLEVEL/PASSWDGIDLEVEL in
          Security.config, as secQueryUserA() decides it) has AdminPassword.
          New IDs are the largest in use below the root ID plus one. New
          passwords are stored as ACrypt(password, user ID as spelled),
          which is what security.library checks at the console and, for
          lower-case IDs, equal to Envoy's ECrypt (architecture §5.2).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>
#include <proto/pam.h>
#include <proto/alib.h>
#include <libraries/security.h>
#include <libraries/pam.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "acc_store.h"

#define secBase     (s->SecLib)
#define PamBase     (s->PamLib)

/* usergroup.library by vector (its stubs need a global base) */
static void ug_setpwent(struct Library *UserGroupBase) { AROS_LC0NR(void, setpwent, struct Library *, UserGroupBase, 21, Usergroup); }
static struct passwd *ug_getpwent(struct Library *UserGroupBase) { return AROS_LC0(struct passwd *, getpwent, struct Library *, UserGroupBase, 22, Usergroup); }
static void ug_endpwent(struct Library *UserGroupBase) { AROS_LC0NR(void, endpwent, struct Library *, UserGroupBase, 23, Usergroup); }
static void ug_setgrent(struct Library *UserGroupBase) { AROS_LC0NR(void, setgrent, struct Library *, UserGroupBase, 26, Usergroup); }
static struct group *ug_getgrent(struct Library *UserGroupBase) { return AROS_LC0(struct group *, getgrent, struct Library *, UserGroupBase, 27, Usergroup); }
static void ug_endgrent(struct Library *UserGroupBase) { AROS_LC0NR(void, endgrent, struct Library *, UserGroupBase, 28, Usergroup); }

/*------------------------------------------------------------------------*/
/* Helpers                                                                 */
/*------------------------------------------------------------------------*/

static int Lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

int AccStricmp(CONST_STRPTR a, CONST_STRPTR b)
{
    while (*a && Lower(*a) == Lower(*b))
        a++, b++;
    return Lower(*a) - Lower(*b);
}

void AccCopyName(STRPTR dst, CONST_STRPTR src, ULONG size)
{
    ULONG n = 0;
    while (src && src[n] && n < size - 1)
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}

/* MuFS ID from a usergroup.library ID */
static UWORD UG2MU(ULONG id)
{
    if (id == 0)
        return ACC_ROOT_UID;
    if (id == (ULONG)-2)
        return ACC_NOBODY_UID;
    return (UWORD)id;
}

/* The 11-character ACrypt hash from amiga.lib, as security.library computes it */
static void Hash(STRPTR buffer, CONST_STRPTR password, CONST_STRPTR salt)
{
    ACrypt((UBYTE *)buffer, (const UBYTE *)password, (const UBYTE *)salt);
}

static void Log(struct AccStore *s, const char *text)
{
    if (s->Verbose)
    {
        printf("AccountsServer: %s\n", text);
        fflush(stdout);
    }
}

static void FreeLists(struct AccStore *s)
{
    struct AccUser *u;
    struct AccGroup *g;

    while ((u = (struct AccUser *)RemHead((struct List *)&s->Users)))
        FreeVec(u);
    while ((g = (struct AccGroup *)RemHead((struct List *)&s->Groups)))
    {
        FreeVec(g->Members);
        FreeVec(g);
    }
    s->Loaded = FALSE;
}

/* Lists are kept in name order, case-insensitively (§8.5) */
static void InsertUser(struct AccStore *s, struct AccUser *u)
{
    struct AccUser *n;
    ForeachNode(&s->Users, n)
    {
        if (AccStricmp(u->Name, n->Name) < 0)
        {
            Insert((struct List *)&s->Users, (struct Node *)u, ((struct Node *)n)->ln_Pred);
            return;
        }
    }
    AddTail((struct List *)&s->Users, (struct Node *)u);
}

static void InsertGroup(struct AccStore *s, struct AccGroup *g)
{
    struct AccGroup *n;
    ForeachNode(&s->Groups, n)
    {
        if (AccStricmp(g->Name, n->Name) < 0)
        {
            Insert((struct List *)&s->Groups, (struct Node *)g, ((struct Node *)n)->ln_Pred);
            return;
        }
    }
    AddTail((struct List *)&s->Groups, (struct Node *)g);
}

static void DeriveFlags(struct AccStore *s, struct AccUser *u)
{
    u->Flags = 0;
    if (u->Uid == ACC_ROOT_UID)
        u->Flags |= UFLAGF_AdminAll | UFLAGF_AdminGroups;
    if (u->Hash[0])
        u->Flags |= UFLAGF_NeedsPassword;
    if (s->Writable)
    {
        if (u->Uid <= s->PasswdUidLevel || u->Gid <= s->PasswdGidLevel)
            u->Flags |= UFLAGF_AdminPassword;
    }
}

static BOOL AddMemberSorted(struct AccStore *s, struct AccGroup *g, UWORD uid)
{
    struct AccUser *u = StoreUserById(s, uid);
    ULONG i, pos;

    if (!u || StoreIsMember(g, uid))
        return FALSE;
    if (g->NumMembers >= g->MaxMembers)
    {
        ULONG nmax = g->MaxMembers ? g->MaxMembers * 2 : 8;
        UWORD *nm = AllocVec(nmax * sizeof(UWORD), MEMF_CLEAR | MEMF_PUBLIC);
        if (!nm)
            return FALSE;
        if (g->Members)
        {
            memcpy(nm, g->Members, g->NumMembers * sizeof(UWORD));
            FreeVec(g->Members);
        }
        g->Members = nm;
        g->MaxMembers = nmax;
    }
    for (pos = 0; pos < g->NumMembers; pos++)
    {
        struct AccUser *m = StoreUserById(s, g->Members[pos]);
        if (m && AccStricmp(u->Name, m->Name) < 0)
            break;
    }
    for (i = g->NumMembers; i > pos; i--)
        g->Members[i] = g->Members[i - 1];
    g->Members[pos] = uid;
    g->NumMembers++;
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* File backend                                                            */
/*------------------------------------------------------------------------*/

static UBYTE *ReadWholeFile(CONST_STRPTR path, ULONG *lenp)
{
    BPTR fh;
    UBYTE *data = NULL;
    LONG size;

    if (!(fh = Open(path, MODE_OLDFILE)))
        return NULL;
    if (Seek(fh, 0, OFFSET_END) >= 0 && (size = Seek(fh, 0, OFFSET_BEGINNING)) >= 0)
    {
        if ((data = AllocVec(size + 1, MEMF_CLEAR | MEMF_PUBLIC)))
        {
            if (size && Read(fh, data, size) != size)
            {
                FreeVec(data);
                data = NULL;
            }
            else
                *lenp = size;
        }
    }
    Close(fh);
    return data;
}

static BOOL FileDate(CONST_STRPTR path, struct DateStamp *ds)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    BOOL ok = FALSE;

    if (!(lock = Lock(path, SHARED_LOCK)))
        return FALSE;
    if ((fib = AllocDosObject(DOS_FIB, NULL)))
    {
        if (Examine(lock, fib))
        {
            *ds = fib->fib_Date;
            ok = TRUE;
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
    return ok;
}

/* Split a line at '|' into at most n parts; returns the number found */
static int SplitFields(STRPTR line, STRPTR *part, int n)
{
    int i = 0;
    part[i++] = line;
    while (*line && i < n)
    {
        if (*line == '|')
        {
            *line = '\0';
            part[i++] = line + 1;
        }
        line++;
    }
    return i;
}

static void ReadConfigLevels(struct AccStore *s)
{
    UBYTE *data;
    ULONG len = 0;

    s->PasswdUidLevel = ACC_NOBODY_UID;
    s->PasswdGidLevel = ACC_NOBODY_UID;
    if (!s->ConfigPath[0] || !(data = ReadWholeFile(s->ConfigPath, &len)))
        return;
    {
        STRPTR p = (STRPTR)data;
        while (p && *p)
        {
            STRPTR nl = strchr(p, '\n');
            if (nl)
                *nl = '\0';
            if (!strncmp(p, "PASSWDUIDLEVEL=", 15))
                s->PasswdUidLevel = (UWORD)strtoul(p + 15, NULL, 10);
            else if (!strncmp(p, "PASSWDGIDLEVEL=", 15))
                s->PasswdGidLevel = (UWORD)strtoul(p + 15, NULL, 10);
            p = nl ? nl + 1 : NULL;
        }
    }
    FreeVec(data);
}

static void LoadFiles(struct AccStore *s)
{
    UBYTE *data;
    ULONG len = 0;
    STRPTR p;

    FreeLists(s);
    ReadConfigLevels(s);

    /* UserID|Password|uid|gid|UserName|HomeDir|Shell */
    if ((data = ReadWholeFile(s->PasswdPath, &len)))
    {
        p = (STRPTR)data;
        while (p && *p)
        {
            STRPTR nl = strchr(p, '\n'), part[7];
            if (nl)
                *nl = '\0';
            if (*p && SplitFields(p, part, 7) == 7)
            {
                struct AccUser *u = AllocVec(sizeof(struct AccUser), MEMF_CLEAR | MEMF_PUBLIC);
                if (u)
                {
                    AccCopyName(u->Name, part[0], sizeof(u->Name));
                    AccCopyName(u->Hash, part[1], sizeof(u->Hash));
                    u->Uid = (UWORD)strtoul(part[2], NULL, 10);
                    u->Gid = (UWORD)strtoul(part[3], NULL, 10);
                    AccCopyName(u->Gecos, part[4], sizeof(u->Gecos));
                    AccCopyName(u->Home, part[5], sizeof(u->Home));
                    AccCopyName(u->Shell, part[6], sizeof(u->Shell));
                    DeriveFlags(s, u);
                    InsertUser(s, u);
                }
            }
            p = nl ? nl + 1 : NULL;
        }
        FreeVec(data);
    }
    FileDate(s->PasswdPath, &s->PasswdDate);

    /* GroupID|gid|MgrUid|GroupName, then uid:gid[,gid...] relations */
    if ((data = ReadWholeFile(s->GroupPath, &len)))
    {
        p = (STRPTR)data;
        while (p && *p)
        {
            STRPTR nl = strchr(p, '\n'), part[4];
            if (nl)
                *nl = '\0';
            if (strchr(p, '|'))
            {
                if (SplitFields(p, part, 4) == 4)
                {
                    struct AccGroup *g = AllocVec(sizeof(struct AccGroup), MEMF_CLEAR | MEMF_PUBLIC);
                    if (g)
                    {
                        AccCopyName(g->Name, part[0], sizeof(g->Name));
                        g->Gid = (UWORD)strtoul(part[1], NULL, 10);
                        g->Admin = (UWORD)strtoul(part[2], NULL, 10);
                        AccCopyName(g->Desc, part[3], sizeof(g->Desc));
                        InsertGroup(s, g);
                    }
                }
            }
            else if (strchr(p, ':'))
            {
                char *end;
                UWORD uid = (UWORD)strtoul(p, &end, 10);
                while (*end == ':' || *end == ',')
                {
                    struct AccGroup *g = StoreGroupById(s, (UWORD)strtoul(end + 1, &end, 10));
                    if (g)
                        AddMemberSorted(s, g, uid);
                }
            }
            p = nl ? nl + 1 : NULL;
        }
        FreeVec(data);
    }
    FileDate(s->GroupPath, &s->GroupDate);
    s->Loaded = TRUE;
}

static BOOL WriteText(CONST_STRPTR path, CONST_STRPTR text)
{
    BPTR fh;
    LONG len = strlen(text);
    BOOL ok;

    if (!(fh = Open(path, MODE_NEWFILE)))
        return FALSE;
    ok = Write(fh, (APTR)text, len) == len;
    Close(fh);
    return ok;
}

static void SaveFiles(struct AccStore *s)
{
    struct AccUser *u;
    struct AccGroup *g;
    STRPTR text;
    ULONG size = 1024, pos;

    ForeachNode(&s->Users, u) size += 512;
    ForeachNode(&s->Groups, g) size += 256 + 8 * g->NumMembers;
    if (!(text = AllocVec(size, MEMF_CLEAR | MEMF_PUBLIC)))
        return;

    if (s->UsersDirty)
    {
        pos = 0;
        ForeachNode(&s->Users, u)
            pos += snprintf(text + pos, size - pos, "%s|%s|%u|%u|%s|%s|%s\n", u->Name, u->Hash, u->Uid, u->Gid, u->Gecos, u->Home, u->Shell);
        if (WriteText(s->PasswdPath, text))
        {
            Log(s, "password file written");
            s->UsersDirty = FALSE;
        }
        else
            Log(s, "cannot write the password file");
    }
    if (s->GroupsDirty)
    {
        pos = 0;
        ForeachNode(&s->Groups, g)
            pos += snprintf(text + pos, size - pos, "%s|%u|%u|%s\n", g->Name, g->Gid, g->Admin, g->Desc);
        pos += snprintf(text + pos, size - pos, "\n");
        ForeachNode(&s->Users, u)
        {
            BOOL first = TRUE;
            ForeachNode(&s->Groups, g)
            {
                if (StoreIsMember(g, u->Uid))
                {
                    if (first)
                        pos += snprintf(text + pos, size - pos, "%u:%u", u->Uid, g->Gid);
                    else
                        pos += snprintf(text + pos, size - pos, ",%u", g->Gid);
                    first = FALSE;
                }
            }
            if (!first)
                pos += snprintf(text + pos, size - pos, "\n");
        }
        if (WriteText(s->GroupPath, text))
        {
            Log(s, "group file written");
            s->GroupsDirty = FALSE;
        }
        else
            Log(s, "cannot write the group file");
    }
    FreeVec(text);
    FileDate(s->PasswdPath, &s->PasswdDate);
    FileDate(s->GroupPath, &s->GroupDate);
    /* the security server reloads on the file notification; give it a moment
     * before the next request may ask to verify a changed password */
    Delay(10);
}

/*------------------------------------------------------------------------*/
/* usergroup.library backend (read-only)                                   */
/*------------------------------------------------------------------------*/

static void LoadUserGroup(struct AccStore *s)
{
    struct Library *UserGroupBase = s->UserGroupBase;
    struct passwd *pw;
    struct group *gr;

    FreeLists(s);
    ug_setpwent(UserGroupBase);
    while ((pw = ug_getpwent(UserGroupBase)))
    {
        struct AccUser *u = AllocVec(sizeof(struct AccUser), MEMF_CLEAR | MEMF_PUBLIC);
        if (!u)
            break;
        AccCopyName(u->Name, pw->pw_name, sizeof(u->Name));
        if (pw->pw_passwd && strcmp(pw->pw_passwd, "*"))
            AccCopyName(u->Hash, pw->pw_passwd, sizeof(u->Hash));
        u->Uid = UG2MU(pw->pw_uid);
        u->Gid = UG2MU(pw->pw_gid);
        AccCopyName(u->Gecos, pw->pw_gecos, sizeof(u->Gecos));
        AccCopyName(u->Home, pw->pw_dir, sizeof(u->Home));
        AccCopyName(u->Shell, pw->pw_shell, sizeof(u->Shell));
        DeriveFlags(s, u);
        InsertUser(s, u);
    }
    ug_endpwent(UserGroupBase);

    ug_setgrent(UserGroupBase);
    while ((gr = ug_getgrent(UserGroupBase)))
    {
        struct AccGroup *g = AllocVec(sizeof(struct AccGroup), MEMF_CLEAR | MEMF_PUBLIC);
        char **m;
        if (!g)
            break;
        AccCopyName(g->Name, gr->gr_name, sizeof(g->Name));
        g->Gid = UG2MU(gr->gr_gid);
        g->Admin = ACC_ROOT_UID;
        InsertGroup(s, g);
        for (m = gr->gr_mem; m && *m; m++)
        {
            struct AccUser *u = StoreUserByName(s, *m);
            if (u)
                AddMemberSorted(s, g, u->Uid);
        }
    }
    ug_endgrent(UserGroupBase);
    s->Loaded = TRUE;
}

/*------------------------------------------------------------------------*/
/* Interface                                                               */
/*------------------------------------------------------------------------*/

BOOL StoreInit(struct AccStore *s, BOOL verbose)
{
    BPTR lock;

    memset(s, 0, sizeof(*s));
    NewMinList(&s->Users);
    NewMinList(&s->Groups);
    s->Verbose = verbose;
    s->PamLib = OpenLibrary("pam.library", 0);
    s->UserGroupBase = OpenLibrary("usergroup.library", 0);

    if ((s->SecLib = OpenLibrary("security.library", 0)))
    {
        if (secIsConfigured() && (lock = secGetPasswdDirLock()))
        {
            if (NameFromLock(lock, s->PasswdPath, sizeof(s->PasswdPath)))
            {
                strcpy(s->GroupPath, s->PasswdPath);
                AddPart(s->PasswdPath, secPasswd_FileName, sizeof(s->PasswdPath));
                AddPart(s->GroupPath, secGroup_FileName, sizeof(s->GroupPath));
                s->Writable = TRUE;
            }
            UnLock(lock);
        }
        if (s->Writable && (lock = secGetConfigDirLock()))
        {
            if (NameFromLock(lock, s->ConfigPath, sizeof(s->ConfigPath)))
                AddPart(s->ConfigPath, secConfig_FileName, sizeof(s->ConfigPath));
            UnLock(lock);
        }
    }
    if (!s->Writable && !s->UserGroupBase)
    {
        Log(s, "no account store: neither a configured security.library nor usergroup.library");
        return FALSE;
    }
    Log(s, s->Writable ? "store: the system's password and group files (read/write)"
                       : "store: usergroup.library (read-only)");
    StoreRefresh(s);
    return TRUE;
}

void StoreCleanup(struct AccStore *s)
{
    FreeLists(s);
    if (s->SecLib)
        CloseLibrary(s->SecLib);
    if (s->UserGroupBase)
        CloseLibrary(s->UserGroupBase);
    if (s->PamLib)
        CloseLibrary(s->PamLib);
}

/* Reload when the files changed under us (or on the first call) */
void StoreRefresh(struct AccStore *s)
{
    if (s->Writable)
    {
        struct DateStamp dp, dg;
        BOOL changed = !s->Loaded;

        if (FileDate(s->PasswdPath, &dp) && CompareDates(&dp, &s->PasswdDate) != 0)
            changed = TRUE;
        if (FileDate(s->GroupPath, &dg) && CompareDates(&dg, &s->GroupDate) != 0)
            changed = TRUE;
        if (changed && !s->UsersDirty && !s->GroupsDirty)
        {
            LoadFiles(s);
            Log(s, "account files (re)loaded");
        }
    }
    else
        LoadUserGroup(s);
}

void StoreCommit(struct AccStore *s)
{
    if (s->Writable && (s->UsersDirty || s->GroupsDirty))
        SaveFiles(s);
}

struct AccUser *StoreUserByName(struct AccStore *s, CONST_STRPTR name)
{
    struct AccUser *u;
    if (!name || !name[0])
        return NULL;
    ForeachNode(&s->Users, u)
        if (!AccStricmp(u->Name, name))
            return u;
    return NULL;
}

struct AccUser *StoreUserById(struct AccStore *s, UWORD uid)
{
    struct AccUser *u;
    ForeachNode(&s->Users, u)
        if (u->Uid == uid)
            return u;
    return NULL;
}

struct AccGroup *StoreGroupByName(struct AccStore *s, CONST_STRPTR name)
{
    struct AccGroup *g;
    if (!name || !name[0])
        return NULL;
    ForeachNode(&s->Groups, g)
        if (!AccStricmp(g->Name, name))
            return g;
    return NULL;
}

struct AccGroup *StoreGroupById(struct AccStore *s, UWORD gid)
{
    struct AccGroup *g;
    ForeachNode(&s->Groups, g)
        if (g->Gid == gid)
            return g;
    return NULL;
}

BOOL StoreIsMember(struct AccGroup *g, UWORD uid)
{
    ULONG i;
    for (i = 0; i < g->NumMembers; i++)
        if (g->Members[i] == uid)
            return TRUE;
    return FALSE;
}

struct AccUser *StoreNextMember(struct AccStore *s, struct AccGroup *g, struct AccUser *after)
{
    ULONG i = 0;

    if (after)
    {
        while (i < g->NumMembers && g->Members[i] != after->Uid)
            i++;
        i++;
    }
    for (; i < g->NumMembers; i++)
    {
        struct AccUser *u = StoreUserById(s, g->Members[i]);
        if (u)
            return u;
    }
    return NULL;
}

/*
 * Password check. Without NeedsPassword any token verifies (the Envoy
 * rule). Otherwise the authentication layer decides: pam.library service
 * "envoy" with the token as given (clear text, or the 11-character Envoy
 * hash with PAMTOK_ENVOY). Without pam.library the stored hash is compared
 * directly.
 */
ULONG StoreVerify(struct AccStore *s, struct AccUser *u, CONST_STRPTR token, BOOL hashed, CONST_STRPTR rhost)
{
    if (!u)
        return ENVOYERR_UNKNOWNUSER;
    if (!(u->Flags & UFLAGF_NeedsPassword))
        return 0;
    if (!token)
        token = "";

    if (s->PamLib)
    {
        struct TagItem tags[] = {
            { PAMT_AuthTok,       (IPTR)token },
            { PAMT_AuthTokFormat, hashed ? PAMTOK_ENVOY : PAMTOK_CLEAR },
            { rhost ? PAMT_RemoteHost : TAG_IGNORE, (IPTR)rhost },
            { TAG_DONE, 0 }
        };
        struct PamHandle *h;
        LONG r;

        if (!(h = PamStartA("envoy", u->Name, NULL, tags)))
            return ENVOYERR_UNKNOWNUSER;
        if ((r = PamAuthenticate(h, PAM_SILENT)) == PAM_SUCCESS)
            r = PamAcctMgmt(h, PAM_SILENT);
        PamEnd(h, r);
        return r == PAM_SUCCESS ? 0 : ENVOYERR_UNKNOWNUSER;
    }
    else
    {
        char buffer[16];
        if (hashed)
            return strcmp(token, u->Hash) ? ENVOYERR_UNKNOWNUSER : 0;
        Hash(buffer, token, u->Name);
        return strcmp(buffer, u->Hash) ? ENVOYERR_UNKNOWNUSER : 0;
    }
}

static UWORD NewId(struct AccStore *s, BOOL users)
{
    ULONG max = 0;

    if (users)
    {
        struct AccUser *u;
        ForeachNode(&s->Users, u)
            if (u->Uid != ACC_ROOT_UID && u->Uid > max)
                max = u->Uid;
    }
    else
    {
        struct AccGroup *g;
        ForeachNode(&s->Groups, g)
            if (g->Gid != ACC_ROOT_UID && g->Gid > max)
                max = g->Gid;
    }
    return (max + 1 >= ACC_ROOT_UID) ? 0 : (UWORD)(max + 1);
}

ULONG StoreAddUser(struct AccStore *s, CONST_STRPTR name, UWORD gid, ULONG flags, CONST_STRPTR password, struct AccUser **result)
{
    struct AccUser *u;
    UWORD uid;

    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    if (!(uid = NewId(s, TRUE)))
        return ACCERROR_NOFREEUSERS;
    if (!(u = AllocVec(sizeof(struct AccUser), MEMF_CLEAR | MEMF_PUBLIC)))
        return ENVOYERR_NORESOURCES;
    AccCopyName(u->Name, name, sizeof(u->Name));
    u->Uid = uid;
    u->Gid = gid;
    if ((flags & UFLAGF_NeedsPassword) && password && password[0])
        Hash(u->Hash, password, u->Name);
    AccCopyName(u->Gecos, name, sizeof(u->Gecos));
    snprintf(u->Home, sizeof(u->Home), "SYS:Security/Profiles/%s", u->Name);
    strcpy(u->Shell, "Shell");
    DeriveFlags(s, u);
    InsertUser(s, u);
    s->UsersDirty = TRUE;
    *result = u;
    return 0;
}

ULONG StoreDeleteUser(struct AccStore *s, struct AccUser *u)
{
    struct AccGroup *g;

    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    ForeachNode(&s->Groups, g)
    {
        if (StoreIsMember(g, u->Uid))
        {
            StoreRemoveMember(s, g, u);
            s->GroupsDirty = TRUE;
        }
    }
    Remove((struct Node *)u);
    FreeVec(u);
    s->UsersDirty = TRUE;
    return 0;
}

ULONG StoreModifyUser(struct AccStore *s, struct AccUser *u, CONST_STRPTR name, UWORD gid, ULONG flags)
{
    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    if (name && name[0] && strcmp(name, u->Name))
    {
        AccCopyName(u->Name, name, sizeof(u->Name));
        Remove((struct Node *)u);
        InsertUser(s, u);
    }
    u->Gid = gid;
    /* the only flag with a stored counterpart: clearing it removes the password */
    if (!(flags & UFLAGF_NeedsPassword))
        u->Hash[0] = '\0';
    DeriveFlags(s, u);
    s->UsersDirty = TRUE;
    return 0;
}

ULONG StoreSetPassword(struct AccStore *s, struct AccUser *u, CONST_STRPTR password)
{
    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    if (password && password[0])
        Hash(u->Hash, password, u->Name);
    else
        u->Hash[0] = '\0';
    DeriveFlags(s, u);
    s->UsersDirty = TRUE;
    return 0;
}

ULONG StoreAddGroup(struct AccStore *s, CONST_STRPTR name, ULONG flags, UWORD admin, struct AccGroup **result)
{
    struct AccGroup *g;
    UWORD gid;

    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    if (!(gid = NewId(s, FALSE)))
        return ACCERROR_NOFREEGROUPS;
    if (!(g = AllocVec(sizeof(struct AccGroup), MEMF_CLEAR | MEMF_PUBLIC)))
        return ENVOYERR_NORESOURCES;
    AccCopyName(g->Name, name, sizeof(g->Name));
    g->Gid = gid;
    g->Admin = admin;
    g->Flags = flags;
    AccCopyName(g->Desc, name, sizeof(g->Desc));
    InsertGroup(s, g);
    s->GroupsDirty = TRUE;
    *result = g;
    return 0;
}

ULONG StoreDeleteGroup(struct AccStore *s, struct AccGroup *g)
{
    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    Remove((struct Node *)g);
    FreeVec(g->Members);
    FreeVec(g);
    s->GroupsDirty = TRUE;
    return 0;
}

ULONG StoreModifyGroup(struct AccStore *s, struct AccGroup *g, CONST_STRPTR name, UWORD gid, UWORD admin, ULONG flags)
{
    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    if (name && name[0] && strcmp(name, g->Name))
    {
        AccCopyName(g->Name, name, sizeof(g->Name));
        Remove((struct Node *)g);
        InsertGroup(s, g);
    }
    g->Gid = gid;
    g->Admin = admin;
    g->Flags = flags;
    s->GroupsDirty = TRUE;
    return 0;
}

ULONG StoreAddMember(struct AccStore *s, struct AccGroup *g, struct AccUser *u)
{
    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    if (StoreIsMember(g, u->Uid))
        return ACCERROR_USEREXISTS;
    if (!AddMemberSorted(s, g, u->Uid))
        return ENVOYERR_NORESOURCES;
    s->GroupsDirty = TRUE;
    return 0;
}

ULONG StoreRemoveMember(struct AccStore *s, struct AccGroup *g, struct AccUser *u)
{
    ULONG i;

    if (!s->Writable)
        return ACCERROR_NOPRIVS;
    for (i = 0; i < g->NumMembers; i++)
    {
        if (g->Members[i] == u->Uid)
        {
            for (; i + 1 < g->NumMembers; i++)
                g->Members[i] = g->Members[i + 1];
            g->NumMembers--;
            s->GroupsDirty = TRUE;
            return 0;
        }
    }
    return ENVOYERR_UNKNOWNMEMBER;
}
