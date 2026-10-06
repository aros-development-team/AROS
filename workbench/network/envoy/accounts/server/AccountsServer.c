/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy Accounts Server for AROS.

    AccountsServer SHUTDOWN/S,VERBOSE/S

    Publishes the entity "Accounts Server" and answers the Envoy accounts
    protocol (re/spec/services-accounts.md §6) from the system's own account
    store (acc_store.c). A second invocation makes the running one quit;
    SHUTDOWN also waits until it has gone. Start it detached:
        Run >NIL: SYS:System/Network/Envoy/AccountsServer
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/nipc.h>
#include <envoy/nipc.h>
#include <string.h>
#include <stdio.h>

#include "acc_store.h"
#include "acc_cmds.h"

#define PORTNAME    "Accounts Server"

/* nipc.library is opened by the link library's auto-open code */

const char version[] = "$VER: AccountsServer 50.0 (6.10.2026)";

static void Serve(struct AccStore *s, struct Transaction *t)
{
    char host[128];
    UBYTE *req = t->trans_RequestData;
    ULONG len = t->trans_ReqDataActual;

    host[0] = '\0';
    GetHostName(t->trans_SourceEntity, host, sizeof(host));

    if (!req)
        t->trans_Error = ENVOYERR_SMALLREQBUFF;
    else
        t->trans_Error = AccHandle(s, t->trans_Command, req, len, host);

    /* the request buffer is the response (§6.1); copy if it is separate */
    if (t->trans_ResponseData && req)
    {
        ULONG n = len < t->trans_RespDataLength ? len : t->trans_RespDataLength;
        if (t->trans_ResponseData != req)
            memcpy(t->trans_ResponseData, req, n);
        t->trans_RespDataActual = n;
    }
    else
        t->trans_RespDataActual = 0;

    if (s->Verbose)
    {
        printf("AccountsServer: command %d from %s -> %lu\n", t->trans_Command, host, (unsigned long)t->trans_Error);
        fflush(stdout);
    }
}

int main(void)
{
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda;
    struct MsgPort *port, *existing;
    struct Entity *me;
    struct AccStore store;
    ULONG sig = 0;
    BOOL verbose;
    int rc = RETURN_OK;

    if (!(rda = ReadArgs("SHUTDOWN/S,VERBOSE/S", args, NULL)))
    {
        PrintFault(IoErr(), "AccountsServer");
        return RETURN_ERROR;
    }
    verbose = args[1] != 0;

    Forbid();
    existing = FindPort(PORTNAME);
    if (existing)
        Signal(existing->mp_SigTask, SIGBREAKF_CTRL_C);
    Permit();
    if (existing)
    {
        if (args[0])
        {
            int n;
            for (n = 0; n < 50 && FindPort(PORTNAME); n++)
                Delay(10);
        }
        printf("AccountsServer: running instance told to quit\n");
        FreeArgs(rda);
        return RETURN_OK;
    }
    if (args[0])
    {
        FreeArgs(rda);
        return RETURN_OK;
    }
    FreeArgs(rda);

    if (!StoreInit(&store, verbose))
    {
        printf("AccountsServer: no account store available\n");
        return RETURN_FAIL;
    }
    if (!(port = CreateMsgPort()))
        return RETURN_FAIL;
    port->mp_Node.ln_Name = PORTNAME;
    port->mp_Node.ln_Pri = 0;
    AddPort(port);

    if (!(me = CreateEntity(ENT_Name, (IPTR)"Accounts Server", ENT_Public, TRUE, ENT_AllocSignal, (IPTR)&sig, TAG_DONE)))
    {
        printf("AccountsServer: cannot create the entity\n");
        RemPort(port);
        DeleteMsgPort(port);
        StoreCleanup(&store);
        return RETURN_FAIL;
    }
    if (verbose)
    {
        printf("AccountsServer: up\n");
        fflush(stdout);
    }

    for (;;)
    {
        struct Transaction *t;
        ULONG got = Wait((1UL << sig) | SIGBREAKF_CTRL_C | (1UL << port->mp_SigBit));
        struct Message *m;

        while ((m = GetMsg(port)))
            ReplyMsg(m);
        if (got & SIGBREAKF_CTRL_C)
            break;
        StoreRefresh(&store);
        while ((t = GetTransaction(me)))
        {
            if (t->trans_Type == TYPE_SERVICING)
            {
                Serve(&store, t);
                ReplyTransaction(t);
            }
        }
        StoreCommit(&store);        /* queue empty: write what changed (§8.1) */
    }

    StoreCommit(&store);
    DeleteEntity(me);
    RemPort(port);
    DeleteMsgPort(port);
    StoreCleanup(&store);
    if (verbose)
        printf("AccountsServer: down\n");
    return rc;
}
