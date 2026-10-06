/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Services Manager - owns the public entity "Services Manager" and
          starts services for FindService() clients
          (re/spec/services-accounts.md §2-§4).

          A request (command 100, 128 bytes: service name, user name,
          password) names a service of ENV:Envoy/services.prefs; its
          .service library is opened and its StartServiceA() called with
          the credentials and the client's host name; the entity name the
          service returns goes back in the 64-byte response. Errors: 560
          unknown (or inactive) service, 561 library cannot be opened, 562
          malformed request, otherwise the service's own result.

          The configuration is reloaded whenever the ENV: file changes.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/nipc.h>
#include <proto/services.h>
#include <aros/libcall.h>
#include <dos/notify.h>
#include <envoy/nipc.h>
#include <envoy/services.h>
#include <envoy/errors.h>
#include <string.h>

#include "../services_private.h"
#include "prefsfile.h"

const char version[] = "$VER: ServicesManager 50.0 (6.10.2026)";

struct Library *NIPCBase;
struct Library *ServicesBase;

#define SVCREQ_SIZE             128
#define SVCREQ_USER             64
#define SVCREQ_PASSWORD         96
#define SVCRESP_SIZE            64
#define SVCCMD_FINDSERVICE      100

/* xxx.service vectors (svc_lib.fd): StartServiceA is the second function */
#define LVO_StartServiceA       6

static ULONG CallStartService(struct Library *svc, struct TagItem *tags)
{
    return AROS_LC1(ULONG, StartServiceA, AROS_LCA(struct TagItem *, tags, A0), struct Library *, svc, LVO_StartServiceA, Svc);
}

static void LoadConfiguration(void)
{
    struct List entries;
    struct PrefsEntry *e;
    struct ServiceNode *n;
    LONG count;

    NEWLIST(&entries);
    count = ReadServicesPrefs(SERVICES_PREFS_ENV, &entries);
    if (count < 0)
        count = ReadServicesPrefs(SERVICES_PREFS_ENVARC, &entries);

    /* replace the list: old nodes go when the lock is released */
    for (n = LockServiceList(); n; n = NextService(n))
        MarkServiceTemp(n);
    UnlockServiceList();
    ForeachNode(&entries, e)
    {
        struct TagItem tags[] = { { SVCL_Active, e->pe_Active }, { TAG_DONE, 0 } };
        AddService(e->pe_Name, e->pe_Path, tags);
    }
    FreePrefsEntries(&entries);
}

static void CopyField(char *dst, ULONG dstsize, const UBYTE *src, ULONG srcsize)
{
    ULONG n = 0;

    while (n < srcsize && n < dstsize - 1 && src[n])
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}

static void HandleRequest(struct Transaction *t)
{
    ULONG err = 0;
    UBYTE *req = t->trans_RequestData;

    if (t->trans_ResponseData && t->trans_RespDataLength >= SVCRESP_SIZE)
        memset(t->trans_ResponseData, 0, SVCRESP_SIZE);

    if (t->trans_Command != SVCCMD_FINDSERVICE || !req || t->trans_ReqDataActual < SVCREQ_SIZE
        || !t->trans_ResponseData || t->trans_RespDataLength < SVCRESP_SIZE)
    {
        err = ENVOYERR_BADSTARTSERVICE;
    }
    else
    {
        char name[SVCNODE_NAMESIZE], user[32], pass[32], path[SVCNODE_PATHSIZE];
        struct ServiceNode *n;
        BOOL active = FALSE;

        CopyField(name, sizeof(name), req, SVCREQ_USER);
        CopyField(user, sizeof(user), req + SVCREQ_USER, SVCREQ_PASSWORD - SVCREQ_USER);
        CopyField(pass, sizeof(pass), req + SVCREQ_PASSWORD, SVCREQ_SIZE - SVCREQ_PASSWORD);
        path[0] = '\0';

        LockServiceList();
        if ((n = FindServiceByName(name)))
        {
            IPTR a = 0;
            struct TagItem tags[] = { { SVCL_Active, (IPTR)&a }, { TAG_DONE, 0 } };

            GetServiceAttrsA(n, tags);
            active = a ? TRUE : FALSE;
            strncpy(path, n->sn_Path, sizeof(path) - 1);
        }
        UnlockServiceList();

        if (!n || !active)
            err = ENVOYERR_UNKNOWNSERVICE;
        else
        {
            struct Library *svc = OpenLibrary(path, 0);

            if (!svc)
                err = ENVOYERR_OPENSERVICEFAIL;
            else
            {
                char host[128];
                struct TagItem tags[] =
                {
                    { SSVC_UserName,    (IPTR)user                      },
                    { SSVC_Password,    (IPTR)pass                      },
                    { SSVC_EntityName,  (IPTR)t->trans_ResponseData     },
                    { SSVC_HostName,    (IPTR)host                      },
                    { TAG_DONE,         0                               }
                };

                host[0] = '\0';
                GetHostName(t->trans_SourceEntity, host, sizeof(host));
                err = CallStartService(svc, tags);
                CloseLibrary(svc);
            }
        }
    }
    t->trans_Error = err;
    t->trans_RespDataActual = (t->trans_ResponseData && t->trans_RespDataLength >= SVCRESP_SIZE) ? SVCRESP_SIZE : 0;
    ReplyTransaction(t);
}

int main(void)
{
    struct Entity *entity = NULL;
    struct NotifyRequest notify;
    ULONG esig = 0;
    BYTE nsig;
    BOOL notifying = FALSE;
    int rc = RETURN_FAIL;

    if (!(NIPCBase = OpenLibrary(NIPCNAME, 50)))
    {
        PutStr("ServicesManager: cannot open nipc.library 50\n");
        return RETURN_FAIL;
    }
    if (!(ServicesBase = OpenLibrary("services.library", 50)))
    {
        PutStr("ServicesManager: cannot open services.library 50\n");
        CloseLibrary(NIPCBase);
        return RETURN_FAIL;
    }
    if ((nsig = AllocSignal(-1)) < 0)
        goto done;
    if (!(entity = CreateEntity(ENT_Name, (IPTR)SERVICES_MANAGER_ENTITY, ENT_Public, TRUE, ENT_AllocSignal, (IPTR)&esig, TAG_DONE)))
    {
        PutStr("ServicesManager: cannot create the entity (already running?)\n");
        goto done;
    }

    LoadConfiguration();

    memset(&notify, 0, sizeof(notify));
    notify.nr_Name = (STRPTR)SERVICES_PREFS_ENV;
    notify.nr_Flags = NRF_SEND_SIGNAL;
    notify.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
    notify.nr_stuff.nr_Signal.nr_SignalNum = nsig;
    notifying = StartNotify(&notify) ? TRUE : FALSE;

    for (;;)
    {
        struct Transaction *t;
        ULONG got = Wait((1UL << esig) | (1UL << nsig) | SIGBREAKF_CTRL_C);

        if (got & SIGBREAKF_CTRL_C)
            break;
        if (got & (1UL << nsig))
        {
            Delay(10);                      /* let the writer finish */
            SetSignal(0, 1UL << nsig);
            LoadConfiguration();
        }
        while ((t = GetTransaction(entity)))
        {
            if (t->trans_Type == TYPE_SERVICING)
                HandleRequest(t);
        }
    }
    rc = RETURN_OK;

done:
    if (notifying)
        EndNotify(&notify);
    if (entity)
        DeleteEntity(entity);
    if (nsig >= 0)
        FreeSignal(nsig);
    {
        struct ServiceNode *n;
        for (n = LockServiceList(); n; n = NextService(n))
            MarkServiceTemp(n);
        UnlockServiceList();
    }
    CloseLibrary(ServicesBase);
    CloseLibrary(NIPCBase);
    return rc;
}
