/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service - THIN compatibility stub (devkit svc_lib.fd
          layout).  The EFS server itself now runs as the standalone envoyfs
          daemon, managed by the AROSTCP service framework
          (db/services.d/envoyfs); it owns the public entity "Filesystem"
          and, when Envoy's ServicesManager is not running, answers
          FindService() requests itself.

          This stub keeps the original discovery path working when the REAL
          ServicesManager owns the "Services Manager" entity: its
          StartServiceA() no longer starts a server process - it answers
          with the entity name "Filesystem" when the daemon is running and
          fails otherwise (start the daemon via the service framework).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/nipc.h>
#include <envoy/nipc.h>
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
        Writes "Filesystem" into the SSVC_EntityName buffer when the envoyfs
        daemon's public entity exists.  SSVC_UserName, SSVC_Password and
        SSVC_HostName are ignored: the mount transaction carries the
        credentials.

    RESULT
        0, or ENVOYERR_UNKNOWNSERVICE when the daemon is not running
        (enable it in the AROSTCP service manager: db/services.d/envoyfs).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Library *UtilityBase = OpenLibrary("utility.library", 36);
    struct Library *NIPCBase = OpenLibrary(NIPCNAME, 50);
    STRPTR entname = NULL;
    ULONG err = ENVOYERR_UNKNOWNSERVICE;

    if (UtilityBase != NULL)
        entname = (STRPTR)GetTagData(SSVC_EntityName, 0, tagList);

    if (NIPCBase != NULL)
    {
        struct Entity *me;

        if ((me = CreateEntity(ENT_AllocSignal, 0, TAG_DONE)))
        {
            ULONG finderr = 0;
            struct Entity *daemon =
                FindEntity(NULL, (STRPTR)FS_ENTITY_NAME, me, &finderr);

            if (daemon != NULL)
            {
                LoseEntity(daemon);
                err = 0;
            }
            DeleteEntity(me);
        }
        else
            err = ENVOYERR_NORESOURCES;
    }
    else
        err = ENVOYERR_NORESOURCES;

    if (!err && entname)
    {
        strncpy(entname, FS_ENTITY_NAME, 63);
        entname[63] = '\0';
    }

    if (NIPCBase)
        CloseLibrary(NIPCBase);
    if (UtilityBase)
        CloseLibrary(UtilityBase);
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

    struct Library *UtilityBase = OpenLibrary("utility.library", 36);
    struct TagItem *tag, *tstate = tagList;

    if (UtilityBase == NULL)
        return;
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
    CloseLibrary(UtilityBase);

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
        Advisory shutdown request: stops the envoyfs daemon (found by its
        public marker port), which releases every mount's locks and file
        handles.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MsgPort *port;

    Forbid();
    if ((port = FindPort(FS_DAEMON_PORT)))
        Signal(port->mp_SigTask, SIGBREAKF_CTRL_C);
    Permit();

    AROS_LIBFUNC_EXIT
}
