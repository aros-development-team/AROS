/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - secUserAdminA(): adding, changing and removing
          accounts (Security.server context)

          The server owns the user database, so the change is made to its
          definitions and written with UpdateUserDefs() (new file, size
          check, rename - the old file stays if anything fails). On a
          failed write the definitions are dropped and read again, so that
          memory and disk agree.
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

static ULONG Commit(struct SecurityBase *secBase, CONST_STRPTR what, CONST_STRPTR userid)
{
    SIPTR args[2] = { (SIPTR)what, (SIPTR)userid };

    if (!UpdateUserDefs(secBase))
    {
        /* read the file again: memory must not differ from the disk */
        FreeDefs(secBase);
        GetUserDefs(secBase);
        return secUAERR_WRITE;
    }
    VLogF(secBase, LOGF_Flag_Type_Information, SECLOG_SUB_SERVER, "account %s: '%s'", args);
    return secUAERR_OK;
}

ULONG UserAdmin(struct SecurityBase *secBase, struct Task *client, struct secUARequest *req)
{
    struct secUserDef *def, *prev = NULL, *nd;
    char hash[secHASHBUFSIZE];

    if (!client || (GetTaskOwner(secBase, client) >> 16) != secROOT_UID)
        return secUAERR_NOTROOT;
    if (!secBase->Configured || !GetUserDefs(secBase))
        return secUAERR_UNAVAILABLE;
    if (!req || !req->UserID || !req->UserID[0])
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
        return Commit(secBase, "added", req->UserID);

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
        FreeDef(def);
        return Commit(secBase, "changed", req->NewUserID ? req->NewUserID : req->UserID);

    case secUA_DELETE:
        if (!(def = FindUser(secBase, req->UserID, &prev)))
            return secUAERR_NOTFOUND;
        if (def->uid == secROOT_UID)
            return secUAERR_PROTECTED;
        if (prev)
            prev->Next = def->Next;
        else
            secBase->UserDefs = def->Next;
        FreeDef(def);
        return Commit(secBase, "deleted", req->UserID);

    case secUA_PASSWORD:
        if (!(def = FindUser(secBase, req->UserID, NULL)))
            return secUAERR_NOTFOUND;
        if (!req->Password || strlen(req->Password) >= secPASSWORDSIZE)
            return secUAERR_BADARGS;
        if (!req->Password[0])
            def->Password[0] = '\0';
        else if (!EncryptPassword(secBase, def->Password, def->UserID, req->Password))
            return secUAERR_BADARGS;
        return Commit(secBase, "password set", req->UserID);
    }
    return secUAERR_BADARGS;
}
