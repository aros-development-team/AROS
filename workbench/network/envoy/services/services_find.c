/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: services.library - FindServiceA() and LoseService()
          (re/spec/services-accounts.md §1)
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/nipc.h>
#include <envoy/errors.h>
#include <string.h>

#include "services_intern.h"

/* The command-100 request: name at +0 (64), user at +64 (32), password at +96 (32) */
#define SVCREQ_SIZE             128
#define SVCREQ_NAME             0
#define SVCREQ_USER             64
#define SVCREQ_PASSWORD         96
#define SVCRESP_SIZE            64
#define SVCCMD_FINDSERVICE      100
#define SVCTIMEOUT              10

static void PutField(UBYTE *buf, ULONG size, CONST_STRPTR s)
{
    ULONG n = 0;

    if (s)
        while (n < size - 1 && s[n])
        {
            buf[n] = s[n];
            n++;
        }
    buf[n] = '\0';
}

/*****************************************************************************

    NAME */
        AROS_LH4(struct Entity *, FindServiceA,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, remoteHost, A0),
        AROS_LHA(CONST_STRPTR, serviceName, A1),
        AROS_LHA(struct Entity *, srcEntity, A2),
        AROS_LHA(struct TagItem *, tagList, A3),

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 5, Services)

/*  FUNCTION
        Connect to a service: ask the Services Manager of remoteHost (NULL
        for this machine) to start serviceName for the user given by the
        tags, then FindEntity() the entity it names, from srcEntity.

    INPUTS
        remoteHost  - host name, or NULL for the local host
        serviceName - name of the service as configured on that host
        srcEntity   - the caller's entity; the returned link belongs to it
        tagList     - FSVC_UserName (STRPTR), FSVC_PassWord (STRPTR),
                      FSVC_Error (ULONG *)

    RESULT
        The connected entity, or NULL. On failure the ULONG pointed to by
        FSVC_Error receives: the FindEntity() error if the Services Manager
        could not be reached (521/522/523), the Services Manager's reply
        (560 unknown service, 561 service library cannot be opened, 562 bad
        request, or the service's own StartServiceA() error), the error of
        the final FindEntity(), or 501 for a resource failure.

    NOTES
        Every successful call needs a LoseService().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG *errptr = (ULONG *)GetTagData(FSVC_Error, 0, tagList);
    CONST_STRPTR user = (CONST_STRPTR)GetTagData(FSVC_UserName, 0, tagList);
    CONST_STRPTR pass = (CONST_STRPTR)GetTagData(FSVC_PassWord, 0, tagList);
    struct Entity *me = NULL, *mgr, *result = NULL;
    struct Transaction *t = NULL;
    ULONG err = ENVOYERR_NORESOURCES;

    if (!srcEntity || !serviceName)
    {
        err = ENVOYERR_UNKNOWNENTITY;
        goto done;
    }
    if (!(me = CreateEntity(ENT_AllocSignal, 0, TAG_DONE)))
        goto done;
    if (!(t = AllocTransaction(TRN_AllocReqBuffer, SVCREQ_SIZE, TRN_AllocRespBuffer, SVCRESP_SIZE, TAG_DONE)))
        goto done;

    err = 0;
    if (!(mgr = FindEntity(remoteHost, SERVICES_MANAGER_ENTITY, me, &err)))
    {
        if (!err)
            err = ENVOYERR_UNKNOWNENTITY;
        goto done;
    }

    /* unlike the original, never send what the buffer held before */
    memset(t->trans_RequestData, 0, SVCREQ_SIZE);
    PutField((UBYTE *)t->trans_RequestData + SVCREQ_NAME, SVCREQ_USER - SVCREQ_NAME, serviceName);
    PutField((UBYTE *)t->trans_RequestData + SVCREQ_USER, SVCREQ_PASSWORD - SVCREQ_USER, user);
    PutField((UBYTE *)t->trans_RequestData + SVCREQ_PASSWORD, SVCREQ_SIZE - SVCREQ_PASSWORD, pass);
    memset(t->trans_ResponseData, 0, SVCRESP_SIZE);
    t->trans_ReqDataActual = SVCREQ_SIZE;
    t->trans_Command = SVCCMD_FINDSERVICE;
    t->trans_Timeout = SVCTIMEOUT;

    err = DoTransaction(mgr, me, t);
    LoseEntity(mgr);
    if (!err)
        err = t->trans_Error;
    if (!err)
    {
        char name[SVCRESP_SIZE];

        PutField((UBYTE *)name, sizeof(name), (CONST_STRPTR)t->trans_ResponseData);
        result = FindEntity(remoteHost, name, srcEntity, &err);
        if (!result && !err)
            err = ENVOYERR_UNKNOWNENTITY;
    }

done:
    if (t)
        FreeTransaction(t);
    if (me)
        DeleteEntity(me);
    if (errptr)
        *errptr = result ? 0 : err;
    return result;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, LoseService,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),

/*  LOCATION */
        struct ServicesBase *, ServicesBase, 6, Services)

/*  FUNCTION
        Give up an entity obtained from FindServiceA().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (entity)
        LoseEntity(entity);

    AROS_LIBFUNC_EXIT
}
