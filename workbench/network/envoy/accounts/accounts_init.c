/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: accounts.library - library set-up and the per-task contexts.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/nipc.h>
#include <aros/symbolsets.h>
#include <string.h>

#include "accounts_intern.h"

static int Accounts_Init(struct AccountsBase *AccountsBase)
{
    D(bug("accounts.library: init, base %p\n", AccountsBase));
    InitSemaphore(&AccountsBase->Sem);
    NewMinList(&AccountsBase->Contexts);
    if (!(AccountsBase->acc_DOSBase = OpenLibrary("dos.library", 36)))
        return FALSE;
    if (!(AccountsBase->acc_NIPCBase = OpenLibrary("nipc.library", 50)))
        return FALSE;
    return TRUE;
}

static int Accounts_Expunge(struct AccountsBase *AccountsBase)
{
    if (AccountsBase->acc_NIPCBase)
        CloseLibrary(AccountsBase->acc_NIPCBase);
    if (AccountsBase->acc_DOSBase)
        CloseLibrary(AccountsBase->acc_DOSBase);
    return TRUE;
}

static struct AccContext *FindContext(struct AccountsBase *AccountsBase, struct Task *task)
{
    struct AccContext *ctx;

    ForeachNode(&AccountsBase->Contexts, ctx)
    {
        if (ctx->Task == task)
            return ctx;
    }
    return NULL;
}

static void FreeContext(struct AccountsBase *AccountsBase, struct AccContext *ctx)
{
    Remove((struct Node *)ctx);
    if (ctx->Link)
        LoseEntity(ctx->Link);
    if (ctx->Me)
        DeleteEntity(ctx->Me);
    FreeVec(ctx);
}

/*
 * The context of the calling task, created on first use. The entity is
 * created here, by the task that will wait on it.
 */
struct AccContext *AccGetContext(struct AccountsBase *AccountsBase)
{
    struct Task *me = FindTask(NULL);
    struct AccContext *ctx;

    ObtainSemaphore(&AccountsBase->Sem);
    if (!(ctx = FindContext(AccountsBase, me)))
    {
        if ((ctx = AllocVec(sizeof(struct AccContext), MEMF_CLEAR | MEMF_PUBLIC)))
        {
            ctx->Task = me;
            ctx->OpenCount = 1;
            AddTail((struct List *)&AccountsBase->Contexts, (struct Node *)ctx);
        }
    }
    if (ctx && !ctx->Me)
    {
        ULONG sig = 0;
        ctx->Me = CreateEntity(ENT_Name, (IPTR)"accounts.library", ENT_AllocSignal, (IPTR)&sig, TAG_DONE);
    }
    ReleaseSemaphore(&AccountsBase->Sem);
    return (ctx && ctx->Me) ? ctx : NULL;
}

static int Accounts_Open(struct AccountsBase *AccountsBase)
{
    struct AccContext *ctx;

    D(bug("accounts.library: open by task %p\n", FindTask(NULL)));
    ObtainSemaphore(&AccountsBase->Sem);
    if ((ctx = FindContext(AccountsBase, FindTask(NULL))))
        ctx->OpenCount++;
    ReleaseSemaphore(&AccountsBase->Sem);
    return TRUE;
}

static int Accounts_Close(struct AccountsBase *AccountsBase)
{
    struct AccContext *ctx;

    ObtainSemaphore(&AccountsBase->Sem);
    if ((ctx = FindContext(AccountsBase, FindTask(NULL))) && --ctx->OpenCount <= 0)
        FreeContext(AccountsBase, ctx);
    ReleaseSemaphore(&AccountsBase->Sem);
    return TRUE;
}

ADD2INITLIB(Accounts_Init, 0);
ADD2EXPUNGELIB(Accounts_Expunge, 0);
ADD2OPENLIB(Accounts_Open, 0);
ADD2CLOSELIB(Accounts_Close, 0);
