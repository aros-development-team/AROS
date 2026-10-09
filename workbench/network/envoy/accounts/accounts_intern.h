/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: accounts.library - private definitions.

          The library is a client of the Envoy "Accounts Server" entity
          (re/spec/services-accounts.md §5, §6). Every public call goes
          through a transaction; the private Acc#?() calls take the server
          entity explicitly, the public ones use a link the library keeps
          per calling task (the entity that receives the replies belongs to
          the task that created it).
*/
#ifndef ACCOUNTS_INTERN_H
#define ACCOUNTS_INTERN_H

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <envoy/nipc.h>
#include <envoy/accounts.h>
#include <envoy/errors.h>
#include <aros/debug.h>

/* Wire layouts (§6.2): all multi-byte fields big-endian */
#define ACC_AUTHSIZE        64      /* struct AuthorityInfo as sent        */
#define ACC_USERSIZE        72      /* UserInfo (40) + password field (32) */
#define ACC_GROUPSIZE       40      /* GroupInfo                           */
#define ACC_NAMESIZE        32

/* Commands (§6.3) */
#define ACCCMD_ADDUSER          200
#define ACCCMD_DELETEUSER       201
#define ACCCMD_ADDGROUP         202
#define ACCCMD_DELETEGROUP      203
#define ACCCMD_ADDMEMBER        204
#define ACCCMD_REMOVEMEMBER     205
#define ACCCMD_NEXTUSER         206
#define ACCCMD_NEXTMEMBER       207
#define ACCCMD_NEXTGROUP        208
#define ACCCMD_IDTOUSER         209
#define ACCCMD_NAMETOUSER       210
#define ACCCMD_IDTOGROUP        211
#define ACCCMD_NAMETOGROUP      212
#define ACCCMD_VERIFYUSER       213
#define ACCCMD_MODIFYUSER       214
#define ACCCMD_MODIFYGROUP      215
#define ACCCMD_MEMBEROF         216
#define ACCCMD_VERIFYUSERCRYPT  217
#define ACCCMD_SETPASSWORD      218

#define ACC_SERVER_ENTITY       "Accounts Server"
#define ACC_SERVER_VAR          "Envoy/AccountsServer"
#define ACC_TIMEOUT             5

struct AccContext
{
    struct MinNode      Node;
    struct Task        *Task;
    LONG                OpenCount;
    struct Entity      *Me;             /* this task's source entity         */
    struct Entity      *Link;           /* cached link to the Accounts Server */
    BOOL                HostValid;
    char                Host[128];
};

struct AccountsBase
{
    struct Library          LibNode;
    struct SignalSemaphore  Sem;
    struct MinList          Contexts;
    struct Library         *acc_NIPCBase;
    struct Library         *acc_DOSBase;
};

#define NIPCBase    (AccountsBase->acc_NIPCBase)
#define DOSBase     ((struct DosLibrary *)AccountsBase->acc_DOSBase)

/* accounts_init.c */
struct AccContext *AccGetContext(struct AccountsBase *AccountsBase);

/* accounts_wire.c */
void AccPackUser(UBYTE *dst, const struct UserInfo *user);
void AccUnpackUser(struct UserInfo *user, const UBYTE *src);
void AccPackGroup(UBYTE *dst, const struct GroupInfo *group);
void AccUnpackGroup(struct GroupInfo *group, const UBYTE *src);
void AccPutString(UBYTE *dst, CONST_STRPTR src, ULONG size);
ULONG AccTransact(struct AccountsBase *AccountsBase, struct Entity *server, UBYTE cmd, UBYTE *buffer, ULONG length);

/* accounts_ecrypt.c */
STRPTR AccECrypt(STRPTR buffer, CONST_STRPTR password, CONST_STRPTR username);

#endif /* ACCOUNTS_INTERN_H */
