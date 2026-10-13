/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: NipcTest - exercise nipc.library.

    NipcTest SERVER/S,NAME/K,HOST/K,ENTITY/K,COUNT/K/N,SIZE/K/N,INQUIRY/S,PING/S,SECS/K/N,ACCOUNTS/S,SERVICE/K

    ACCOUNTS          client: list the users of HOST's "Accounts Server" (Envoy
                      command 206, in-place 72-byte records) until error 543.
    SERVICE=name      client: ask HOST's "Services Manager" for the entity of a
                      service (Envoy command 100).
    SERVER            create a public entity (NAME, default "Echo Server") and
                      answer every request by copying the request data into the
                      response, upper-cased, with trans_Error = command; runs for
                      SECS seconds (default 60) or until CTRL-C.
    HOST= ENTITY=     client: FindEntity(HOST, ENTITY), then COUNT transactions
                      of SIZE bytes (default 1 x 64), checking the echo; PING
                      adds three PingEntity() calls.
    INQUIRY           run NIPCInquiry(QUERY_HOSTNAME, QUERY_IPADDR, QUERY_ENTITY)
                      for SECS seconds (default 3) and list the answers.
    Without arguments: local round trip in one process (server entity and
    client entity, local FindEntity path).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/nipc.h>
#include <proto/utility.h>
#include <envoy/nipc.h>
#include <envoy/errors.h>
#include <string.h>
#include <stdio.h>

struct Library *NIPCBase;

static void ServeOne(struct Transaction *t)
{
    ULONG n = t->trans_ReqDataActual, i;
    UBYTE *src = t->trans_RequestData, *dst = t->trans_ResponseData;

    if (n > t->trans_RespDataLength)
        n = t->trans_RespDataLength;
    if (dst && src)
        for (i = 0; i < n; i++)
            dst[i] = (src[i] >= 'a' && src[i] <= 'z') ? src[i] - 32 : src[i];
    t->trans_RespDataActual = n;
    t->trans_Error = t->trans_Command;
}

static int RunServer(CONST_STRPTR name, LONG secs)
{
    struct Entity *e;
    ULONG sig = 0, served = 0;
    LONG left = secs * 50;

    if (!(e = CreateEntity(ENT_Name, (IPTR)name, ENT_Public, TRUE, ENT_AllocSignal, (IPTR)&sig, TAG_DONE)))
    {
        printf("CreateEntity failed\n");
        return RETURN_FAIL;
    }
    printf("serving entity '%s' (signal %lu) for %ld s\n", name, (unsigned long)sig, (long)secs);
    while (left > 0)
    {
        struct Transaction *t;
        ULONG got = SetSignal(0, (1UL << sig) | SIGBREAKF_CTRL_C);
        if (got & SIGBREAKF_CTRL_C)
            break;
        if (!(got & (1UL << sig)))
        {
            Delay(10);                  /* 0.2 s; a test program may poll */
            left -= 10;
            continue;
        }
        while ((t = GetTransaction(e)))
        {
            if (t->trans_Type == TYPE_SERVICING)
            {
                char host[128] = "?", en[64] = "?";
                GetHostName(t->trans_SourceEntity, host, sizeof(host));
                GetEntityName(t->trans_SourceEntity, en, sizeof(en));
                ServeOne(t);
                served++;
                printf("  request cmd %d, %lu bytes from %s@%s\n", t->trans_Command, (unsigned long)t->trans_ReqDataActual, en, host);
                ReplyTransaction(t);
            }
        }
    }
    printf("served %lu requests\n", (unsigned long)served);
    DeleteEntity(e);
    return RETURN_OK;
}

static int RunClient(CONST_STRPTR host, CONST_STRPTR entity, LONG count, LONG size, BOOL ping)
{
    struct Entity *me, *dest;
    struct Transaction *t;
    ULONG err = 0, i, j;
    int rc = RETURN_OK;
    char hostname[128];

    if (!(me = CreateEntity(ENT_Name, (IPTR)"NipcTest Client", ENT_AllocSignal, 0, TAG_DONE)))
    {
        printf("CreateEntity failed\n");
        return RETURN_FAIL;
    }
    dest = FindEntity((STRPTR)host, (STRPTR)entity, me, &err);
    if (!dest)
    {
        printf("FindEntity(%s, %s) failed: %lu\n", host ? (const char *)host : "<local>", (const char *)entity, (unsigned long)err);
        DeleteEntity(me);
        return RETURN_FAIL;
    }
    GetHostName(dest, hostname, sizeof(hostname));
    printf("found '%s' on host '%s'\n", entity, hostname);

    if (ping)
        for (i = 0; i < 3; i++)
            printf("  ping: %lu us\n", (unsigned long)PingEntity(dest, 2000000));

    if ((t = AllocTransaction(TRN_AllocReqBuffer, size, TRN_AllocRespBuffer, size, TAG_DONE)))
    {
        for (i = 0; i < (ULONG)count && rc == RETURN_OK; i++)
        {
            UBYTE *req = t->trans_RequestData, *resp = t->trans_ResponseData;
            for (j = 0; j < (ULONG)size; j++)
                req[j] = 'a' + ((i + j) % 26);
            memset(resp, 0, size);
            t->trans_Command = (UBYTE)(i + 1);
            t->trans_ReqDataActual = size;
            t->trans_RespDataActual = size;
            t->trans_Timeout = 10;
            err = DoTransaction(dest, me, t);
            if (err != (ULONG)((i + 1) & 0xFF))
            {
                printf("  transaction %lu: error %lu (expected %lu)\n", (unsigned long)i, (unsigned long)err, (unsigned long)((i + 1) & 0xFF));
                rc = RETURN_WARN;
                break;
            }
            for (j = 0; j < (ULONG)size; j++)
            {
                UBYTE want = req[j] >= 'a' && req[j] <= 'z' ? req[j] - 32 : req[j];
                if (resp[j] != want)
                {
                    printf("  transaction %lu: byte %lu is %02x, expected %02x\n", (unsigned long)i, (unsigned long)j, resp[j], want);
                    rc = RETURN_WARN;
                    break;
                }
            }
            if (t->trans_RespDataActual != (ULONG)size)
            {
                printf("  transaction %lu: %lu response bytes, expected %ld\n", (unsigned long)i, (unsigned long)t->trans_RespDataActual, (long)size);
                rc = RETURN_WARN;
            }
        }
        if (rc == RETURN_OK)
            printf("  %ld transactions of %ld bytes echoed correctly\n", (long)count, (long)size);
        FreeTransaction(t);
    }
    LoseEntity(dest);
    DeleteEntity(me);
    return rc;
}


/* Envoy accounts: iterate users with command 206 (re/spec/services-accounts.md §6.4) */
static int RunAccounts(CONST_STRPTR host)
{
    struct Entity *me, *dest;
    struct Transaction *t;
    ULONG err = 0;
    int rc = RETURN_FAIL, n = 0;

    if (!(me = CreateEntity(ENT_Name, (IPTR)"NipcTest Client", ENT_AllocSignal, 0, TAG_DONE)))
        return RETURN_FAIL;
    if (!(dest = FindEntity((STRPTR)host, "Accounts Server", me, &err)))
    {
        printf("FindEntity(%s, Accounts Server) failed: %lu\n", (const char *)host, (unsigned long)err);
        DeleteEntity(me);
        return RETURN_FAIL;
    }
    if ((t = AllocTransaction(TRN_AllocReqBuffer, 72, TAG_DONE)))
    {
        UBYTE *rec = t->trans_RequestData;
        memset(rec, 0, 72);
        for (;;)
        {
            t->trans_Command = 206;
            t->trans_ReqDataActual = 72;
            t->trans_ResponseData = rec;            /* in place */
            t->trans_RespDataLength = 72;
            t->trans_RespDataActual = 72;
            t->trans_Timeout = 10;
            err = DoTransaction(dest, me, t);
            if (err)
                break;
            printf("  user '%s' uid %u gid %u flags 0x%08lx\n", (char *)rec, (rec[32] << 8) | rec[33], (rec[34] << 8) | rec[35],
                   (unsigned long)(((ULONG)rec[36] << 24) | (rec[37] << 16) | (rec[38] << 8) | rec[39]));
            n++;
            if (n > 100)
                break;
        }
        printf("  NextUser ended with error %lu after %d users%s\n", (unsigned long)err, n, err == 543 ? " (ENVOYERR_LASTUSER: end of list)" : "");
        rc = (err == 543) ? RETURN_OK : RETURN_WARN;
        t->trans_ResponseData = NULL;
        FreeTransaction(t);
    }
    LoseEntity(dest);
    DeleteEntity(me);
    return rc;
}

/* Envoy FindService: command 100 to the Services Manager (re/spec/services-accounts.md §1) */
static int RunService(CONST_STRPTR host, CONST_STRPTR service)
{
    struct Entity *me, *dest;
    struct Transaction *t;
    ULONG err = 0;
    int rc = RETURN_FAIL;

    if (!(me = CreateEntity(ENT_Name, (IPTR)"NipcTest Client", ENT_AllocSignal, 0, TAG_DONE)))
        return RETURN_FAIL;
    if (!(dest = FindEntity((STRPTR)host, "Services Manager", me, &err)))
    {
        printf("FindEntity(%s, Services Manager) failed: %lu\n", (const char *)host, (unsigned long)err);
        DeleteEntity(me);
        return RETURN_FAIL;
    }
    if ((t = AllocTransaction(TRN_AllocReqBuffer, 128, TRN_AllocRespBuffer, 64, TAG_DONE)))
    {
        memset(t->trans_RequestData, 0, 128);
        strncpy(t->trans_RequestData, (const char *)service, 63);
        t->trans_Command = 100;
        t->trans_Timeout = 10;
        err = DoTransaction(dest, me, t);
        printf("  FindService(\"%s\"): error %lu%s, entity '%s'\n", (const char *)service, (unsigned long)err,
               err == 560 ? " (ENVOYERR_UNKNOWNSERVICE)" : "", err ? "" : (char *)t->trans_ResponseData);
        rc = (err == 0 || err == 560) ? RETURN_OK : RETURN_WARN;
        FreeTransaction(t);
    }
    LoseEntity(dest);
    DeleteEntity(me);
    return rc;
}

static ULONG InqCount;

AROS_UFH3(static IPTR, InqHook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct Task *, caller, A2),
          AROS_UFHA(struct TagItem *, tags, A1))
{
    AROS_USERFUNC_INIT

    struct TagItem *tag, *tstate = tags;

    if (!tags)
    {
        printf("inquiry finished, %lu answers\n", (unsigned long)InqCount);
        Signal(caller, SIGBREAKF_CTRL_D);
        return TRUE;
    }
    InqCount++;
    printf("answer:");
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case QUERY_IPADDR:
            printf(" ip=%lu.%lu.%lu.%lu", (unsigned long)(tag->ti_Data >> 24) & 255, (unsigned long)(tag->ti_Data >> 16) & 255,
                   (unsigned long)(tag->ti_Data >> 8) & 255, (unsigned long)tag->ti_Data & 255);
            break;
        case QUERY_HOSTNAME: printf(" host='%s'", (STRPTR)tag->ti_Data); break;
        case QUERY_ENTITY:   printf(" entity='%s'", (STRPTR)tag->ti_Data); break;
        case QUERY_OWNER:    printf(" owner='%s'", (STRPTR)tag->ti_Data); break;
        case QUERY_NIPCVERSION: printf(" nipc=%lu.%lu", (unsigned long)tag->ti_Data >> 16, (unsigned long)tag->ti_Data & 0xFFFF); break;
        default: printf(" %08lx=%08lx", (unsigned long)tag->ti_Tag, (unsigned long)tag->ti_Data); break;
        }
    }
    printf("\n");
    return TRUE;

    AROS_USERFUNC_EXIT
}

static int RunInquiry(LONG secs)
{
    struct Hook hook;
    struct TagItem tags[] = { { QUERY_HOSTNAME, 0 }, { QUERY_IPADDR, 0 }, { QUERY_ENTITY, 0 }, { QUERY_OWNER, 0 }, { QUERY_NIPCVERSION, 0 }, { TAG_DONE, 0 } };

    memset(&hook, 0, sizeof(hook));
    hook.h_Entry = (HOOKFUNC)InqHook;
    InqCount = 0;
    SetSignal(0, SIGBREAKF_CTRL_D);
    if (!NIPCInquiryA(&hook, secs, 100, tags))
    {
        printf("NIPCInquiryA failed\n");
        return RETURN_FAIL;
    }
    Wait(SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_C);
    return RETURN_OK;
}

int main(void)
{
    IPTR args[11] = { 0 };
    struct RDArgs *rda;
    int rc = RETURN_FAIL;
    char local[128];

    if (!(rda = ReadArgs("SERVER/S,NAME/K,HOST/K,ENTITY/K,COUNT/K/N,SIZE/K/N,INQUIRY/S,PING/S,SECS/K/N,ACCOUNTS/S,SERVICE/K", args, NULL)))
    {
        PrintFault(IoErr(), "NipcTest");
        return RETURN_ERROR;
    }
    if (!(NIPCBase = OpenLibrary("nipc.library", 50)))
    {
        printf("NipcTest: cannot open nipc.library\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    GetHostName(NULL, local, sizeof(local));
    printf("nipc.library %d.%d, this host is '%s'\n", NIPCBase->lib_Version, NIPCBase->lib_Revision, local);

    if (args[0])
        rc = RunServer(args[1] ? (CONST_STRPTR)args[1] : (CONST_STRPTR)"Echo Server", args[8] ? *(LONG *)args[8] : 60);
    else if (args[6])
        rc = RunInquiry(args[8] ? *(LONG *)args[8] : 3);
    else if (args[9] && args[2])
        rc = RunAccounts((CONST_STRPTR)args[2]);
    else if (args[10] && args[2])
        rc = RunService((CONST_STRPTR)args[2], (CONST_STRPTR)args[10]);
    else if (args[3])
        rc = RunClient((CONST_STRPTR)args[2], (CONST_STRPTR)args[3], args[4] ? *(LONG *)args[4] : 1, args[5] ? *(LONG *)args[5] : 64, args[7] ? TRUE : FALSE);
    else
    {
        /* local round trip: a server entity in this task, served between the client calls */
        struct Entity *srv, *me, *dest;
        struct Transaction *t;
        ULONG err;
        srv = CreateEntity(ENT_Name, (IPTR)"Local Echo", ENT_Public, TRUE, TAG_DONE);
        me = CreateEntity(ENT_Name, (IPTR)"Local Client", TAG_DONE);
        dest = srv && me ? FindEntity(NULL, "Local Echo", me, &err) : NULL;
        printf("local FindEntity: %s\n", dest ? "ok" : "failed");
        if (dest && (t = AllocTransaction(TRN_AllocReqBuffer, 32, TRN_AllocRespBuffer, 32, TAG_DONE)))
        {
            struct Transaction *r;
            strcpy(t->trans_RequestData, "hello local entity");
            t->trans_ReqDataActual = 19;
            t->trans_Command = 7;
            BeginTransaction(dest, me, t);
            r = GetTransaction(srv);
            printf("server got %s, type %d, cmd %d, data '%s'\n", r == t ? "the same structure" : "something else", r ? r->trans_Type : -1, r ? r->trans_Command : -1, r ? (char *)r->trans_RequestData : "");
            if (r) { ServeOne(r); ReplyTransaction(r); }
            err = WaitTransaction(t);
            printf("client: error %lu, response '%s', %lu bytes\n", (unsigned long)err, (char *)t->trans_ResponseData, (unsigned long)t->trans_RespDataActual);
            rc = (err == 7 && !strcmp(t->trans_ResponseData, "HELLO LOCAL ENTITY")) ? RETURN_OK : RETURN_WARN;
            FreeTransaction(t);
        }
        if (dest) LoseEntity(dest);
        if (me) DeleteEntity(me);
        if (srv) DeleteEntity(srv);
    }
    CloseLibrary(NIPCBase);
    FreeArgs(rda);
    return rc;
}
