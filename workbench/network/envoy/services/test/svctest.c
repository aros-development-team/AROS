/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SvcTest - exercise services.library.

    SvcTest HOST/K,SERVICE/K,USER/K,PASSWORD/K,COUNT/K/N,SIZE/K/N

    FindService(HOST, SERVICE) (default: the local host, "Echo") with the
    given credentials, then COUNT transactions (default 1) of SIZE bytes
    (default 64) through the returned entity, expecting the echo service's
    upper-cased copy with trans_Error = command. Prints the FSVC_Error code
    on failure; return code 5 then.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/nipc.h>
#include <proto/services.h>
#include <envoy/nipc.h>
#include <envoy/services.h>
#include <envoy/errors.h>
#include <string.h>
#include <stdio.h>

struct Library *NIPCBase;
struct Library *ServicesBase;

static const char *ErrorText(ULONG err)
{
    switch (err)
    {
    case ENVOYERR_NORESOURCES:      return "no resources";
    case ENVOYERR_UNKNOWNHOST:      return "unknown host";
    case ENVOYERR_UNKNOWNENTITY:    return "unknown entity";
    case ENVOYERR_NORESOLVER:       return "no resolver";
    case ENVOYERR_CANTDELIVER:      return "cannot deliver";
    case ENVOYERR_TIMEOUT:          return "timeout";
    case ENVOYERR_UNKNOWNSERVICE:   return "unknown service";
    case ENVOYERR_OPENSERVICEFAIL:  return "service library cannot be opened";
    case ENVOYERR_BADSTARTSERVICE:  return "bad request";
    }
    return "";
}

static int Echo(struct Entity *svc, struct Entity *me, LONG count, LONG size)
{
    struct Transaction *t;
    LONG i, ok = 0;

    if (!(t = AllocTransaction(TRN_AllocReqBuffer, size, TRN_AllocRespBuffer, size, TAG_DONE)))
    {
        printf("  AllocTransaction failed\n");
        return RETURN_FAIL;
    }
    for (i = 0; i < count; i++)
    {
        UBYTE *req = t->trans_RequestData, *resp = t->trans_ResponseData;
        LONG k;
        ULONG err;
        BOOL good = TRUE;

        for (k = 0; k < size; k++)
            req[k] = 'a' + ((k + i) % 26);
        memset(resp, 0, size);
        t->trans_ReqDataActual = size;
        t->trans_Command = (UBYTE)(i + 1);
        t->trans_Timeout = 10;
        err = DoTransaction(svc, me, t);
        if (err != (ULONG)(i + 1))
        {
            printf("  transaction %ld: error %lu (%s), expected %ld\n", (long)i + 1, (unsigned long)err, ErrorText(err), (long)i + 1);
            good = FALSE;
        }
        else if (t->trans_RespDataActual != (ULONG)size)
        {
            printf("  transaction %ld: %lu response bytes, expected %ld\n", (long)i + 1, (unsigned long)t->trans_RespDataActual, (long)size);
            good = FALSE;
        }
        else
            for (k = 0; k < size; k++)
                if (resp[k] != req[k] - 32)
                {
                    printf("  transaction %ld: byte %ld wrong\n", (long)i + 1, (long)k);
                    good = FALSE;
                    break;
                }
        if (good)
            ok++;
    }
    FreeTransaction(t);
    printf("  %ld of %ld transactions of %ld bytes echoed correctly\n", (long)ok, (long)count, (long)size);
    return ok == count ? RETURN_OK : RETURN_ERROR;
}

int main(void)
{
    IPTR args[6] = { 0 };
    struct RDArgs *rda;
    CONST_STRPTR host, service, user, pass;
    LONG count = 1, size = 64;
    struct Entity *me, *svc;
    ULONG err = 0;
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs("HOST/K,SERVICE/K,USER/K,PASSWORD/K,COUNT/K/N,SIZE/K/N", args, NULL)))
    {
        PrintFault(IoErr(), "SvcTest");
        return RETURN_FAIL;
    }
    host = (CONST_STRPTR)args[0];
    service = args[1] ? (CONST_STRPTR)args[1] : (CONST_STRPTR)"Echo";
    user = (CONST_STRPTR)args[2];
    pass = (CONST_STRPTR)args[3];
    if (args[4]) count = *(LONG *)args[4];
    if (args[5]) size = *(LONG *)args[5];
    if (size < 1) size = 1;

    if (!(NIPCBase = OpenLibrary(NIPCNAME, 50)))
    {
        printf("cannot open nipc.library 50\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (!(ServicesBase = OpenLibrary("services.library", 50)))
    {
        printf("cannot open services.library 50\n");
        CloseLibrary(NIPCBase);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if ((me = CreateEntity(ENT_Name, (IPTR)"SvcTest Client", ENT_AllocSignal, 0, TAG_DONE)))
    {
        svc = FindService(host, service, me,
                          FSVC_Error, (IPTR)&err,
                          user ? FSVC_UserName : TAG_IGNORE, (IPTR)user,
                          pass ? FSVC_PassWord : TAG_IGNORE, (IPTR)pass,
                          TAG_DONE);
        if (svc)
        {
            char ename[64] = "?", hname[128] = "?";

            GetEntityName(svc, ename, sizeof(ename));
            GetHostName(svc, hname, sizeof(hname));
            printf("FindService(%s, %s): entity '%s' on host '%s'\n", host ? (const char *)host : "local", service, ename, hname);
            rc = Echo(svc, me, count, size);
            LoseService(svc);
        }
        else
        {
            printf("FindService(%s, %s) failed: %lu %s\n", host ? (const char *)host : "local", service, (unsigned long)err, ErrorText(err));
            rc = RETURN_WARN;
        }
        DeleteEntity(me);
    }
    CloseLibrary(ServicesBase);
    CloseLibrary(NIPCBase);
    FreeArgs(rda);
    return rc;
}
