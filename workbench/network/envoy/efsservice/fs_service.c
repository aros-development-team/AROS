/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service - the xxx.service function table (devkit
          svc_lib.fd). StartServiceA() starts the one server process on
          first use and always answers with the entity name "Filesystem"
          (re/spec/efs-protocol.md §1.3, §5.1); credentials travel in the
          mount transaction, not here.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/dostags.h>
#include <envoy/services.h>
#include <envoy/errors.h>
#include <string.h>

#include "fs_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH0(void, RexxReserved,

/*  LOCATION */
        struct FSServiceBase *, FSServiceBase, 5, FSService)

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
        struct FSServiceBase *, FSServiceBase, 6, FSService)

/*  FUNCTION
        Called by the Services Manager for every FindService() client.
        Starts the EFS server process if it is not running and writes
        "Filesystem" into the SSVC_EntityName buffer. SSVC_UserName,
        SSVC_Password and SSVC_HostName are ignored: the mount transaction
        carries the credentials.

    RESULT
        0, or ENVOYERR_NORESOURCES (the original answers 103) when the
        server could not be started.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR entname = (STRPTR)GetTagData(SSVC_EntityName, 0, tagList);
    ULONG err = 0;

    ObtainSemaphore(&FSServiceBase->fb_Sem);
    if (!FSServiceBase->fb_Server)
    {
        struct TagItem ptags[] =
        {
            { NP_Entry,     (IPTR)ServerProcess      },
            { NP_Name,      (IPTR)"EFS_Server"       },
            { NP_Priority,  0                        },
            { NP_StackSize, 131072                   },
            { NP_UserData,  (IPTR)FSServiceBase      },
            { TAG_DONE,     0                        }
        };

        FSServiceBase->fb_Starter = FindTask(NULL);
        FSServiceBase->fb_StartResult = 0;
        SetSignal(0, SIGF_SINGLE);
        Forbid();
        FSServiceBase->fb_Lib.lib_OpenCnt++;         /* held by the server process */
        FSServiceBase->fb_Server = CreateNewProc(ptags);
        Permit();
        if (FSServiceBase->fb_Server)
            Wait(SIGF_SINGLE);
        else
        {
            Forbid();
            FSServiceBase->fb_Lib.lib_OpenCnt--;
            Permit();
        }
        if (!FSServiceBase->fb_Server || !FSServiceBase->fb_StartResult)
        {
            FSServiceBase->fb_Server = NULL;
            err = ENVOYERR_NORESOURCES;
        }
    }
    if (!err)
        FSServiceBase->fb_Clients++;
    ReleaseSemaphore(&FSServiceBase->fb_Sem);

    if (!err && entname)
    {
        strncpy(entname, FS_ENTITY_NAME, 63);
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
        struct FSServiceBase *, FSServiceBase, 7, FSService)

/*  FUNCTION
        SVCAttrs_Name (STRPTR *) receives "Filesystem"; SVCAttrs_FullService
        (BOOL *) receives TRUE: AttemptShutdown() is honoured.

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
        case SVCAttrs_Name:         *(STRPTR *)tag->ti_Data = (STRPTR)FS_SERVICE_NAME; break;
        case SVCAttrs_FullService:  *(BOOL *)tag->ti_Data = TRUE; break;
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
        struct FSServiceBase *, FSServiceBase, 8, FSService)

/*  FUNCTION
        Nothing can be set; the service name is fixed.

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
        struct FSServiceBase *, FSServiceBase, 9, FSService)

/*  FUNCTION
        Advisory shutdown request: stops the server process, which releases
        every mount's locks and file handles.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&FSServiceBase->fb_Sem);
    if (FSServiceBase->fb_Server)
        Signal(&FSServiceBase->fb_Server->pr_Task, SIGBREAKF_CTRL_C);
    ReleaseSemaphore(&FSServiceBase->fb_Sem);

    AROS_LIBFUNC_EXIT
}
