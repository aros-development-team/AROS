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

/* Configuration defaults (Security.config MAXTRIES / LOCKTIME) */
#define secAUTH_DEFMAXTRIES     (5)
#define secAUTH_DEFLOCKTIME     (300)

/* Requests queued at the authentication process before it answers BUSY */
#define secAUTH_QUEUEMAX        (16)

/* Lifetime of a login ticket, seconds */
#define secAUTH_TICKETLIFE      (60)

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
    CONST_STRPTR            RemoteHost;
    struct secPrivUserInfo  *Info;          /* filled on success if not NULL (Verify) /     */
                                            /* allocated and returned (Redeem)              */
    ULONG                   Ticket;         /* out (Verify, if wanted) / in (Redeem)        */
    BOOL                    WantTicket;
    LONG                    Res;            /* secVERIFY_#? (Verify), BOOL (others)         */
};

#define secAAction_Quit         0
#define secAAction_Verify       1           /* check a user's token, no side effect         */
#define secAAction_Redeem       2           /* turn a ticket into a user record             */

extern struct Process *CreateAuthServer(struct SecurityBase *secBase);
extern BOOL StartAuthServer(struct SecurityBase *secBase);
extern LONG SendAuthPacket(struct SecurityBase *secBase, struct secAPacket *pkt);

/* secLoginA(): the user a ticket was issued for (secAllocUserInfo()'d), or NULL */
extern struct secPrivUserInfo *RedeemTicket(struct SecurityBase *secBase, ULONG ticket);

#endif /* _SECURITY_AUTH_H */
