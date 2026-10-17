/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library authentication process ("Security.auth").

          Answers secVerifyUserA(), and every password check of the login
          requests: is this token right for this user. The process is
          separate from Security.server so that a login attempt, however
          slow its hash or however many arrive from the network, never
          stalls the server that the filesystems depend on. The server
          stays the owner of the user database; the hash of one user is
          fetched from it with a private request that only this process may
          make (secSAction_GetUserHash), and the comparison, the failure
          bookkeeping, the log line and the monitors are handled here.

          Failures are slowed down per user name: after the n-th failure
          for a name, a request for that name is held back (not refused)
          for min(2^(n-1), LOGINDELAYMAX) seconds, however many tasks or
          hosts send them. Names are hashed into a fixed table with a key
          chosen at start-up, so known and unknown names behave the same and
          nothing can be pushed out of it. Requests from a remote host
          (secT_RemoteHost, accepted from root only) are in addition locked
          out after MAXTRIES failures for LOCKTIME seconds; local requests
          are never locked out, only slowed down, so that nobody can keep
          root away from the console. All times come from the timer's
          up-time, which no task can set.

          A successful verification can also issue a one-time login ticket,
          which secLoginA() turns into a login for the same task without
          asking for a password again (RedeemTicket()). Tickets are 128-bit
          random values from entropy.resource (none are issued without it),
          bound to the task and its unique ID, valid for a few seconds, and
          dropped when the task ends or the holder gives them back.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/timer.h>
#include <proto/entropy.h>
#include <proto/pam.h>
#include <libraries/pam.h>
#include <clib/alib_protos.h>
#include <dos/dosextens.h>
#include <devices/timer.h>
#include <resources/entropy.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_auth.h"
#include "security_keys.h"
#include "security_server.h"
#include "security_task.h"
#include "security_monitor.h"
#include "security_memory.h"
#include "security_crypto.h"
#include "security_userinfo.h"

#define AUTHPRI                 (2)             /* below the server (4) */
#define AUTHSTACK               (AROS_STACKSIZE * 2)

/* Per-user throttle bucket */
struct secThrottle
{
    ULONG                   Fails;
    ULONG                   LastFail;
    ULONG                   NextAllowed;    /* up-time seconds */
    char                    UserID[secUSERIDSIZE];  /* last name that failed here (secAuthControlA()) */
};

/* One lock-out record per (user, remote host) */
struct secTally
{
    struct MinNode          Node;
    ULONG                   Fails;
    ULONG                   LockedUntil;    /* seconds, 0 = not locked */
    ULONG                   LastFail;
    char                    UserID[secUSERIDSIZE];
    char                    Host[secAUTH_HOSTMAX + 1];
};

/* One outstanding login ticket */
struct secAuthTicket
{
    struct MinNode          Node;
    struct secLoginTicket   Id;
    ULONG                   Expires;        /* seconds */
    struct Task             *Task;          /* the task it was issued to */
    ULONG                   TaskUnique;     /* and its et_UniqueID */
    struct secPrivUserInfo  *Info;
};

/* Tasks that tried tickets that did not exist */
struct secRedeemFail
{
    struct MinNode          Node;
    struct Task             *Task;
    ULONG                   TaskUnique;
    ULONG                   Fails;
    ULONG                   Until;
};

struct AuthState
{
    struct timerequest      *TimerReq;      /* up-time and the hold-back wake-up */
    struct MsgPort          *TimerPort;
    BOOL                    TimerPending;
    struct Library          *EntropyBase;
    struct secThrottle      Throttle[secAUTH_THROTTLEBUCKETS];
    ULONG                   HashKey[2];
    struct MinList          Tallies;
    ULONG                   NumTallies;
    ULONG                   LastTallyExpiry;
    struct MinList          Tickets;
    ULONG                   NumTickets;
    struct MinList          RedeemFails;
    struct MinList          Held;           /* requests waiting for their user's delay */
};

/* Up-time in seconds */
static ULONG NowSeconds(struct AuthState *st)
{
    struct Device *TimerBase = st->TimerReq->tr_node.io_Device;
    struct timeval tv;

    GetUpTime(&tv);
    return (ULONG)tv.tv_secs;
}

static ULONG TaskUnique(struct Task *task)
{
    return task ? (ULONG)GetETaskID(task) : 0;
}

/* ------------------------------------------------------------------------ */
/* Per-user throttle                                                        */

static struct secThrottle *ThrottleFor(struct SecurityBase *secBase, struct AuthState *st, CONST_STRPTR userid)
{
    ULONG h = st->HashKey[0];
    CONST_STRPTR p;

    /* FNV-1a over the key and the case-folded name */
    for (p = userid; *p; p++)
    {
        h ^= (UBYTE)ToLower(*p);
        h *= 16777619UL;
    }
    h ^= st->HashKey[1];
    h *= 16777619UL;
    return &st->Throttle[h % secAUTH_THROTTLEBUCKETS];
}

static void ThrottleFail(struct SecurityBase *secBase, struct secThrottle *t, ULONG now)
{
    ULONG delay, max = secBase->Config.LoginDelayMax;

    if (now - t->LastFail > secAUTH_THROTTLEDECAY)
        t->Fails = 0;
    if (t->Fails < 31)
        t->Fails++;
    t->LastFail = now;
    if (!max)
        return;
    delay = (t->Fails > 16) ? max : (1UL << (t->Fails - 1));
    if (delay > max)
        delay = max;
    t->NextAllowed = now + delay;
}

/* ------------------------------------------------------------------------ */
/* Remote lock-out                                                          */

static void DropTally(struct AuthState *st, struct secTally *t)
{
    Remove((struct Node *)t);
    Free(t, sizeof(struct secTally));
    st->NumTallies--;
}

static struct secTally *FindTally(struct AuthState *st, CONST_STRPTR userid, CONST_STRPTR host, BOOL create, ULONG now)
{
    struct secTally *t, *victim = NULL;

    ForeachNode(&st->Tallies, t)
    {
        if (!strcmp(t->UserID, userid) && !strcmp(t->Host, host))
            return t;
    }
    if (!create)
        return NULL;
    if (st->NumTallies >= secAUTH_TALLYMAX)
    {
        /* Make room: the oldest record that is not locked, else the oldest */
        ForeachNode(&st->Tallies, t)
        {
            if ((LONG)(t->LockedUntil - now) <= 0 && (!victim || (LONG)(t->LastFail - victim->LastFail) < 0))
                victim = t;
        }
        if (!victim)
            victim = (struct secTally *)GetHead(&st->Tallies);
        DropTally(st, victim);
    }
    if (!(t = MAlloc(sizeof(struct secTally))))
        return NULL;
    memset(t, 0, sizeof(*t));
    strncpy(t->UserID, userid, sizeof(t->UserID) - 1);
    strncpy(t->Host, host, sizeof(t->Host) - 1);
    AddTail((struct List *)&st->Tallies, (struct Node *)t);
    st->NumTallies++;
    return t;
}

/* Forget records that have been quiet for a day (at most once a minute) */
static void ExpireTallies(struct AuthState *st, ULONG now)
{
    struct secTally *t, *next;

    if (now - st->LastTallyExpiry < 60)
        return;
    st->LastTallyExpiry = now;
    ForeachNodeSafe(&st->Tallies, t, next)
    {
        if (now - t->LastFail > 86400UL && (LONG)(t->LockedUntil - now) <= 0)
            DropTally(st, t);
    }
}

/* ------------------------------------------------------------------------ */
/* Tickets                                                                  */

static void FreeTicket(struct SecurityBase *secBase, struct AuthState *st, struct secAuthTicket *t)
{
    Remove((struct Node *)t);
    if (t->Info)
        secFreeUserInfo((struct secUserInfo *)t->Info);
    memset(&t->Id, 0, sizeof(t->Id));
    Free(t, sizeof(struct secAuthTicket));
    st->NumTickets--;
    secBase->AuthTickets = st->NumTickets;
}

static void ExpireTickets(struct SecurityBase *secBase, struct AuthState *st, ULONG now)
{
    struct secAuthTicket *t, *next;

    ForeachNodeSafe(&st->Tickets, t, next)
    {
        if ((LONG)(t->Expires - now) <= 0)
            FreeTicket(secBase, st, t);
    }
}

static void FreeAllTickets(struct SecurityBase *secBase, struct AuthState *st)
{
    while (!IsMinListEmpty(&st->Tickets))
        FreeTicket(secBase, st, (struct secAuthTicket *)GetHead(&st->Tickets));
}

/* Compare without an early exit */
static BOOL SameTicket(const struct secLoginTicket *a, const struct secLoginTicket *b)
{
    UBYTE diff = 0;
    int i;

    for (i = 0; i < secTICKETSIZE; i++)
        diff |= a->Id[i] ^ b->Id[i];
    return diff == 0;
}

static BOOL IssueTicket(struct SecurityBase *secBase, struct AuthState *st, struct Task *task, struct secPrivUserInfo *src, ULONG now, struct secLoginTicket *out)
{
    struct Library *EntropyBase = st->EntropyBase;
    struct secAuthTicket *t;
    struct secPrivUserInfo *copy;
    ULONG mine = 0, unique = TaskUnique(task);

    if (!EntropyBase)
        return FALSE;
    ExpireTickets(secBase, st, now);
    ForeachNode(&st->Tickets, t)
    {
        if (t->Task == task && t->TaskUnique == unique)
            mine++;
    }
    if (mine >= secAUTH_TICKETSPERTASK || st->NumTickets >= secAUTH_TICKETSMAX)
        return FALSE;

    if (!(copy = (struct secPrivUserInfo *)secAllocUserInfo()))
        return FALSE;
    CopyMem(src, copy, sizeof(struct secPrivUserInfo));
    copy->Pattern = NULL;
    copy->Pub.SecGroups = NULL;
    copy->Pub.NumSecGroups = 0;
    if (src->Pub.NumSecGroups && (copy->Pub.SecGroups = MAlloc(src->Pub.NumSecGroups * sizeof(UWORD))))
    {
        CopyMem(src->Pub.SecGroups, copy->Pub.SecGroups, src->Pub.NumSecGroups * sizeof(UWORD));
        copy->Pub.NumSecGroups = src->Pub.NumSecGroups;
    }
    if (!(t = MAlloc(sizeof(struct secAuthTicket))))
    {
        secFreeUserInfo((struct secUserInfo *)copy);
        return FALSE;
    }
    if (GetEntropy(t->Id.Id, secTICKETSIZE) != secTICKETSIZE)
    {
        Free(t, sizeof(struct secAuthTicket));
        secFreeUserInfo((struct secUserInfo *)copy);
        return FALSE;
    }
    t->Expires = now + secAUTH_TICKETLIFE;
    t->Task = task;
    t->TaskUnique = unique;
    t->Info = copy;
    AddTail((struct List *)&st->Tickets, (struct Node *)t);
    st->NumTickets++;
    secBase->AuthTickets = st->NumTickets;
    CopyMem(&t->Id, out, sizeof(*out));
    return TRUE;
}

static struct secRedeemFail *FindRedeemFail(struct AuthState *st, struct Task *task, ULONG unique)
{
    struct secRedeemFail *f;

    ForeachNode(&st->RedeemFails, f)
    {
        if (f->Task == task && f->TaskUnique == unique)
            return f;
    }
    return NULL;
}

static struct secPrivUserInfo *TakeTicket(struct SecurityBase *secBase, struct AuthState *st, const struct secLoginTicket *id, struct Task *task, ULONG now)
{
    struct secAuthTicket *t;
    struct secPrivUserInfo *info;
    struct secRedeemFail *f;
    ULONG unique = TaskUnique(task);

    if ((f = FindRedeemFail(st, task, unique)) && f->Fails >= secAUTH_REDEEMFAILS)
    {
        if ((LONG)(f->Until - now) > 0)
            return NULL;
        f->Fails = 0;
    }

    ExpireTickets(secBase, st, now);
    ForeachNode(&st->Tickets, t)
    {
        if (SameTicket(&t->Id, id) && t->Task == task && t->TaskUnique == unique)
        {
            info = t->Info;
            t->Info = NULL;
            FreeTicket(secBase, st, t);
            if (f)
            {
                Remove((struct Node *)f);
                Free(f, sizeof(*f));
            }
            return info;
        }
    }

    /* Not issued to this task (or gone): count it */
    if (!f && (f = MAlloc(sizeof(*f))))
    {
        memset(f, 0, sizeof(*f));
        f->Task = task;
        f->TaskUnique = unique;
        AddTail((struct List *)&st->RedeemFails, (struct Node *)f);
    }
    if (f)
    {
        f->Fails++;
        f->Until = now + secBase->Config.LockTime;
    }
    {
        SIPTR args[1] = { (SIPTR)(task && task->tc_Node.ln_Name ? task->tc_Node.ln_Name : "?") };
        VLogF(secBase, LOGF_Flag_Type_Warn, SECLOG_SUB_AUTH, "invalid login ticket presented by '%s'", args);
    }
    return NULL;
}

static void DiscardTicket(struct SecurityBase *secBase, struct AuthState *st, const struct secLoginTicket *id, struct Task *task)
{
    struct secAuthTicket *t;
    ULONG unique = TaskUnique(task);

    ForeachNode(&st->Tickets, t)
    {
        if (SameTicket(&t->Id, id) && t->Task == task && t->TaskUnique == unique)
        {
            FreeTicket(secBase, st, t);
            return;
        }
    }
}

static void TaskEnded(struct SecurityBase *secBase, struct AuthState *st, struct Task *task, ULONG unique)
{
    struct secAuthTicket *t, *next;
    struct secRedeemFail *f;

    ForeachNodeSafe(&st->Tickets, t, next)
    {
        if (t->Task == task && t->TaskUnique == unique)
            FreeTicket(secBase, st, t);
    }
    if ((f = FindRedeemFail(st, task, unique)))
    {
        Remove((struct Node *)f);
        Free(f, sizeof(*f));
    }
}

/* ------------------------------------------------------------------------ */
/* Verification                                                             */

/* Is the stored user ID usable for the Envoy form of the hash? ACrypt()
 * only looks at the first 12 characters of the name. */
static BOOL LowerCaseID(struct SecurityBase *secBase, CONST_STRPTR id)
{
    int i;

    for (i = 0; i < 12 && id[i]; i++)
    {
        if (id[i] != (char)ToLower(id[i]))
            return FALSE;
    }
    return TRUE;
}

/* Has this request's user still to wait? Returns the time, 0 if not */
static ULONG HoldUntil(struct SecurityBase *secBase, struct AuthState *st, struct secAPacket *pkt, ULONG now)
{
    struct secThrottle *t;

    if (pkt->Type != secAAction_Verify || !pkt->UserID || !pkt->UserID[0])
        return 0;
    t = ThrottleFor(secBase, st, pkt->UserID);
    return ((LONG)(t->NextAllowed - now) > 0) ? t->NextAllowed : 0;
}

/*
 * A user with registered hardware tokens types a code from one of them
 * after the password: find the token whose code ends what was typed (its
 * scheme's plugin recognises it), have the plugin check the code, refuse
 * a replay (the counter must grow) and remember the new counter.
 * *pwdpart receives the password part (freed by the caller with
 * FreeV()). FALSE if no token accepted the code.
 */
static BOOL CheckKeyCode(struct SecurityBase *secBase, struct secAPacket *pkt, struct secKeyRec *keys, STRPTR *pwdpart)
{
    struct secKeyRec *rec;
    ULONG toklen = strlen(pkt->Token), counter = 0, codelen = 0;
    SIPTR args[2] = { (SIPTR)pkt->UserID, 0 };
    BOOL noplugin = FALSE;

    *pwdpart = NULL;
    for (rec = keys; rec; rec = rec->Next)
    {
        ULONG r;

        if (rec->Type != secKEY_OTP)
            continue;
        r = KeyMatchCode(secBase, rec, pkt->Token, &codelen);
        if (r == secpiNOTSUPP)
        {
            noplugin = TRUE;
            continue;
        }
        if (r != secpiTRUE || codelen == 0 || codelen > toklen)
            continue;

        /* the password is what comes before the code */
        if ((*pwdpart = MAllocV(toklen - codelen + 1)))
        {
            CopyMem((APTR)pkt->Token, *pwdpart, toklen - codelen);
            (*pwdpart)[toklen - codelen] = '\0';
        }
        r = KeyVerifyCode(secBase, rec, pkt->Token + toklen - codelen, &counter);
        if (r != secpiTRUE)
            return FALSE;
        if (counter <= rec->Counter)
        {
            args[1] = (SIPTR)rec->Label;
            VLogF(secBase, LOGF_Flag_Type_Warn, SECLOG_SUB_AUTH, "'%s': a code of token '%s' was used again (replay)", args);
            return FALSE;
        }
        if (!SendServerPacket(secBase, secSAction_KeyCounter, (SIPTR)pkt->UserID, (SIPTR)rec->PublicID, (SIPTR)counter, 0))
        {
            VLogF(secBase, LOGF_Flag_Type_Error, SECLOG_SUB_AUTH, "'%s': the token counter could not be saved; the code is refused", args);
            return FALSE;
        }
        return TRUE;
    }
    if (noplugin)
        VLogF(secBase, LOGF_Flag_Type_Error, SECLOG_SUB_AUTH, "'%s' has a hardware token whose scheme has no plugin loaded", args);
    return FALSE;
}

static LONG Verify(struct SecurityBase *secBase, struct AuthState *st, struct secAPacket *pkt, struct Task *client)
{
    char hash[secHASHBUFSIZE];
    struct secPrivUserInfo *info;
    struct secTally *tally = NULL;
    CONST_STRPTR host = pkt->RemoteHost ? pkt->RemoteHost : (CONST_STRPTR)"";
    CONST_STRPTR service = pkt->Service ? pkt->Service : (CONST_STRPTR)"";
    ULONG now = NowSeconds(st);
    UWORD from = client ? (UWORD)(GetTaskOwner(secBase, client) >> 16) : secNOBODY_UID;
    LONG res = secVERIFY_FAILED;
    BOOL found, ok = FALSE;

    if (!secBase->Configured)
        return secVERIFY_UNAVAILABLE;
    if (!pkt->UserID || !pkt->UserID[0] || !pkt->Token)
        return secVERIFY_BADARGS;

    /* Token forms are checked before the user is looked up, so that the
     * answer says nothing about whether the user exists */
    switch (pkt->TokenType)
    {
    case secHASH_NONE:
        break;
    case secHASH_ACRYPT_LC:
        if (!(secBase->Config.Flags & secCFGF_HashedLogin))
            return secVERIFY_NOTSUPPORTED;
        break;
    default:
        return secVERIFY_NOTSUPPORTED;
    }

    if (pkt->RemoteHost)
    {
        ExpireTallies(st, now);
        if ((tally = FindTally(st, pkt->UserID, host, FALSE, now)) && (LONG)(tally->LockedUntil - now) > 0)
            return secVERIFY_LOCKED;
    }

    if (!(info = (struct secPrivUserInfo *)secAllocUserInfo()))
        return secVERIFY_BUSY;

    memset(hash, 0, sizeof(hash));
    found = (BOOL)SendServerPacket(secBase, secSAction_GetUserHash, (SIPTR)pkt->UserID, (SIPTR)hash, (SIPTR)info, 0);

    if (found)
    {
        if (pkt->TokenType == secHASH_NONE)
        {
            struct secKeyRec *keys = NULL;
            STRPTR pwdpart = NULL;
            CONST_STRPTR token = pkt->Token;
            BOOL keyok = TRUE;

            /* registered hardware tokens: the typed token must end with a code from one */
            if (secBase->Config.Flags & secCFGF_KeyLogin)
                SendServerPacket(secBase, secSAction_GetUserKeys, (SIPTR)pkt->UserID, (SIPTR)&keys, 0, 0);
            if (keys)
            {
                keyok = CheckKeyCode(secBase, pkt, keys, &pwdpart);
                if (pwdpart)
                    token = pwdpart;
            }
            else if ((secBase->Config.Flags & secCFGF_KeyRequired) && !pkt->RemoteHost)
            {
                SIPTR args[1] = { (SIPTR)pkt->UserID };
                VLogF(secBase, LOGF_Flag_Type_Warn, SECLOG_SUB_AUTH, "'%s' has no hardware token, and KEYREQUIRED is set", args);
                keyok = FALSE;
            }
            /* An empty password from the network is never accepted */
            if (!(pkt->RemoteHost && !token[0]))
                ok = verifypass(secBase, info->Pub.UserID, hash, token);
            ok = ok && keyok;
            if (pwdpart)
            {
                memset(pwdpart, 0, strlen(pwdpart));
                FreeV(pwdpart);
            }
            FreeKeys(keys);
        }
        else if (strlen(hash) == secACRYPT_LEN && LowerCaseID(secBase, info->Pub.UserID))
            ok = !strcmp(hash, pkt->Token);
    }
    else if (pkt->TokenType == secHASH_NONE)
    {
        /* Spend the time of a real comparison */
        verifypass(secBase, pkt->UserID, "AAAAAAAAAAA", pkt->Token);
    }
    memset(hash, 0, sizeof(hash));
    D(bug(DEBUG_NAME_STR " %s: user '%s' type %lu service '%s' host '%s': found %d ok %d\n", __func__, pkt->UserID, (unsigned long)pkt->TokenType, service, host, found, ok);)

    if (ok)
    {
        res = secVERIFY_OK;
        ThrottleFor(secBase, st, pkt->UserID)->Fails = 0;
        ThrottleFor(secBase, st, pkt->UserID)->NextAllowed = 0;
        if (tally)
            DropTally(st, tally);
        if (pkt->Info)
        {
            /* The caller's secUserInfo: public part, own copy of the groups */
            struct secUserInfo *dst = &pkt->Info->Pub;
            UWORD *groups = dst->SecGroups;
            UWORD ngroups = dst->NumSecGroups;

            CopyMem(&info->Pub, dst, sizeof(struct secUserInfo));
            dst->SecGroups = NULL;
            dst->NumSecGroups = 0;
            if (groups)
                Free(groups, ngroups * sizeof(UWORD));
            if (info->Pub.NumSecGroups && (dst->SecGroups = MAlloc(info->Pub.NumSecGroups * sizeof(UWORD))))
            {
                CopyMem(info->Pub.SecGroups, dst->SecGroups, info->Pub.NumSecGroups * sizeof(UWORD));
                dst->NumSecGroups = info->Pub.NumSecGroups;
            }
            pkt->Info->Password = info->Password;
        }
        if (pkt->WantTicket && client)
        {
            if (!IssueTicket(secBase, st, client, info, now, &pkt->Ticket))
                memset(&pkt->Ticket, 0, sizeof(pkt->Ticket));
        }
    }
    else
    {
        struct secThrottle *t = ThrottleFor(secBase, st, pkt->UserID);

        ThrottleFail(secBase, t, now);
        strncpy(t->UserID, pkt->UserID, sizeof(t->UserID) - 1);
        if (pkt->RemoteHost && (tally = FindTally(st, pkt->UserID, host, TRUE, now)))
        {
            tally->Fails++;
            tally->LastFail = now;
            if (secBase->Config.MaxTries && tally->Fails >= secBase->Config.MaxTries)
            {
                tally->LockedUntil = now + secBase->Config.LockTime;
                tally->Fails = 0;
            }
        }
        CallMonitors(secBase, secTrgB_LoginFail, from, 0, pkt->UserID);
    }

    if (!pkt->NoLog &&
        ((ok && (secBase->Config.LogFlags & secLogF_Login)) ||
         (!ok && (secBase->Config.LogFlags & secLogF_LoginFail))))
    {
        SIPTR args[3];
        args[0] = (SIPTR)pkt->UserID;
        args[1] = (SIPTR)service;
        args[2] = (SIPTR)(pkt->RemoteHost ? host : (CONST_STRPTR)"local");
        VLogF(secBase, ok ? LOGF_Flag_Type_Verbose : LOGF_Flag_Type_Warn, SECLOG_SUB_AUTH,
              ok ? "verified '%s' for %s from %s" : "verification of '%s' for %s from %s failed", args);
    }

    secFreeUserInfo((struct secUserInfo *)info);
    return res;
}

/* ------------------------------------------------------------------------ */
/* secAuthControlA()                                                        */

static void Control(struct SecurityBase *secBase, struct AuthState *st, struct secAPacket *pkt)
{
    ULONG now = NowSeconds(st), n = 0, i;
    struct secTally *t, *next;

    if (pkt->Clear)
    {
        for (i = 0; i < secAUTH_THROTTLEBUCKETS; i++)
        {
            struct secThrottle *b = &st->Throttle[i];
            if (pkt->RemoteHost && !pkt->UserID)
                break;
            if (!pkt->UserID || b == ThrottleFor(secBase, st, pkt->UserID))
                memset(b, 0, sizeof(*b));
        }
        ForeachNodeSafe(&st->Tallies, t, next)
        {
            if ((!pkt->UserID || !Stricmp(t->UserID, pkt->UserID)) &&
                (!pkt->RemoteHost || !Stricmp(t->Host, pkt->RemoteHost)))
                DropTally(st, t);
        }
        {
            SIPTR args[2] = { (SIPTR)(pkt->UserID ? pkt->UserID : (CONST_STRPTR)"*"),
                              (SIPTR)(pkt->RemoteHost ? pkt->RemoteHost : (CONST_STRPTR)"*") };
            VLogF(secBase, LOGF_Flag_Type_Information, SECLOG_SUB_AUTH, "failure records cleared for user %s, host %s", args);
        }
        return;
    }

    for (i = 0; i < secAUTH_THROTTLEBUCKETS; i++)
    {
        struct secThrottle *b = &st->Throttle[i];

        if (!b->Fails || now - b->LastFail > secAUTH_THROTTLEDECAY)
            continue;
        if (pkt->List && n < pkt->ListSize)
        {
            struct secAuthRecord *r = &pkt->List[n];
            memset(r, 0, sizeof(*r));
            r->Kind = secAUTHREC_DELAY;
            r->Failures = b->Fails;
            r->Seconds = ((LONG)(b->NextAllowed - now) > 0) ? b->NextAllowed - now : 0;
            strncpy(r->UserID, b->UserID, sizeof(r->UserID) - 1);
        }
        n++;
    }
    ForeachNode(&st->Tallies, t)
    {
        if (pkt->List && n < pkt->ListSize)
        {
            struct secAuthRecord *r = &pkt->List[n];
            memset(r, 0, sizeof(*r));
            r->Kind = secAUTHREC_LOCKOUT;
            r->Failures = t->Fails;
            r->Seconds = ((LONG)(t->LockedUntil - now) > 0) ? t->LockedUntil - now : 0;
            strncpy(r->UserID, t->UserID, sizeof(r->UserID) - 1);
            strncpy(r->Source, t->Host, sizeof(r->Source) - 1);
        }
        n++;
    }
    pkt->Count = n;
}

/* ------------------------------------------------------------------------ */
/* The process                                                              */

static void Answer(struct SecurityBase *secBase, struct secAPacket *pkt)
{
    Forbid();
    secBase->AuthPending--;
    Permit();
    ReplyMsg((struct Message *)pkt);
}

/* Wake up for the earliest held request */
static void ArmTimer(struct AuthState *st, ULONG now)
{
    struct secAPacket *pkt;
    ULONG due = 0;
    BOOL any = FALSE;

    if (st->TimerPending)
    {
        AbortIO((struct IORequest *)st->TimerReq);
        WaitIO((struct IORequest *)st->TimerReq);
        st->TimerPending = FALSE;
    }
    ForeachNode(&st->Held, pkt)
    {
        ULONG when = (ULONG)(IPTR)pkt->Msg.mn_Node.ln_Name;
        if (!any || (LONG)(when - due) < 0)
            due = when;
        any = TRUE;
    }
    if (!any)
        return;
    st->TimerReq->tr_node.io_Command = TR_ADDREQUEST;
    st->TimerReq->tr_time.tv_secs = ((LONG)(due - now) > 0) ? due - now : 0;
    st->TimerReq->tr_time.tv_micro = ((LONG)(due - now) > 0) ? 0 : 1000;
    SendIO((struct IORequest *)st->TimerReq);
    st->TimerPending = TRUE;
}

/* Close a login's pam.library transaction. The task that logged in may be
 * gone: nobody is asked anything and nothing is printed. */
static void CloseSession(struct SecurityBase *secBase, APTR handle)
{
    struct Library *PamBase = secBase->sec_PamBase;

    if (!PamBase || !handle)
        return;
    PamSetItem(handle, PAM_CONV, NULL);
    PamSetItem(handle, PAM_AROS_INPUT, NULL);
    PamSetItem(handle, PAM_AROS_OUTPUT, NULL);
    PamCloseSession(handle, PAM_SILENT);
    PamEnd(handle, PAM_SUCCESS);
}

static BOOL Handle(struct SecurityBase *secBase, struct AuthState *st, struct secAPacket *pkt)
{
    struct Task *client = pkt->Msg.mn_ReplyPort ? pkt->Msg.mn_ReplyPort->mp_SigTask : NULL;
    BOOL quit = FALSE;

    switch (pkt->Type)
    {
    case secAAction_Quit:
        quit = TRUE;
        pkt->Res = TRUE;
        break;

    case secAAction_Verify:
        pkt->Res = Verify(secBase, st, pkt, client);
        break;

    case secAAction_Redeem:
        pkt->Info = TakeTicket(secBase, st, &pkt->Ticket, client, NowSeconds(st));
        pkt->Res = (pkt->Info != NULL);
        break;

    case secAAction_Discard:
        DiscardTicket(secBase, st, &pkt->Ticket, client);
        pkt->Res = TRUE;
        break;

    case secAAction_Control:
        Control(secBase, st, pkt);
        pkt->Res = TRUE;
        break;

    case secAAction_KeyAdmin:
        /* a user changes their own keys: their ticket proves the password,
         * and is used up; the server does the rest */
        pkt->Res = secKAERR_NOTALLOWED;
        if (pkt->KeyReq && pkt->KeyReq->HasTicket && client)
        {
            struct secPrivUserInfo *info = TakeTicket(secBase, st, &pkt->Ticket, client, NowSeconds(st));

            if (info)
            {
                pkt->KeyReq->Verified = (pkt->KeyReq->UserID && !strcmp(info->Pub.UserID, pkt->KeyReq->UserID));
                pkt->KeyReq->CallerUid = info->Pub.uid;
                secFreeUserInfo(&info->Pub);
                if (pkt->KeyReq->Verified)
                    pkt->Res = (LONG)SendServerPacket(secBase, secSAction_KeyAdmin, (SIPTR)pkt->KeyReq, 0, 0, 0);
            }
        }
        break;

    case secAAction_TaskGone:
        TaskEnded(secBase, st, pkt->Task, pkt->TaskUnique);
        FreeVec(pkt);
        return FALSE;

    case secAAction_EndSession:
        CloseSession(secBase, pkt->PamHandle);
        FreeVec(pkt);
        return FALSE;

    default:
        pkt->Res = 0;
        break;
    }
    Answer(secBase, pkt);
    return quit;
}

static void AuthProcess(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct SecurityBase *secBase = (struct SecurityBase *)me->pr_Task.tc_UserData;
    struct DosPacket *spkt;
    struct MsgPort *port = NULL;
    struct secAPacket *pkt, *next;
    struct AuthState *st;
    BOOL quit = FALSE;
    ULONG now;

    D(bug(DEBUG_NAME_STR " %s: starting\n", __func__);)

    WaitPort(&me->pr_MsgPort);
    spkt = (struct DosPacket *)((struct Message *)GetMsg(&me->pr_MsgPort))->mn_Node.ln_Name;

    /* The state is large (the throttle table): not on the stack */
    if (!(st = AllocVec(sizeof(struct AuthState), MEMF_PUBLIC | MEMF_CLEAR)) ||
        !(port = CreateMsgPort()) ||
        !(st->TimerPort = CreateMsgPort()) ||
        !(st->TimerReq = (struct timerequest *)CreateIORequest(st->TimerPort, sizeof(struct timerequest))) ||
        OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)st->TimerReq, 0))
    {
        if (st)
        {
            if (st->TimerReq)
            {
                st->TimerReq->tr_node.io_Device = NULL;
                DeleteIORequest((struct IORequest *)st->TimerReq);
            }
            if (st->TimerPort)
                DeleteMsgPort(st->TimerPort);
            FreeVec(st);
        }
        if (port)
            DeleteMsgPort(port);
        Forbid();
        secBase->Auth = NULL;
        ReplyPkt(spkt, DOSFALSE, 0);
        return;
    }
    NEWLIST(&st->Tallies);
    NEWLIST(&st->Tickets);
    NEWLIST(&st->RedeemFails);
    NEWLIST(&st->Held);

    /* Tickets and the throttle key need real randomness; without the
     * resource there are no tickets and the key is merely unusual */
    if ((st->EntropyBase = OpenResource(ENTROPYNAME)))
    {
        struct Library *EntropyBase = st->EntropyBase;
        if (GetEntropy(st->HashKey, sizeof(st->HashKey)) != sizeof(st->HashKey))
            st->EntropyBase = NULL;
    }
    if (!st->EntropyBase)
    {
        st->HashKey[0] = 2166136261UL ^ (ULONG)(IPTR)st;
        st->HashKey[1] = NowSeconds(st) ^ (ULONG)(IPTR)me;
    }

    secBase->AuthPort = port;
    ReplyPkt(spkt, DOSTRUE, 0);

    do
    {
        Wait((1UL << port->mp_SigBit) | (1UL << st->TimerPort->mp_SigBit));

        if (st->TimerPending && CheckIO((struct IORequest *)st->TimerReq))
        {
            WaitIO((struct IORequest *)st->TimerReq);
            st->TimerPending = FALSE;
        }

        /* Held requests whose delay has passed */
        now = NowSeconds(st);
        ForeachNodeSafe(&st->Held, pkt, next)
        {
            if (quit)
                break;
            if ((LONG)((ULONG)(IPTR)pkt->Msg.mn_Node.ln_Name - now) <= 0)
            {
                ULONG until;

                Remove((struct Node *)pkt);
                /* another failure may have pushed it further out */
                if ((until = HoldUntil(secBase, st, pkt, now)))
                {
                    pkt->Msg.mn_Node.ln_Name = (char *)(IPTR)until;
                    AddTail((struct List *)&st->Held, (struct Node *)pkt);
                }
                else
                    quit = Handle(secBase, st, pkt);
            }
        }

        while (!quit && (pkt = (struct secAPacket *)GetMsg(port)))
        {
            ULONG until;

            now = NowSeconds(st);
            if ((until = HoldUntil(secBase, st, pkt, now)))
            {
                pkt->Msg.mn_Node.ln_Name = (char *)(IPTR)until;
                AddTail((struct List *)&st->Held, (struct Node *)pkt);
                continue;
            }
            quit = Handle(secBase, st, pkt);
        }
        if (!quit)
            ArmTimer(st, NowSeconds(st));
    } while (!quit);

    Forbid();
    secBase->AuthPort = NULL;
    Permit();

    if (st->TimerPending)
    {
        AbortIO((struct IORequest *)st->TimerReq);
        WaitIO((struct IORequest *)st->TimerReq);
    }
    while ((pkt = (struct secAPacket *)RemHead((struct List *)&st->Held)))
    {
        pkt->Res = secVERIFY_UNAVAILABLE;
        Answer(secBase, pkt);
    }
    while ((pkt = (struct secAPacket *)GetMsg(port)))
    {
        if (pkt->Type == secAAction_TaskGone || pkt->Type == secAAction_EndSession)
        {
            if (pkt->Type == secAAction_EndSession)
                CloseSession(secBase, pkt->PamHandle);
            FreeVec(pkt);
            continue;
        }
        pkt->Res = (pkt->Type == secAAction_Verify) ? secVERIFY_UNAVAILABLE : 0;
        Answer(secBase, pkt);
    }

    FreeAllTickets(secBase, st);
    while (!IsMinListEmpty(&st->Tallies))
        DropTally(st, (struct secTally *)GetHead(&st->Tallies));
    while (!IsMinListEmpty(&st->RedeemFails))
    {
        struct MinNode *f = (struct MinNode *)RemHead((struct List *)&st->RedeemFails);
        Free(f, sizeof(struct secRedeemFail));
    }
    CloseDevice((struct IORequest *)st->TimerReq);
    DeleteIORequest((struct IORequest *)st->TimerReq);
    DeleteMsgPort(st->TimerPort);
    memset(st, 0, sizeof(*st));
    FreeVec(st);
    DeleteMsgPort(port);
    Forbid();
    secBase->Auth = NULL;
}

struct Process *CreateAuthServer(struct SecurityBase *secBase)
{
    struct TagItem tags[] =
    {
        { NP_Entry,     (IPTR)AuthProcess       },
        { NP_Name,      (IPTR)AUTHNAME          },
        { NP_Priority,  AUTHPRI                 },
        { NP_StackSize, AUTHSTACK               },
        { NP_UserData,  (IPTR)secBase           },
        { TAG_DONE,     0                       }
    };

    secBase->Auth = CreateNewProc(tags);
    D(bug(DEBUG_NAME_STR " %s: '" AUTHNAME "' @ 0x%p\n", __func__, secBase->Auth);)
    return secBase->Auth;
}

BOOL StartAuthServer(struct SecurityBase *secBase)
{
    struct Process *auth = secBase->Auth;

    if (!auth)
        return FALSE;
    if (!DoPkt(&auth->pr_MsgPort, ACTION_STARTUP, 0, 0, 0, 0, 0))
    {
        /* the process has cleared secBase->Auth and is gone */
        VLogF(secBase, LOGF_Flag_Type_Error, SECLOG_SUB_AUTH, "the authentication process could not start", NULL);
        return FALSE;
    }
    return TRUE;
}

/* Queue accounting per calling user (SendAuthPacket(), under Forbid()) */
static BOOL QueueEnter(struct SecurityBase *secBase, UWORD uid)
{
    int i, freeslot = -1;

    if (secBase->AuthPending >= secAUTH_QUEUEMAX)
        return FALSE;
    for (i = 0; i < secAUTH_QUEUEUIDS; i++)
    {
        if (secBase->AuthQueue[i].Count && secBase->AuthQueue[i].Uid == uid)
        {
            if (secBase->AuthQueue[i].Count >= secAUTH_QUEUEPERUID)
                return FALSE;
            secBase->AuthQueue[i].Count++;
            secBase->AuthPending++;
            return TRUE;
        }
        if (!secBase->AuthQueue[i].Count && freeslot < 0)
            freeslot = i;
    }
    if (freeslot < 0)
        return FALSE;
    secBase->AuthQueue[freeslot].Uid = uid;
    secBase->AuthQueue[freeslot].Count = 1;
    secBase->AuthPending++;
    return TRUE;
}

static void QueueLeave(struct SecurityBase *secBase, UWORD uid)
{
    int i;

    for (i = 0; i < secAUTH_QUEUEUIDS; i++)
    {
        if (secBase->AuthQueue[i].Count && secBase->AuthQueue[i].Uid == uid)
        {
            secBase->AuthQueue[i].Count--;
            return;
        }
    }
}

/*
 * Send a packet to the authentication process and wait for the reply.
 * Returns secVERIFY_UNAVAILABLE if it is not running, secVERIFY_BUSY if too
 * much is queued (in all, or for the calling user), otherwise pkt->Res.
 * The answer may come late: a request for a user name that has just
 * failed is held back by the process.
 */
LONG SendAuthPacket(struct SecurityBase *secBase, struct secAPacket *pkt)
{
    struct MsgPort *port;
    LONG res = secVERIFY_UNAVAILABLE;
    struct Task *me = FindTask(NULL);

    if (!secBase->AuthPort || me == (struct Task *)secBase->Auth)
        return res;

    pkt->CallerUid = (UWORD)(GetTaskOwner(secBase, me) >> 16);
    if ((port = CreateMsgPort()))
    {
        pkt->Msg.mn_ReplyPort = port;
        pkt->Msg.mn_Length = sizeof(struct secAPacket);

        Forbid();
        if (secBase->AuthPort && QueueEnter(secBase, pkt->CallerUid))
        {
            PutMsg(secBase->AuthPort, (struct Message *)pkt);
            Permit();
            do
            {
                WaitPort(port);
            } while (GetMsg(port) != (struct Message *)pkt);
            Forbid();
            QueueLeave(secBase, pkt->CallerUid);
            Permit();
            res = pkt->Res;
        }
        else
        {
            res = secBase->AuthPort ? secVERIFY_BUSY : secVERIFY_UNAVAILABLE;
            Permit();
        }
        DeleteMsgPort(port);
    }
    return res;
}

struct secPrivUserInfo *RedeemTicket(struct SecurityBase *secBase, const struct secLoginTicket *ticket)
{
    struct secAPacket pkt;

    memset(&pkt, 0, sizeof(pkt));
    pkt.Type = secAAction_Redeem;
    CopyMem((APTR)ticket, &pkt.Ticket, sizeof(pkt.Ticket));
    if (SendAuthPacket(secBase, &pkt) == TRUE)
        return pkt.Info;
    return NULL;
}

void AuthTaskGone(struct SecurityBase *secBase, struct Task *task)
{
    struct secAPacket *pkt;

    /* Only worth a message while tickets are outstanding */
    if (!secBase->AuthPort || !secBase->AuthTickets || task == (struct Task *)secBase->Auth)
        return;
    if ((pkt = AllocVec(sizeof(struct secAPacket), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        pkt->Type = secAAction_TaskGone;
        pkt->Task = task;
        pkt->TaskUnique = TaskUnique(task);
        pkt->Msg.mn_Length = sizeof(struct secAPacket);
        Forbid();
        if (secBase->AuthPort)
            PutMsg(secBase->AuthPort, (struct Message *)pkt);
        else
            FreeVec(pkt);
        Permit();
    }
}

void EndPamSession(struct SecurityBase *secBase, APTR pamhandle)
{
    struct secAPacket *pkt;

    if (!pamhandle)
        return;
    if ((pkt = AllocVec(sizeof(struct secAPacket), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        pkt->Type = secAAction_EndSession;
        pkt->PamHandle = pamhandle;
        pkt->Msg.mn_Length = sizeof(struct secAPacket);
        Forbid();
        if (secBase->AuthPort)
        {
            PutMsg(secBase->AuthPort, (struct Message *)pkt);
            pkt = NULL;
        }
        Permit();
        if (!pkt)
            return;
        FreeVec(pkt);
    }
    /* No authentication process (shutting down): the transaction is left
     * open and its memory is not given back */
    D(bug(DEBUG_NAME_STR " %s: could not close pam.library session %p\n", __func__, pamhandle);)
}
