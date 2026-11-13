/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    cifs_mount.c - CIFS/SMB mountfile parsing and writing (moved from the
    editor's prefsdata.c when the Mounted Shares page was modularised).

    A CIFS share is a DOSDriver mountfile with "EHandler = smb-handler" whose
    Control string carries the smb-handler arguments.  The editor's generic
    scanner pre-parses the Mountfile grammar (struct NetPrefsMountInfo); this
    module only interprets the Control string.

    MountedShare field mapping:
      ms_volume = share name,  ms_extra = workgroup,  ms_secret = password.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

#include "cifs_intern.h"

/* smb-handler's Control argument template (see AROS smb-handler docs) */
enum
{
    ARG_WORKGROUP,
    ARG_USERNAME,
    ARG_PASSWORD,
    ARG_CHANGECASE,
    ARG_CASESENSITIVE,
    ARG_OMITHIDDEN,
    ARG_QUIET,
    ARG_CLIENTNAME,
    ARG_SERVERNAME,
    ARG_DEVICENAME,
    ARG_VOLUMENAME,
    ARG_CACHESIZE,
    ARG_DEBUGLEVEL,
    ARG_TIMEZONEOFFSET,
    ARG_DSTOFFSET,
    ARG_TRANSLATIONFILE,
    ARG_SERVICE,
    NUM_CONTROLARGS
};

static const TEXT control_template[] =
    "DOMAIN=WORKGROUP/K,"
    "USER=USERNAME/K,"
    "PASSWORD/K,"
    "CHANGECASE/S,"
    "CASE=CASESENSITIVE/S,"
    "OMITHIDDEN/S,"
    "QUIET/S,"
    "CLIENT=CLIENTNAME/K,"
    "SERVER=SERVERNAME/K,"
    "DEVICE=DEVICENAME/K,"
    "VOLUME=VOLUMENAME/K,"
    "CACHE=CACHESIZE/N/K,"
    "DEBUGLEVEL=DEBUG/N/K,"
    "TZ=TIMEZONEOFFSET/N/K,"
    "DST=DSTOFFSET/N/K,"
    "TRANSLATE=TRANSLATIONFILE/K,"
    "SERVICE/A";

/* Claim and parse one mountfile: TRUE if it is a CIFS mount. */
BOOL CIFS_ReadMount(const struct NetPrefsMountInfo *mi, struct MountedShare *ms)
{
    IPTR control_args[NUM_CONTROLARGS] = {0};
    struct RDArgs *rd, *res;
    STRPTR buf;
    ULONG len;
    BOOL success = FALSE;

    if (mi->nmi_EHandler == NULL
        || strcasecmp(mi->nmi_EHandler, CIFS_HANDLER) != 0)
        return FALSE;
    if (mi->nmi_Control == NULL)
        return FALSE;

    /* ReadArgs wants a buffer it may scan past the end of a token on; parse
     * our own copy rather than the caller's string. */
    len = strlen(mi->nmi_Control);
    buf = AllocVec(len + 2, MEMF_ANY | MEMF_CLEAR);
    if (buf == NULL)
        return FALSE;
    CopyMem((APTR)mi->nmi_Control, buf, len);

    rd = AllocDosObject(DOS_RDARGS, NULL);
    if (rd != NULL)
    {
        rd->RDA_Source.CS_Buffer = (UBYTE *)buf;
        rd->RDA_Source.CS_Length = len;
        rd->RDA_Flags = RDAF_NOPROMPT;

        res = ReadArgs(control_template, (IPTR *)control_args, rd);
        if (res != NULL)
        {
            CONST_STRPTR p = (CONST_STRPTR)control_args[ARG_SERVICE];

            /* SERVICE = "//host/share" */
            if (p != NULL && p[0] == '/' && p[1] == '/')
            {
                STRPTR svc = FilePart((STRPTR)p);
                ULONG n = (ULONG)(svc - p);

                strlcpy(ms->ms_volume, svc, sizeof(ms->ms_volume));
                n = (n >= 3) ? n - 3 : 0;    /* strip "//" and the '/' */
                if (n >= sizeof(ms->ms_host))
                    n = sizeof(ms->ms_host) - 1;
                CopyMem((APTR)(p + 2), ms->ms_host, n);
                ms->ms_host[n] = '\0';

                p = (CONST_STRPTR)control_args[ARG_USERNAME];
                strlcpy(ms->ms_user, p != NULL ? p : (CONST_STRPTR)"",
                        sizeof(ms->ms_user));
                p = (CONST_STRPTR)control_args[ARG_WORKGROUP];
                strlcpy(ms->ms_extra, p != NULL ? p : (CONST_STRPTR)"",
                        sizeof(ms->ms_extra));
                p = (CONST_STRPTR)control_args[ARG_PASSWORD];
                strlcpy(ms->ms_secret, p != NULL ? p : (CONST_STRPTR)"",
                        sizeof(ms->ms_secret));

                success = TRUE;
            }
            FreeArgs(res);
        }
        FreeDosObject(DOS_RDARGS, rd);
    }
    FreeVec(buf);

    return success;
}

/* Write one share's complete mountfile text. */
BOOL CIFS_WriteMount(FILE *f, const struct MountedShare *ms)
{
    fprintf(f, "EHandler = " CIFS_HANDLER "\nActivate = 1\n");
    fprintf(f, "Control = \"");
    if (ms->ms_user[0] != '\0')
        fprintf(f, "USER=*\"%s*\" ", ms->ms_user);
    if (ms->ms_extra[0] != '\0')
        fprintf(f, "WORKGROUP=*\"%s*\" ", ms->ms_extra);
    if (ms->ms_secret[0] != '\0')
        fprintf(f, "PASSWORD=*\"%s*\" ", ms->ms_secret);
    fprintf(f, "SERVICE=*\"//%s/%s*\"\"\n", ms->ms_host, ms->ms_volume);
    return TRUE;
}

/* Defaults for a freshly added CIFS share. */
void CIFS_InitShare(struct MountedShare *ms, CONST_STRPTR domain)
{
    if (domain == NULL || domain[0] == '\0')
        domain = "workgroup";

    strlcpy(ms->ms_device, CIFS_DEFAULTDEV, sizeof(ms->ms_device));
    ms->ms_host[0] = '\0';
    strlcpy(ms->ms_volume, "share", sizeof(ms->ms_volume));
    strlcpy(ms->ms_user, "guest", sizeof(ms->ms_user));
    ms->ms_secret[0] = '\0';
    strlcpy(ms->ms_extra, domain, sizeof(ms->ms_extra));
    ms->ms_active = TRUE;
}
