/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_volumes.h"

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, secIsVolumeSecured,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, volname, A0),
        AROS_LHA(ULONG, dostype, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 60, Security)

/*  FUNCTION
        Filesystem API: should the handler of this volume enforce
        ownership? TRUE for multi-user dostypes (muF#?, muAF, muPF) and for
        volumes that carry a valid key file (secKey_FileName) in their root
        directory, which is how a filesystem without a multi-user dostype
        (e.g. SFS) is marked as multi-user. The server (re)scans volumes at
        startup and whenever secFSRendezVous() is called, so a handler should
        call secFSRendezVous() when a volume comes online and ask again.

    INPUTS
        volname - the volume or device name, with or without ':' (may be NULL)
        dostype - the volume's dostype

    RESULT
        TRUE if the handler should enforce. Note that enforcement only
        makes sense while secIsConfigured() is TRUE as well.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secNativeVolume *nv;
    BOOL res = FALSE;
    ULONG len;

    if (IsSecFSDosType(dostype))
        return TRUE;
    if (!volname || !secBase->sec_AfterDOSDone)
        return FALSE;

    len = strlen(volname);
    if (len && volname[len - 1] == ':')
        len--;

    ObtainSemaphoreShared(&secBase->VolumesSem);
    ForeachNode(&secBase->NativeVolumes, nv)
    {
        if (strlen(nv->Name) == len && !Strnicmp(nv->Name, volname, len))
        {
            res = TRUE;
            break;
        }
    }
    ReleaseSemaphore(&secBase->VolumesSem);
    return res;

    AROS_LIBFUNC_EXIT
} /* secIsVolumeSecured */
