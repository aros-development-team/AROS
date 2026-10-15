/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - secUserAdminA(): adding, changing and removing
          accounts and groups (Security.server context)

          The server owns the user database, so the change is made to its
          definitions and written with UpdateUserDefs() / UpdateGroupDefs()
          (new file, size check, rename - the old file stays if anything
          fails). On a failed write the definitions are dropped and read
          again, so that memory and disk agree.

          The group file holds the groups and, after an empty line, the
          secondary groups of the users by uid; it is rewritten when a
          group changes, and when a user who has secondary groups is
          removed or renumbered.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "security_intern.h"
#include "security_server.h"
#include "security_auth.h"
#include "security_crypto.h"
#include "security_memory.h"
#include "security_task.h"
#include "security_support.h"

/* A field may not break the password file's format */
static BOOL FieldOK(CONST_STRPTR s, ULONG size, BOOL empty)
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
    return (n < size) && (empty || n > 0);
}

static BOOL UserIDOK(CONST_STRPTR s)
{
    return FieldOK(s, secUSERIDSIZE, FALSE) && !strchr(s, ':') && !strchr(s, ' ');
}

static struct secUserDef *FindUser(struct SecurityBase *secBase, CONST_STRPTR userid, struct secUserDef **prev)
{
    struct secUserDef *def, *p = NULL;

    for (def = GetUserDefs(secBase); def; p = def, def = def->Next)
    {
        if (!strcmp(def->UserID, userid))
        {
            if (prev)
                *prev = p;
            return def;
        }
    }
    return NULL;
}

/* Another user than 'self' with this ID (case-insensitive) or uid? */
static BOOL Taken(struct SecurityBase *secBase, struct secUserDef *self, CONST_STRPTR userid, LONG uid)
{
    struct secUserDef *def;

    for (def = GetUserDefs(secBase); def; def = def->Next)
    {
        if (def == self)
            continue;
        if (userid && !Stricmp(def->UserID, userid))
            return TRUE;
        if (uid >= 0 && def->uid == (UWORD)uid)
            return TRUE;
    }
    return FALSE;
}

static BOOL GroupExists(struct SecurityBase *secBase, LONG gid)
{
    struct secGroupDef *g;

    for (g = GetGroupDefs(secBase); g; g = g->Next)
        if (g->gid == (UWORD)gid)
            return TRUE;
    return FALSE;
}

/* A new definition from fields; the hash is copied as stored */
static struct secUserDef *BuildDef(struct SecurityBase *secBase, CONST_STRPTR userid, CONST_STRPTR hash, UWORD uid, UWORD gid,
                                   CONST_STRPTR name, CONST_STRPTR home, CONST_STRPTR shell)
{
    STRPTR line;
    struct secUserDef *def = NULL;
    ULONG len = strlen(userid) + strlen(hash) + strlen(name) + strlen(home) + strlen(shell) + 32;
    SIPTR args[7];

    if (!(line = MAllocV(len)))
        return NULL;
    args[0] = (SIPTR)userid;
    args[1] = (SIPTR)hash;
    args[2] = uid;
    args[3] = gid;
    args[4] = (SIPTR)name;
    args[5] = (SIPTR)home;
    args[6] = (SIPTR)shell;
    FormatString("%s|%s|%iu|%iu|%s|%s|%s", args, line, len);
    def = MakeUserDef(secBase, line);
    FreeV(line);
    return def;
}

static void FreeDef(struct secUserDef *def)
{
    if (def->NumSecGroups)
        Free(def->SecGroups, def->NumSecGroups * sizeof(UWORD));
    FreeV(def);
}

/* Memory no longer matches the disk: read the files again */
static void Reload(struct SecurityBase *secBase)
{
    FreeDefs(secBase);
    GetUserDefs(secBase);
}

/* groups: the group file too - when a user with secondary groups went
 * away or got another uid */
static ULONG Commit(struct SecurityBase *secBase, CONST_STRPTR what, CONST_STRPTR userid, BOOL groups)
{
    SIPTR args[2] = { (SIPTR)what, (SIPTR)userid };

    if (!UpdateUserDefs(secBase))
    {
        Reload(secBase);
        return secUAERR_WRITE;
    }
    VLogF(secBase, LOGF_Flag_Type_Information, SECLOG_SUB_SERVER, "account %s: '%s'", args);
    if (groups && !UpdateGroupDefs(secBase))
    {
        /* the account changed; its secondary groups are as before */
        VLogF(secBase, LOGF_Flag_Type_Error, SECLOG_SUB_SERVER, "account %s: '%s', but its groups could not be updated", args);
        Reload(secBase);
    }
    return secUAERR_OK;
}

/* ------------------------------------------------------------------------ */
/* Groups                                                                   */

static BOOL GroupIDOK(CONST_STRPTR s)
{
    return FieldOK(s, secGROUPIDSIZE, FALSE) && !strchr(s, ':') && !strchr(s, ' ');
}

static struct secGroupDef *FindGroup(struct SecurityBase *secBase, CONST_STRPTR groupid, struct secGroupDef **prev)
{
    struct secGroupDef *def, *p = NULL;

    for (def = GetGroupDefs(secBase); def; p = def, def = def->Next)
    {
        if (!strcmp(def->GroupID, groupid))
        {
            if (prev)
                *prev = p;
            return def;
        }
    }
    return NULL;
}

/* Another group than 'self' with this ID (case-insensitive) or gid? */
static BOOL GroupTaken(struct SecurityBase *secBase, struct secGroupDef *self, CONST_STRPTR groupid, LONG gid)
{
    struct secGroupDef *def;

    for (def = GetGroupDefs(secBase); def; def = def->Next)
    {
        if (def == self)
            continue;
        if (groupid && !Stricmp(def->GroupID, groupid))
            return TRUE;
        if (gid >= 0 && def->gid == (UWORD)gid)
            return TRUE;
    }
    return FALSE;
}

/* Somebody's primary group? */
static BOOL PrimaryGroup(struct SecurityBase *secBase, UWORD gid)
{
    struct secUserDef *def;

    for (def = GetUserDefs(secBase); def; def = def->Next)
        if (def->gid == gid)
            return TRUE;
    return FALSE;
}

static struct secGroupDef *BuildGroupDef(CONST_STRPTR groupid, UWORD gid, UWORD mgr, CONST_STRPTR name)
{
    struct secGroupDef *def;
    STRPTR ptr;

    if ((def = MAllocV(sizeof(struct secGroupDef) + strlen(groupid) + strlen(name) + 2)))
    {
        ptr = &((STRPTR)def)[sizeof(struct secGroupDef)];
        def->GroupID = ptr;
        strcpy(ptr, groupid);
        ptr += strlen(groupid) + 1;
        def->GroupName = ptr;
        strcpy(ptr, name);
        def->gid = gid;
        def->MgrUid = mgr;
    }
    return def;
}

/* Add gid to a user's secondary groups (or remove it); FALSE without memory */
static BOOL SetSecGroup(struct secUserDef *def, UWORD gid, BOOL member)
{
    UWORD *groups;
    ULONG i, n = 0;
    BOOL is = FALSE;

    for (i = 0; i < def->NumSecGroups; i++)
        if (def->SecGroups[i] == gid)
            is = TRUE;
    if (is == member)
        return TRUE;

    if (member)
    {
        if (def->NumSecGroups == 65535 || !(groups = MAlloc((def->NumSecGroups + 1) * sizeof(UWORD))))
            return FALSE;
        if (def->NumSecGroups)
            CopyMem(def->SecGroups, groups, def->NumSecGroups * sizeof(UWORD));
        groups[def->NumSecGroups] = gid;
        n = def->NumSecGroups + 1;
    }
    else
    {
        groups = NULL;
        if (def->NumSecGroups > 1)
        {
            if (!(groups = MAlloc((def->NumSecGroups - 1) * sizeof(UWORD))))
                return FALSE;
            for (i = 0; i < def->NumSecGroups; i++)
                if (def->SecGroups[i] != gid)
                    groups[n++] = def->SecGroups[i];
        }
    }
    if (def->NumSecGroups)
        Free(def->SecGroups, def->NumSecGroups * sizeof(UWORD));
    def->SecGroups = groups;
    def->NumSecGroups = n;
    return TRUE;
}

static BOOL Listed(CONST_STRPTR *members, CONST_STRPTR userid)
{
    if (members)
        for (; *members; members++)
            if (!strcmp(*members, userid))
                return TRUE;
    return FALSE;
}

/* Every listed member must be a user */
static BOOL MembersOK(struct SecurityBase *secBase, CONST_STRPTR *members)
{
    if (members)
        for (; *members; members++)
            if (!FindUser(secBase, *members, NULL))
                return FALSE;
    return TRUE;
}

/* The users in 'members' (and nobody else) get gid as a secondary group;
 * a user whose primary group it is needs no entry */
static BOOL SetMembers(struct SecurityBase *secBase, UWORD gid, CONST_STRPTR *members)
{
    struct secUserDef *def;

    for (def = GetUserDefs(secBase); def; def = def->Next)
        if (!SetSecGroup(def, gid, def->gid != gid && Listed(members, def->UserID)))
            return FALSE;
    return TRUE;
}

static ULONG CommitGroups(struct SecurityBase *secBase, CONST_STRPTR what, CONST_STRPTR groupid)
{
    SIPTR args[2] = { (SIPTR)what, (SIPTR)groupid };

    if (!UpdateGroupDefs(secBase))
    {
        Reload(secBase);
        return secUAERR_WRITE;
    }
    VLogF(secBase, LOGF_Flag_Type_Information, SECLOG_SUB_SERVER, "group %s: '%s'", args);
    return secUAERR_OK;
}

static ULONG GroupAdmin(struct SecurityBase *secBase, struct secUARequest *req)
{
    struct secGroupDef *def, *prev = NULL, *nd;
    struct secUserDef *udef;
    ULONG i;

    if (!req->GroupID || !req->GroupID[0])
        return secUAERR_BADARGS;

    switch (req->Operation)
    {
    case secUA_GROUPADD:
        if (!GroupIDOK(req->GroupID) || req->gid < 0 || req->gid > 65535 || req->MgrUid > 65535 ||
            !FieldOK(req->GroupName, secGROUPNAMESIZE, TRUE))
            return secUAERR_BADARGS;
        if (GroupTaken(secBase, NULL, req->GroupID, req->gid))
            return secUAERR_EXISTS;
        if (req->HasMembers && !MembersOK(secBase, req->Members))
            return secUAERR_NOTFOUND;
        if (!(nd = BuildGroupDef(req->GroupID, (UWORD)req->gid, req->MgrUid >= 0 ? (UWORD)req->MgrUid : secROOT_UID,
                                 req->GroupName ? req->GroupName : (CONST_STRPTR)"")))
            return secUAERR_WRITE;
        for (def = GetGroupDefs(secBase); def->Next; def = def->Next)
            ;
        def->Next = nd;
        if (req->HasMembers && !SetMembers(secBase, nd->gid, req->Members))
        {
            Reload(secBase);
            return secUAERR_WRITE;
        }
        return CommitGroups(secBase, "added", req->GroupID);

    case secUA_GROUPMODIFY:
        if (!(def = FindGroup(secBase, req->GroupID, &prev)))
            return secUAERR_NOTFOUND;
        if ((req->NewGroupID && !GroupIDOK(req->NewGroupID)) || req->gid > 65535 || req->MgrUid > 65535 ||
            (req->GroupName && !FieldOK(req->GroupName, secGROUPNAMESIZE, TRUE)))
            return secUAERR_BADARGS;
        if (req->gid >= 0 && req->gid != def->gid)
        {
            /* root's group, and the groups of users, keep their numbers */
            if (def->gid == secROOT_GID)
                return secUAERR_PROTECTED;
            if (PrimaryGroup(secBase, def->gid))
                return secUAERR_INUSE;
        }
        if (def->gid == secROOT_GID && req->NewGroupID && strcmp(req->NewGroupID, def->GroupID))
            return secUAERR_PROTECTED;
        if (GroupTaken(secBase, def, req->NewGroupID, req->gid))
            return secUAERR_EXISTS;
        if (req->HasMembers && !MembersOK(secBase, req->Members))
            return secUAERR_NOTFOUND;
        if (!(nd = BuildGroupDef(req->NewGroupID ? req->NewGroupID : def->GroupID,
                                 req->gid >= 0 ? (UWORD)req->gid : def->gid,
                                 req->MgrUid >= 0 ? (UWORD)req->MgrUid : def->MgrUid,
                                 req->GroupName ? req->GroupName : def->GroupName)))
            return secUAERR_WRITE;
        nd->Next = def->Next;
        if (prev)
            prev->Next = nd;
        else
            secBase->GroupDefs = nd;
        /* renumbered: the members follow */
        if (nd->gid != def->gid)
        {
            for (udef = GetUserDefs(secBase); udef; udef = udef->Next)
                for (i = 0; i < udef->NumSecGroups; i++)
                    if (udef->SecGroups[i] == def->gid)
                        udef->SecGroups[i] = nd->gid;
        }
        FreeV(def);
        if (req->HasMembers && !SetMembers(secBase, nd->gid, req->Members))
        {
            Reload(secBase);
            return secUAERR_WRITE;
        }
        return CommitGroups(secBase, "changed", nd->GroupID);

    case secUA_GROUPDELETE:
        if (!(def = FindGroup(secBase, req->GroupID, &prev)))
            return secUAERR_NOTFOUND;
        if (def->gid == secROOT_GID)
            return secUAERR_PROTECTED;
        if (PrimaryGroup(secBase, def->gid))
            return secUAERR_INUSE;
        if (!SetMembers(secBase, def->gid, NULL))
        {
            Reload(secBase);
            return secUAERR_WRITE;
        }
        if (prev)
            prev->Next = def->Next;
        else
            secBase->GroupDefs = def->Next;
        FreeV(def);
        return CommitGroups(secBase, "deleted", req->GroupID);
    }
    return secUAERR_BADARGS;
}

ULONG UserAdmin(struct SecurityBase *secBase, struct Task *client, struct secUARequest *req)
{
    struct secUserDef *def, *prev = NULL, *nd;
    char hash[secHASHBUFSIZE];
    BOOL groups;

    if (!client || (GetTaskOwner(secBase, client) >> 16) != secROOT_UID)
        return secUAERR_NOTROOT;
    if (!secBase->Configured || !GetUserDefs(secBase) || !GetGroupDefs(secBase))
        return secUAERR_UNAVAILABLE;
    if (!req)
        return secUAERR_BADARGS;
    if (req->Operation >= secUA_GROUPADD && req->Operation <= secUA_GROUPDELETE)
        return GroupAdmin(secBase, req);
    if (!req->UserID || !req->UserID[0])
        return secUAERR_BADARGS;

    switch (req->Operation)
    {
    case secUA_ADD:
        if (!UserIDOK(req->UserID) || req->uid < 1 || req->uid > 65535 || req->gid < 0 || req->gid > 65535 ||
            !FieldOK(req->UserName, secUSERNAMESIZE, TRUE) || !FieldOK(req->HomeDir, secHOMEDIRSIZE, TRUE) ||
            !FieldOK(req->Shell, secSHELLSIZE, TRUE) || (req->Password && strlen(req->Password) >= secPASSWORDSIZE))
            return secUAERR_BADARGS;
        if (Taken(secBase, NULL, req->UserID, req->uid))
            return secUAERR_EXISTS;
        if (!GroupExists(secBase, req->gid))
            return secUAERR_NOGROUP;
        hash[0] = '\0';
        if (req->Password && req->Password[0] && !EncryptPassword(secBase, hash, req->UserID, req->Password))
            return secUAERR_BADARGS;
        if (!(nd = BuildDef(secBase, req->UserID, hash, req->uid, req->gid, req->UserName ? req->UserName : (CONST_STRPTR)"",
                            req->HomeDir ? req->HomeDir : (CONST_STRPTR)"", req->Shell ? req->Shell : (CONST_STRPTR)"")))
        {
            memset(hash, 0, sizeof(hash));
            return secUAERR_BADARGS;
        }
        memset(hash, 0, sizeof(hash));
        for (def = GetUserDefs(secBase); def->Next; def = def->Next)
            ;
        def->Next = nd;
        return Commit(secBase, "added", req->UserID, FALSE);

    case secUA_MODIFY:
        if (!(def = FindUser(secBase, req->UserID, &prev)))
            return secUAERR_NOTFOUND;
        if ((req->NewUserID && !UserIDOK(req->NewUserID)) || (req->uid >= 0 && (req->uid < 1 || req->uid > 65535)) ||
            (req->gid >= 0 && req->gid > 65535) ||
            (req->UserName && !FieldOK(req->UserName, secUSERNAMESIZE, TRUE)) ||
            (req->HomeDir && !FieldOK(req->HomeDir, secHOMEDIRSIZE, TRUE)) ||
            (req->Shell && !FieldOK(req->Shell, secSHELLSIZE, TRUE)))
            return secUAERR_BADARGS;
        /* root keeps its name and number */
        if (def->uid == secROOT_UID &&
            ((req->NewUserID && strcmp(req->NewUserID, def->UserID)) || (req->uid >= 0 && req->uid != secROOT_UID)))
            return secUAERR_PROTECTED;
        if (Taken(secBase, def, req->NewUserID, req->uid))
            return secUAERR_EXISTS;
        if (req->gid >= 0 && !GroupExists(secBase, req->gid))
            return secUAERR_NOGROUP;
        /* The ACrypt hash is salted with the user ID: a renamed account
         * keeps a hash its new name does not match. Refuse a rename of an
         * account with a password unless one is set with it. */
        if (req->NewUserID && strcmp(req->NewUserID, def->UserID) && def->Password[0] &&
            !(req->Password))
            return secUAERR_BADARGS;
        hash[0] = '\0';
        if (req->Password)
        {
            if (req->Password[0] && !EncryptPassword(secBase, hash, req->NewUserID ? req->NewUserID : def->UserID, req->Password))
                return secUAERR_BADARGS;
        }
        else
            strncpy(hash, def->Password, sizeof(hash) - 1);
        nd = BuildDef(secBase, req->NewUserID ? req->NewUserID : def->UserID, hash,
                      req->uid >= 0 ? (UWORD)req->uid : def->uid, req->gid >= 0 ? (UWORD)req->gid : def->gid,
                      req->UserName ? req->UserName : def->UserName, req->HomeDir ? req->HomeDir : def->HomeDir,
                      req->Shell ? req->Shell : def->Shell);
        memset(hash, 0, sizeof(hash));
        if (!nd)
            return secUAERR_BADARGS;
        /* the secondary groups come from the group file */
        nd->SecGroups = def->SecGroups;
        nd->NumSecGroups = def->NumSecGroups;
        def->SecGroups = NULL;
        def->NumSecGroups = 0;
        nd->Next = def->Next;
        if (prev)
            prev->Next = nd;
        else
            secBase->UserDefs = nd;
        groups = (nd->NumSecGroups && nd->uid != def->uid);
        FreeDef(def);
        return Commit(secBase, "changed", req->NewUserID ? req->NewUserID : req->UserID, groups);

    case secUA_DELETE:
        if (!(def = FindUser(secBase, req->UserID, &prev)))
            return secUAERR_NOTFOUND;
        if (def->uid == secROOT_UID)
            return secUAERR_PROTECTED;
        if (prev)
            prev->Next = def->Next;
        else
            secBase->UserDefs = def->Next;
        groups = (def->NumSecGroups != 0);
        FreeDef(def);
        return Commit(secBase, "deleted", req->UserID, groups);

    case secUA_PASSWORD:
        if (!(def = FindUser(secBase, req->UserID, NULL)))
            return secUAERR_NOTFOUND;
        if (!req->Password || strlen(req->Password) >= secPASSWORDSIZE)
            return secUAERR_BADARGS;
        if (!req->Password[0])
            def->Password[0] = '\0';
        else if (!EncryptPassword(secBase, def->Password, def->UserID, req->Password))
            return secUAERR_BADARGS;
        return Commit(secBase, "password set", req->UserID, FALSE);
    }
    return secUAERR_BADARGS;
}
