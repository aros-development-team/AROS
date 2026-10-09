/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Accounts Server - the nineteen commands (re/spec/services-accounts.md
          §6). The request buffer is read and rewritten; the caller copies it
          to the response buffer when that is a different one (§6.1). Where
          the original reads the first record of a creation request and the
          real library fills only the second (§9.1), the second is used.
*/

#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

#include "acc_store.h"
#include "acc_cmds.h"

static inline UWORD Get16(const UBYTE *p) { return (p[0] << 8) | p[1]; }
static inline ULONG Get32(const UBYTE *p) { return ((ULONG)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
static inline void Put16(UBYTE *p, UWORD v) { p[0] = v >> 8; p[1] = v; }
static inline void Put32(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

/* A string field: at most 31 characters, stop at the first NUL (§6.2) */
static void GetName(char *dst, const UBYTE *src)
{
    int n = 0;
    while (n < 31 && src[n])
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}

static void PutName(UBYTE *dst, CONST_STRPTR src)
{
    int n = 0;
    while (n < 31 && src[n])
    {
        dst[n] = src[n];
        n++;
    }
    memset(dst + n, 0, 32 - n);
}

/* U := user record, password field zeroed */
static void PutUser(UBYTE *rec, const struct AccUser *u)
{
    PutName(rec, u->Name);
    Put16(rec + 32, u->Uid);
    Put16(rec + 34, u->Gid);
    Put32(rec + 36, u->Flags);
    memset(rec + 40, 0, 32);
}

static void PutGroup(UBYTE *rec, const struct AccGroup *g)
{
    PutName(rec, g->Name);
    Put16(rec + 32, g->Gid);
    Put16(rec + 34, g->Admin);
    Put32(rec + 36, g->Flags);
}

static BOOL IsStart(const UBYTE *rec)
{
    return Get16(rec + 32) == 0 && rec[0] == '\0';
}

/*
 * The record-match rule (§6.2): ID, name, and the other fields all agree. The name is
 * looked up first: a usergroup.library database can give two accounts the same ID
 * (AROSTCP's default passwd has daemon and guest both at 1), and an ID lookup would then
 * find the wrong one and end every iteration there.
 */
static struct AccUser *MatchUser(struct AccStore *s, const UBYTE *rec)
{
    struct AccUser *u;
    char name[32];

    GetName(name, rec);
    if (!(u = StoreUserByName(s, name)))
        return NULL;
    if (u->Uid != Get16(rec + 32) || u->Gid != Get16(rec + 34) || u->Flags != Get32(rec + 36))
        return NULL;
    return u;
}

static struct AccGroup *MatchGroup(struct AccStore *s, const UBYTE *rec)
{
    struct AccGroup *g;
    char name[32];

    GetName(name, rec);
    if (!(g = StoreGroupByName(s, name)))
        return NULL;
    if (g->Gid != Get16(rec + 32) || g->Admin != Get16(rec + 34) || g->Flags != Get32(rec + 36))
        return NULL;
    return g;
}

/* Block A: the caller, authenticated by name and clear-text password */
static struct AccUser *Authority(struct AccStore *s, const UBYTE *req, CONST_STRPTR rhost)
{
    char name[32], pw[32];
    struct AccUser *u;

    GetName(name, req);
    GetName(pw, req + 32);
    if (!(u = StoreUserByName(s, name)))
        return NULL;
    if (StoreVerify(s, u, pw, FALSE, rhost))
        return NULL;
    return u;
}

static BOOL IsAdmin(const struct AccUser *u) { return (u->Flags & UFLAGF_AdminAll) != 0; }
static BOOL ManagesGroup(const struct AccUser *u, const struct AccGroup *g) { return IsAdmin(u) || g->Admin == u->Uid; }

/* "NewUser", "NewUser2", ... / "NewGroup", "NewGroup2", ... */
static void DefaultName(struct AccStore *s, char *name, BOOL user)
{
    int n;
    for (n = 1; n < 1000; n++)
    {
        if (n == 1)
            strcpy(name, user ? "NewUser" : "NewGroup");
        else
            snprintf(name, 32, user ? "NewUser%d" : "NewGroup%d", n);
        if (!(user ? (APTR)StoreUserByName(s, name) : (APTR)StoreGroupByName(s, name)))
            return;
    }
    name[0] = '\0';
}

/*------------------------------------------------------------------------*/

ULONG AccHandle(struct AccStore *s, UBYTE cmd, UBYTE *req, ULONG len, CONST_STRPTR rhost)
{
    struct AccUser *u, *auth, *target;
    struct AccGroup *g;
    char name[32];
    ULONG err;

    switch (cmd)
    {
    case 206:   /* NextUser: U */
        if (len < 72) return ENVOYERR_SMALLREQBUFF;
        if (IsStart(req))
            u = (struct AccUser *)GetHead(&s->Users);
        else
            u = (u = MatchUser(s, req)) ? (struct AccUser *)GetSucc(u) : NULL;
        if (!u)
        {
            memset(req, 0, 72);
            return ENVOYERR_LASTUSER;
        }
        PutUser(req, u);
        return 0;

    case 208:   /* NextGroup: G */
        if (len < 40) return ENVOYERR_SMALLREQBUFF;
        if (IsStart(req))
            g = (struct AccGroup *)GetHead(&s->Groups);
        else
            g = (g = MatchGroup(s, req)) ? (struct AccGroup *)GetSucc(g) : NULL;
        if (!g)
        {
            memset(req, 0, 40);
            return ENVOYERR_LASTGROUP;
        }
        PutGroup(req, g);
        return 0;

    case 207:   /* NextMember: G, U @40 */
        if (len < 112) return ENVOYERR_SMALLREQBUFF;
        if (!(g = MatchGroup(s, req)))
        {
            memset(req + 40, 0, 72);
            return ENVOYERR_UNKNOWNGROUP;
        }
        if (IsStart(req + 40))
            u = StoreNextMember(s, g, NULL);
        else
            u = ((u = MatchUser(s, req + 40)) && StoreIsMember(g, u->Uid)) ? StoreNextMember(s, g, u) : NULL;
        if (!u)
        {
            memset(req + 40, 0, 72);
            return ENVOYERR_LASTMEMBER;
        }
        PutUser(req + 40, u);
        return 0;

    case 209:   /* IDToUser */
        if (len < 72) return ENVOYERR_SMALLREQBUFF;
        if (!(u = StoreUserById(s, Get16(req + 32))))
            return ENVOYERR_UNKNOWNUSER;
        PutUser(req, u);
        return 0;

    case 210:   /* NameToUser */
        if (len < 72) return ENVOYERR_SMALLREQBUFF;
        GetName(name, req);
        if (!(u = StoreUserByName(s, name)))
            return ENVOYERR_UNKNOWNUSER;
        PutUser(req, u);
        return 0;

    case 211:   /* IDToGroup */
        if (len < 40) return ENVOYERR_SMALLREQBUFF;
        if (!(g = StoreGroupById(s, Get16(req + 32))))
            return ENVOYERR_UNKNOWNGROUP;
        PutGroup(req, g);
        return 0;

    case 212:   /* NameToGroup */
        if (len < 40) return ENVOYERR_SMALLREQBUFF;
        GetName(name, req);
        if (!(g = StoreGroupByName(s, name)))
            return ENVOYERR_UNKNOWNGROUP;
        PutGroup(req, g);
        return 0;

    case 213:   /* VerifyUser: name, clear password @40 */
    case 217:   /* VerifyUserCrypt: name, ECrypt string @40 */
    {
        char token[32];
        if (len < 72) return ENVOYERR_SMALLREQBUFF;
        GetName(name, req);
        GetName(token, req + 40);
        if (!(u = StoreUserByName(s, name)) || StoreVerify(s, u, token, cmd == 217, rhost))
            return ENVOYERR_UNKNOWNUSER;
        PutUser(req, u);
        return 0;
    }

    case 216:   /* MemberOf: G, U @40 */
        if (len < 112) return ENVOYERR_SMALLREQBUFF;
        if (!(g = MatchGroup(s, req)))
            return ENVOYERR_UNKNOWNGROUP;
        if (!(u = MatchUser(s, req + 40)))
            return ENVOYERR_UNKNOWNUSER;
        return StoreIsMember(g, u->Uid) ? 0 : ENVOYERR_UNKNOWNMEMBER;

    /*------------------------------------------------------------------*/

    case 200:   /* AddUser: A, U1 @64, U2 @136 (+ password @176) */
    {
        char pw[32];
        if (len < 208) return ENVOYERR_SMALLREQBUFF;
        if (!(auth = Authority(s, req, rhost))) return ACCERROR_NOAUTHORITY;
        if (!IsAdmin(auth)) return ACCERROR_NOPRIVS;
        GetName(name, req + 136);
        if (!name[0])
            DefaultName(s, name, TRUE);
        if (!name[0] || StoreUserByName(s, name))
            return ACCERROR_USEREXISTS;
        GetName(pw, req + 176);
        if ((err = StoreAddUser(s, name, Get16(req + 170), Get32(req + 172), pw, &u)))
            return err;
        PutUser(req + 64, u);
        PutUser(req + 136, u);
        return 0;
    }

    case 201:   /* DeleteUser: A, U1 @64 */
        if (len < 208) return ENVOYERR_SMALLREQBUFF;
        if (!(auth = Authority(s, req, rhost))) return ACCERROR_NOAUTHORITY;
        if (!IsAdmin(auth)) return ACCERROR_NOPRIVS;
        if (!(target = MatchUser(s, req + 64))) return ENVOYERR_UNKNOWNUSER;
        if (target->Uid == 0 || target->Uid == ACC_ROOT_UID) return ACCERROR_NOPRIVS;
        return StoreDeleteUser(s, target);

    case 202:   /* AddGroup: A, G1 @64, G2 @104 */
        if (len < 144) return ENVOYERR_SMALLREQBUFF;
        if (!(auth = Authority(s, req, rhost))) return ACCERROR_NOAUTHORITY;
        if (!(auth->Flags & (UFLAGF_AdminGroups | UFLAGF_AdminAll))) return ACCERROR_NOPRIVS;
        GetName(name, req + 104);
        if (!name[0])
            DefaultName(s, name, FALSE);
        if (!name[0] || StoreGroupByName(s, name))
            return ACCERROR_GROUPEXISTS;
        if ((err = StoreAddGroup(s, name, Get32(req + 140), auth->Uid, &g)))
            return err;
        PutGroup(req + 64, g);
        PutGroup(req + 104, g);
        return 0;

    case 203:   /* DeleteGroup: A, G1 @64 */
        if (len < 144) return ENVOYERR_SMALLREQBUFF;
        if (!(auth = Authority(s, req, rhost))) return ACCERROR_NOAUTHORITY;
        if (!(g = MatchGroup(s, req + 64))) return ENVOYERR_UNKNOWNGROUP;
        if (!ManagesGroup(auth, g)) return ACCERROR_NOPRIVS;
        return StoreDeleteGroup(s, g);

    case 204:   /* AddMember: A, G @64, U @104 */
    case 205:   /* RemoveMember */
        if (len < 176) return ENVOYERR_SMALLREQBUFF;
        if (!(auth = Authority(s, req, rhost))) return ACCERROR_NOAUTHORITY;
        if (!(g = MatchGroup(s, req + 64))) return ENVOYERR_UNKNOWNGROUP;
        if (!ManagesGroup(auth, g)) return ACCERROR_NOPRIVS;
        if (!(u = MatchUser(s, req + 104))) return ENVOYERR_UNKNOWNUSER;
        return cmd == 204 ? StoreAddMember(s, g, u) : StoreRemoveMember(s, g, u);

    case 214:   /* ModifyUser: A, U1 @64 = new values, U2 @136 = as it is now */
    {
        struct AccUser *other;
        if (len < 208) return ENVOYERR_SMALLREQBUFF;
        if (!(auth = Authority(s, req, rhost))) return ACCERROR_NOAUTHORITY;
        if (!(target = MatchUser(s, req + 136))) return ENVOYERR_UNKNOWNUSER;
        GetName(name, req + 64);
        if ((other = StoreUserByName(s, name)) && other != target) return ACCERROR_USEREXISTS;
        if (IsAdmin(auth))
            err = StoreModifyUser(s, target, name, Get16(req + 98), Get32(req + 100));
        else if (auth == target && (auth->Flags & UFLAGF_AdminName))
            err = StoreModifyUser(s, target, name, target->Gid, target->Flags);
        else
            return ACCERROR_NOPRIVS;
        if (err)
            return err;
        PutUser(req + 64, target);
        return 0;
    }

    case 218:   /* SetPassword: A, U2 @136 = target, new password @176 */
    {
        char pw[32];
        if (len < 208) return ENVOYERR_SMALLREQBUFF;
        if (!(auth = Authority(s, req, rhost))) return ACCERROR_NOAUTHORITY;
        if (!(target = MatchUser(s, req + 136))) return ENVOYERR_UNKNOWNUSER;
        if (!(IsAdmin(auth) || (auth == target && (auth->Flags & UFLAGF_AdminPassword)))) return ACCERROR_NOPRIVS;
        GetName(pw, req + 176);
        return StoreSetPassword(s, target, pw);
    }

    case 215:   /* ModifyGroup: A, G1 @64 = new values, G2 @104 = as it is now */
    {
        struct AccGroup *other;
        if (len < 144) return ENVOYERR_SMALLREQBUFF;
        if (!(auth = Authority(s, req, rhost))) return ACCERROR_NOAUTHORITY;
        if (!(g = MatchGroup(s, req + 104))) return ENVOYERR_UNKNOWNGROUP;
        GetName(name, req + 64);
        if ((other = StoreGroupByName(s, name)) && other != g) return ACCERROR_GROUPEXISTS;
        if (!ManagesGroup(auth, g)) return ACCERROR_NOPRIVS;
        if ((err = StoreModifyGroup(s, g, name, Get16(req + 96), Get16(req + 98), Get32(req + 100))))
            return err;
        PutGroup(req + 64, g);
        return 0;
    }

    default:
        return ENVOYERR_CMDUNKNOWN;
    }
}
