/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library authentication process ("Security.auth").

          Answers secVerifyUserA(): is this token right for this user. The
          process is separate from Security.server so that a login attempt,
          however slow its hash or however many arrive from the network,
          never stalls the server that the filesystems depend on. The server
          stays the owner of the user database; the hash of one user is
          fetched from it with a private request that only this process may
          make (secSAction_GetUserHash), and the comparison, the failure
          tally, the log line and the monitors are handled here.

          A successful verification can also issue a one-time login ticket,
          which secLoginA() turns into a login for the same task without
          asking for a password again (RedeemTicket()).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#include <dos/dosextens.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_auth.h"
#include "security_server.h"
#include "security_task.h"
#include "security_monitor.h"
#include "security_memory.h"
#include "security_crypto.h"
#include "security_userinfo.h"

#define AUTHPRI                 (2)             /* below the server (4) */
#define AUTHSTACK               (AROS_STACKSIZE * 2)

/* One failure record per (user, remote host) */
struct secTally
{
    struct MinNode          Node;
    ULONG                   Fails;
    ULONG                   LockedUntil;    /* seconds, 0 = not locked */
    ULONG                   LastFail;
    char                    UserID[secUSERIDSIZE];
    char                    Host[64];
};

/* One outstanding login ticket */
struct secTicket
{
    struct MinNode          Node;
    ULONG                   Id;
    ULONG                   Expires;        /* seconds */
    struct Task             *Task;          /* the task it was issued to */
    struct secPrivUserInfo  *Info;
};

struct AuthState
{
    struct MinList          Tallies;
    struct MinList          Tickets;
    ULONG                   TicketSeq;
};

/* Seconds since 1978, good enough for time-outs */
static ULONG NowSeconds(struct SecurityBase *secBase)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return (ULONG)ds.ds_Days * 86400UL + (ULONG)ds.ds_Minute * 60UL + (ULONG)ds.ds_Tick / TICKS_PER_SECOND;
}

/* ------------------------------------------------------------------------ */
/* Failure tally                                                            */

static struct secTally *FindTally(struct AuthState *st, CONST_STRPTR userid, CONST_STRPTR host, BOOL create)
{
    struct secTally *t;

    ForeachNode(&st->Tallies, t)
    {
        if (!strcmp(t->UserID, userid) && !strcmp(t->Host, host))
            return t;
    }
    if (!create || !(t = MAlloc(sizeof(struct secTally))))
        return NULL;
    memset(t, 0, sizeof(*t));
    strncpy(t->UserID, userid, sizeof(t->UserID) - 1);
    strncpy(t->Host, host, sizeof(t->Host) - 1);
    AddTail((struct List *)&st->Tallies, (struct Node *)t);
    return t;
}

static void DropTally(struct secTally *t)
{
    Remove((struct Node *)t);
    Free(t, sizeof(struct secTally));
}

/* Forget records that have been quiet for a day */
static void ExpireTallies(struct AuthState *st, ULONG now)
{
    struct secTally *t, *next;

    ForeachNodeSafe(&st->Tallies, t, next)
    {
        if (now - t->LastFail > 86400UL && t->LockedUntil < now)
            DropTally(t);
    }
}

/* ------------------------------------------------------------------------ */
/* Tickets                                                                  */

static void ExpireTickets(struct SecurityBase *secBase, struct AuthState *st, ULONG now)
{
    struct secTicket *t, *next;

    ForeachNodeSafe(&st->Tickets, t, next)
    {
        if ((LONG)(t->Expires - now) < 0)
        {
            Remove((struct Node *)t);
            secFreeUserInfo((struct secUserInfo *)t->Info);
            Free(t, sizeof(struct secTicket));
        }
    }
}

static ULONG IssueTicket(struct SecurityBase *secBase, struct AuthState *st, struct Task *task, struct secPrivUserInfo *src, ULONG now)
{
    struct secTicket *t;
    struct secPrivUserInfo *copy;

    if (!(copy = (struct secPrivUserInfo *)secAllocUserInfo()))
        return 0;
    CopyMem(src, copy, sizeof(struct secPrivUserInfo));
    copy->Pattern = NULL;
    copy->Pub.SecGroups = NULL;
    copy->Pub.NumSecGroups = 0;
    if (src->Pub.NumSecGroups && (copy->Pub.SecGroups = MAlloc(src->Pub.NumSecGroups * sizeof(UWORD))))
    {
        CopyMem(src->Pub.SecGroups, copy->Pub.SecGroups, src->Pub.NumSecGroups * sizeof(UWORD));
        copy->Pub.NumSecGroups = src->Pub.NumSecGroups;
    }
    if (!(t = MAlloc(sizeof(struct secTicket))))
    {
        secFreeUserInfo((struct secUserInfo *)copy);
        return 0;
    }
    st->TicketSeq += 0x9E3779B9UL;
    t->Id = (st->TicketSeq ^ (ULONG)(IPTR)task ^ (now << 7)) | 1;
    t->Expires = now + secAUTH_TICKETLIFE;
    t->Task = task;
    t->Info = copy;
    AddTail((struct List *)&st->Tickets, (struct Node *)t);
    return t->Id;
}

static struct secPrivUserInfo *TakeTicket(struct SecurityBase *secBase, struct AuthState *st, ULONG id, struct Task *task, ULONG now)
{
    struct secTicket *t;
    struct secPrivUserInfo *info;

    ExpireTickets(secBase, st, now);
    ForeachNode(&st->Tickets, t)
    {
        if (t->Id == id)
        {
            if (t->Task != task)
                return NULL;
            Remove((struct Node *)t);
            info = t->Info;
            Free(t, sizeof(struct secTicket));
            return info;
        }
    }
    return NULL;
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

static LONG Verify(struct SecurityBase *secBase, struct AuthState *st, struct secAPacket *pkt, struct Task *client)
{
    char hash[secHASHBUFSIZE];
    struct secPrivUserInfo *info;
    struct secTally *tally;
    CONST_STRPTR host = pkt->RemoteHost ? pkt->RemoteHost : (CONST_STRPTR)"";
    CONST_STRPTR service = pkt->Service ? pkt->Service : (CONST_STRPTR)"";
    ULONG now = NowSeconds(secBase);
    UWORD from = client ? (UWORD)(GetTaskOwner(secBase, client) >> 16) : secNOBODY_UID;
    LONG res = secVERIFY_FAILED;
    BOOL found, ok = FALSE;

    if (!secBase->Configured)
        return secVERIFY_UNAVAILABLE;
    if (!pkt->UserID || !pkt->UserID[0] || !pkt->Token)
        return secVERIFY_BADARGS;

    ExpireTallies(st, now);
    if ((tally = FindTally(st, pkt->UserID, host, FALSE)) && tally->LockedUntil && (LONG)(tally->LockedUntil - now) > 0)
        return secVERIFY_LOCKED;

    if (!(info = (struct secPrivUserInfo *)secAllocUserInfo()))
        return secVERIFY_BUSY;

    memset(hash, 0, sizeof(hash));
    found = (BOOL)SendServerPacket(secBase, secSAction_GetUserHash, (SIPTR)pkt->UserID, (SIPTR)hash, (SIPTR)info, 0);

    if (found)
    {
        switch (pkt->TokenType)
        {
        case secHASH_NONE:
            ok = verifypass(secBase, info->Pub.UserID, hash, pkt->Token);
            break;

        case secHASH_ACRYPT_LC:
            if (!(secBase->Config.Flags & secCFGF_HashedLogin))
                res = secVERIFY_NOTSUPPORTED;
            else if (strlen(hash) == secACRYPT_LEN && LowerCaseID(secBase, info->Pub.UserID))
                ok = !strcmp(hash, pkt->Token);
            break;

        default:
            res = secVERIFY_NOTSUPPORTED;
            break;
        }
    }
    memset(hash, 0, sizeof(hash));
    D(bug(DEBUG_NAME_STR " %s: user '%s' type %lu service '%s' host '%s': found %d ok %d res %ld\n", __func__, pkt->UserID, (unsigned long)pkt->TokenType, service, host, found, ok, (long)res);)

    if (ok)
    {
        res = secVERIFY_OK;
        if (tally)
            DropTally(tally);
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
            pkt->Ticket = IssueTicket(secBase, st, client, info, now);
    }
    else if (res == secVERIFY_FAILED)
    {
        if ((tally = FindTally(st, pkt->UserID, host, TRUE)))
        {
            tally->Fails++;
            tally->LastFail = now;
            if (secBase->Config.MaxTries && tally->Fails >= secBase->Config.MaxTries)
            {
                tally->LockedUntil = now + secBase->Config.LockTime;
                tally->Fails = 0;
            }
        }
    }

    CallMonitors(secBase, ok ? secTrgB_Login : secTrgB_LoginFail, from, ok ? info->Pub.uid : 0, pkt->UserID);

    if ((ok && (secBase->Config.LogFlags & secLogF_Login)) ||
        (!ok && (secBase->Config.LogFlags & secLogF_LoginFail)))
    {
        SIPTR args[4];
        args[0] = (SIPTR)pkt->UserID;
        args[1] = (SIPTR)service;
        args[2] = (SIPTR)host;
        args[3] = (SIPTR)res;
        VLogF(secBase, ok ? "verified '%s' for %s from '%s'" : "verification of '%s' for %s from '%s' failed (%ld)", args);
    }

    secFreeUserInfo((struct secUserInfo *)info);
    return res;
}

/* ------------------------------------------------------------------------ */
/* The process                                                              */

static void AuthProcess(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct SecurityBase *secBase = (struct SecurityBase *)me->pr_Task.tc_UserData;
    struct DosPacket *spkt;
    struct MsgPort *port;
    struct secAPacket *pkt;
    struct AuthState st;
    BOOL quit = FALSE;

    D(bug(DEBUG_NAME_STR " %s: starting\n", __func__);)

    WaitPort(&me->pr_MsgPort);
    spkt = (struct DosPacket *)((struct Message *)GetMsg(&me->pr_MsgPort))->mn_Node.ln_Name;

    if (!(port = CreateMsgPort()))
    {
        ReplyPkt(spkt, DOSFALSE, 0);
        return;
    }
    NEWLIST(&st.Tallies);
    NEWLIST(&st.Tickets);
    st.TicketSeq = (ULONG)(IPTR)me ^ NowSeconds(secBase);

    secBase->AuthPort = port;
    ReplyPkt(spkt, DOSTRUE, 0);

    do
    {
        WaitPort(port);
        while (!quit && (pkt = (struct secAPacket *)GetMsg(port)))
        {
            struct Task *client = pkt->Msg.mn_ReplyPort ? pkt->Msg.mn_ReplyPort->mp_SigTask : NULL;

            switch (pkt->Type)
            {
            case secAAction_Quit:
                quit = TRUE;
                pkt->Res = TRUE;
                break;

            case secAAction_Verify:
                pkt->Res = Verify(secBase, &st, pkt, client);
                break;

            case secAAction_Redeem:
                pkt->Info = TakeTicket(secBase, &st, pkt->Ticket, client, NowSeconds(secBase));
                pkt->Res = (pkt->Info != NULL);
                break;

            default:
                pkt->Res = 0;
                break;
            }
            Forbid();
            secBase->AuthPending--;
            Permit();
            ReplyMsg((struct Message *)pkt);
        }
    } while (!quit);

    Forbid();
    secBase->AuthPort = NULL;
    Permit();

    while ((pkt = (struct secAPacket *)GetMsg(port)))
    {
        pkt->Res = 0;
        Forbid();
        secBase->AuthPending--;
        Permit();
        ReplyMsg((struct Message *)pkt);
    }

    ExpireTickets(secBase, &st, 0xFFFFFFFFUL);
    while (!IsMinListEmpty(&st.Tallies))
        DropTally((struct secTally *)GetHead(&st.Tallies));
    DeleteMsgPort(port);
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
    if (!secBase->Auth)
        return FALSE;
    return (BOOL)DoPkt(&secBase->Auth->pr_MsgPort, ACTION_STARTUP, 0, 0, 0, 0, 0);
}

/*
 * Send a packet to the authentication process and wait for the reply.
 * Returns secVERIFY_UNAVAILABLE if it is not running, secVERIFY_BUSY if too
 * much is queued, otherwise pkt->Res.
 */
LONG SendAuthPacket(struct SecurityBase *secBase, struct secAPacket *pkt)
{
    struct MsgPort *port;
    LONG res = secVERIFY_UNAVAILABLE;

    if (!secBase->AuthPort || FindTask(NULL) == (struct Task *)secBase->Auth)
        return res;

    if ((port = CreateMsgPort()))
    {
        pkt->Msg.mn_ReplyPort = port;
        pkt->Msg.mn_Length = sizeof(struct secAPacket);

        Forbid();
        if (secBase->AuthPort && secBase->AuthPending < secAUTH_QUEUEMAX)
        {
            secBase->AuthPending++;
            PutMsg(secBase->AuthPort, (struct Message *)pkt);
            Permit();
            do
            {
                WaitPort(port);
            } while (GetMsg(port) != (struct Message *)pkt);
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

struct secPrivUserInfo *RedeemTicket(struct SecurityBase *secBase, ULONG ticket)
{
    struct secAPacket pkt;

    memset(&pkt, 0, sizeof(pkt));
    pkt.Type = secAAction_Redeem;
    pkt.Ticket = ticket;
    if (SendAuthPacket(secBase, &pkt) == TRUE)
        return pkt.Info;
    return NULL;
}
