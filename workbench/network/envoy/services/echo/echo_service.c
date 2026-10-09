/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: echo.service - the smallest Envoy service: a shared library with
          the xxx.service function table (devkit svc_lib.fd / svc_lib.doc).
          The Services Manager opens it and calls StartServiceA() for every
          FindService() client; the first call starts one server process
          that owns the public entity "Echo Service" and answers every
          transaction with the request data upper-cased, trans_Error set
          to the command. Later calls just return the entity name.

          The server process keeps the library open (lib_OpenCnt) for as
          long as it runs, so the manager's CloseLibrary() after
          StartServiceA() cannot expunge it.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/nipc.h>
#include <dos/dostags.h>
#include <envoy/nipc.h>
#include <envoy/services.h>
#include <envoy/errors.h>
#include <string.h>

#include "echo_intern.h"

static void Answer(struct Transaction *t)
{
    ULONG n = t->trans_ReqDataActual, i;
    UBYTE *src = t->trans_RequestData, *dst = t->trans_ResponseData;

    if (n > t->trans_RespDataLength)
        n = t->trans_RespDataLength;
    if (src && dst)
        for (i = 0; i < n; i++)
            dst[i] = (src[i] >= 'a' && src[i] <= 'z') ? src[i] - 32 : src[i];
    t->trans_RespDataActual = n;
    t->trans_Error = t->trans_Command;
}

static void ServerProcess(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct EchoBase *EchoBase = (struct EchoBase *)me->pr_Task.tc_UserData;
    struct Library *NIPCBase;
    struct Entity *entity = NULL;
    ULONG sig = 0;

    if ((NIPCBase = OpenLibrary(NIPCNAME, 50)))
        entity = CreateEntity(ENT_Name, (IPTR)ECHO_ENTITY_NAME, ENT_Public, TRUE, ENT_AllocSignal, (IPTR)&sig, TAG_DONE);

    if (!entity)
    {
        if (NIPCBase)
            CloseLibrary(NIPCBase);
        Forbid();
        EchoBase->eb_StartResult = 0;
        EchoBase->eb_Server = NULL;
        EchoBase->eb_Lib.lib_OpenCnt--;
        Signal(EchoBase->eb_Starter, SIGF_SINGLE);
        return;                                 /* Forbid() ends with the process */
    }

    EchoBase->eb_StartResult = 1;
    Signal(EchoBase->eb_Starter, SIGF_SINGLE);

    for (;;)
    {
        struct Transaction *t;
        ULONG got = Wait((1UL << sig) | SIGBREAKF_CTRL_C);

        if (got & SIGBREAKF_CTRL_C)
            break;
        while ((t = GetTransaction(entity)))
        {
            if (t->trans_Type == TYPE_SERVICING)
            {
                Answer(t);
                ReplyTransaction(t);
            }
        }
    }

    DeleteEntity(entity);
    CloseLibrary(NIPCBase);
    Forbid();
    EchoBase->eb_Server = NULL;
    EchoBase->eb_Lib.lib_OpenCnt--;
}

/*****************************************************************************

    NAME */
        AROS_LH0(void, RexxReserved,

/*  LOCATION */
        struct EchoBase *, EchoBase, 5, Echo)

/*  FUNCTION
        Reserved by the service library layout; does nothing.

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, StartServiceA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct EchoBase *, EchoBase, 6, Echo)

/*  FUNCTION
        Called by the Services Manager for a FindService() client. Starts
        the server process if it is not running and writes the name of
        the entity to connect to into the SSVC_EntityName buffer (64
        bytes). SSVC_UserName, SSVC_Password and SSVC_HostName are
        accepted and ignored: this service is open to everyone.

    RESULT
        0, or ENVOYERR_NORESOURCES when the server could not be started.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR entname = (STRPTR)GetTagData(SSVC_EntityName, 0, tagList);
    ULONG err = 0;

    ObtainSemaphore(&EchoBase->eb_Sem);
    if (!EchoBase->eb_Server)
    {
        struct TagItem ptags[] =
        {
            { NP_Entry,     (IPTR)ServerProcess     },
            { NP_Name,      (IPTR)"Echo Service"    },
            { NP_Priority,  0                       },
            { NP_StackSize, 32768                   },
            { NP_UserData,  (IPTR)EchoBase          },
            { TAG_DONE,     0                       }
        };

        EchoBase->eb_Starter = FindTask(NULL);
        EchoBase->eb_StartResult = 0;
        SetSignal(0, SIGF_SINGLE);
        Forbid();
        EchoBase->eb_Lib.lib_OpenCnt++;             /* held by the server process */
        EchoBase->eb_Server = CreateNewProc(ptags);
        Permit();
        if (EchoBase->eb_Server)
            Wait(SIGF_SINGLE);
        else
        {
            Forbid();
            EchoBase->eb_Lib.lib_OpenCnt--;
            Permit();
        }
        if (!EchoBase->eb_Server || !EchoBase->eb_StartResult)
        {
            EchoBase->eb_Server = NULL;
            err = ENVOYERR_NORESOURCES;
        }
    }
    if (!err)
        EchoBase->eb_Clients++;
    ReleaseSemaphore(&EchoBase->eb_Sem);

    if (!err && entname)
    {
        strncpy(entname, ECHO_ENTITY_NAME, 63);
        entname[63] = '\0';
    }
    return err;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, GetServiceAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct EchoBase *, EchoBase, 7, Echo)

/*  FUNCTION
        SVCAttrs_Name (STRPTR *) receives the service name; this is not a
        full service, so SVCAttrs_FullService (BOOL *) receives FALSE.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tagList;

    while ((tag = NextTagItem(&tstate)))
    {
        if (!tag->ti_Data)
            continue;
        switch (tag->ti_Tag)
        {
        case SVCAttrs_Name:         *(STRPTR *)tag->ti_Data = (STRPTR)ECHO_SERVICE_NAME; break;
        case SVCAttrs_FullService:  *(BOOL *)tag->ti_Data = FALSE; break;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, SetServiceAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct EchoBase *, EchoBase, 8, Echo)

/*  FUNCTION
        Not a full service: nothing can be set.

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, AttemptShutdown,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, desc, A0),
        AROS_LHA(ULONG, seconds, D0),

/*  LOCATION */
        struct EchoBase *, EchoBase, 9, Echo)

/*  FUNCTION
        Advisory shutdown request: stops the server process.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&EchoBase->eb_Sem);
    if (EchoBase->eb_Server)
        Signal(&EchoBase->eb_Server->pr_Task, SIGBREAKF_CTRL_C);
    ReleaseSemaphore(&EchoBase->eb_Sem);

    AROS_LIBFUNC_EXIT
}
