/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library authentication process ("Security.auth")
*/
#ifndef _SECURITY_AUTH_H
#define _SECURITY_AUTH_H

#include <exec/ports.h>
#include <libraries/security.h>

#define AUTHNAME                "Security.auth"

/* Size of the buffer the server copies a stored hash into */
#define secHASHBUFSIZE          (128)

/* Configuration defaults (Security.config MAXTRIES / LOCKTIME / LOGINDELAYMAX) */
#define secAUTH_DEFMAXTRIES     (5)
#define secAUTH_DEFLOCKTIME     (300)
#define secAUTH_DEFDELAYMAX     (30)
#define secAUTH_DEFKEYTIMEOUT   (30)
#define secAUTH_DEFKEYAPPID     "aros:login"
#define secAUTH_DEFKEYDEVICE    "fido.device"

/* Requests queued at the authentication process: in all, and per calling
 * user (tasks owned by nobody - login prompts - count as one user) */
#define secAUTH_QUEUEMAX        (32)
#define secAUTH_QUEUEPERUID     (4)
#define secAUTH_QUEUEUIDS       (8)

/* Lifetime of a login ticket, seconds (monotonic clock) */
#define secAUTH_TICKETLIFE      (15)
/* Outstanding tickets: per task and in all */
#define secAUTH_TICKETSPERTASK  (4)
#define secAUTH_TICKETSMAX      (256)
/* Failed redemptions a task may make before it is refused for LOCKTIME */
#define secAUTH_REDEEMFAILS     (3)

/* Per-user throttle: hashed buckets, so that every user name - known or
 * not - is treated alike and no record can be pushed out */
#define secAUTH_THROTTLEBUCKETS (256)
/* Failures older than this (seconds) no longer count towards the delay */
#define secAUTH_THROTTLEDECAY   (900)

/* Remote-source lock-out records */
#define secAUTH_TALLYMAX        (512)
#define secAUTH_HOSTMAX         (255)

struct SecurityBase;
struct secPrivUserInfo;

/*
 * Private Authentication Packet
 */
struct secAPacket
{
    struct Message          Msg;
    LONG                    Type;           /* secAAction_#?                                */
    CONST_STRPTR            UserID;
    CONST_STRPTR            Token;          /* clear password or pre-hashed form            */
    ULONG                   TokenType;      /* secHASH_#?                                   */
    CONST_STRPTR            Service;
    CONST_STRPTR            RemoteHost;     /* only from root, NULL otherwise               */
    struct secPrivUserInfo  *Info;          /* filled on success if not NULL (Verify) /     */
                                            /* allocated and returned (Redeem)              */
    struct secLoginTicket   Ticket;         /* out (Verify, if wanted) / in (Redeem,        */
                                            /* Discard)                                     */
    BOOL                    WantTicket;
    BOOL                    NoLog;          /* root asked for no log line                   */
    UWORD                   CallerUid;      /* queue accounting (SendAuthPacket)            */
    struct Task             *Task;          /* TaskGone: the task that went                 */
    ULONG                   TaskUnique;     /* TaskGone: its et_UniqueID                    */
    struct secAuthRecord    *List;          /* Control: records out                         */
    ULONG                   ListSize;
    ULONG                   Count;          /* Control: number of records                   */
    BOOL                    Clear;          /* Control: clear UserID / RemoteHost / all     */
    APTR                    PamHandle;      /* EndSession                                   */
    struct secKARequest     *KeyReq;        /* KeyAdmin: the request; Res = secKAERR_#?     */
    UBYTE                   *Challenge;     /* Verify: receives the challenge (KEYNEEDED)   */
    const UBYTE             *Assertion;     /* Verify: a token's answer (with Ticket)       */
    ULONG                   AssertionLen;
    CONST_STRPTR            KeyPublicID;    /* Verify: the token that answered              */
    struct secKeyInfo       *KeyList;       /* Verify: receives the challenge tokens        */
    ULONG                   KeyListSize;
    ULONG                   KeyCount;
    LONG                    Res;            /* secVERIFY_#? (Verify), BOOL (others)         */
};

#define secAAction_Quit         0
#define secAAction_Verify       1           /* check a user's token, no side effect         */
#define secAAction_Redeem       2           /* turn a ticket into a user record             */
#define secAAction_Discard      3           /* give an unused ticket back                   */
#define secAAction_TaskGone     4           /* a task ended: drop its tickets. Sent without */
                                            /* a reply port; the process frees the packet   */
#define secAAction_Control      5           /* secAuthControlA(): list / clear the records  */
#define secAAction_EndSession   6           /* close a pam.library session (PamHandle).     */
                                            /* Sent without a reply port, like TaskGone     */
#define secAAction_KeyAdmin     7           /* secKeyAdminA() by a user: redeem their       */
                                            /* ticket, then let the server change their keys*/

extern struct Process *CreateAuthServer(struct SecurityBase *secBase);
extern BOOL StartAuthServer(struct SecurityBase *secBase);
extern LONG SendAuthPacket(struct SecurityBase *secBase, struct secAPacket *pkt);

/* secLoginA(): the user a ticket was issued for (secAllocUserInfo()'d), or NULL */
extern struct secPrivUserInfo *RedeemTicket(struct SecurityBase *secBase, const struct secLoginTicket *ticket);

/* task.resource hook: a task ended (any context that may allocate memory) */
extern void AuthTaskGone(struct SecurityBase *secBase, struct Task *task);

/* A login session ended (any context): its pam.library transaction is
 * closed (PamCloseSession(), PamEnd()) by the authentication process */
extern void EndPamSession(struct SecurityBase *secBase, APTR pamhandle);

#endif /* _SECURITY_AUTH_H */
