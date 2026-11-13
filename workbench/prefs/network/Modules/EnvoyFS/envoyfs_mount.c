/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    envoyfs_mount.c - Envoy Filesystem mountfile parsing and writing.

    An Envoy share is a DOSDriver mountfile with
    "Filesystem = L:EnvoyFileSystem" whose Unit string carries the mount
    parameters: "host¦export¦user¦password¦[flags¦]" (separator 0xA6, each
    field at most 79 characters - see the handler's ParseUnit and
    envoy/prefs/imports).  The password field holds "$" + the 11-character
    ECrypt form, never a clear password.

    MountedShare field mapping:
      ms_volume = export,  ms_secret = "$hash" (kept verbatim),
      ms_extra  = the optional flags characters (B/D/R), kept verbatim so
                  hand-written mountfiles round-trip unchanged.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

#include "envoyfs_intern.h"

/* Claim and parse one mountfile: TRUE if it is an Envoy FS mount. */
BOOL EFS_ReadMount(const struct NetPrefsMountInfo *mi, struct MountedShare *ms)
{
    STRPTR fields[4];
    ULONG sizes[4];
    const UBYTE *p;
    int f = 0;

    if (mi->nmi_Filesystem == NULL
        || strcasecmp(FilePart((STRPTR)mi->nmi_Filesystem), EFS_HANDLER) != 0)
        return FALSE;
    if (mi->nmi_Unit == NULL)
        return FALSE;

    fields[0] = ms->ms_host;    sizes[0] = sizeof(ms->ms_host);
    fields[1] = ms->ms_volume;  sizes[1] = sizeof(ms->ms_volume);
    fields[2] = ms->ms_user;    sizes[2] = sizeof(ms->ms_user);
    fields[3] = ms->ms_secret;  sizes[3] = sizeof(ms->ms_secret);

    /* Clone of the handler's ParseUnit; the 5th field (flags) is kept
     * verbatim in ms_extra rather than decoded. */
    p = (const UBYTE *)mi->nmi_Unit;
    while (f < 5 && *p)
    {
        ULONG n = 0;

        while (*p && *p != EFS_SEPARATOR)
        {
            if (f < 4)
            {
                if (n < sizes[f] - 1)
                    fields[f][n++] = *p;
            }
            else
            {
                if (n < sizeof(ms->ms_extra) - 1)
                    ms->ms_extra[n++] = *p;
            }
            p++;
        }
        if (f < 4)
            fields[f][n] = '\0';
        else
            ms->ms_extra[n] = '\0';
        if (*p)
            p++;
        f++;
    }

    return ms->ms_host[0] != '\0' && ms->ms_volume[0] != '\0';
}

/* Write one share's complete mountfile text - the same text the original
 * FilesystemImports program writes (envoy/prefs/imports MountText), with
 * the optional flags field appended when present. */
BOOL EFS_WriteMount(FILE *f, const struct MountedShare *ms)
{
    fprintf(f,
        "Filesystem = L:" EFS_HANDLER "\n"
        "StackSize = 4000\n"
        "Priority = 5\n"
        "GlobVec = -2\n"
        "Activate = 1\n"
        "Unit=\"%s%c%s%c%s%c%s%c",
        ms->ms_host, EFS_SEPARATOR, ms->ms_volume, EFS_SEPARATOR,
        ms->ms_user, EFS_SEPARATOR, ms->ms_secret, EFS_SEPARATOR);
    if (ms->ms_extra[0] != '\0')
        fprintf(f, "%s%c", ms->ms_extra, EFS_SEPARATOR);
    fprintf(f,
        "\"\n"
        "Surfaces = 0\n"
        "BlocksPerTrack = 0\n"
        "LowCyl = 0\n"
        "HighCyl = 0\n"
        "Device =\"Envoy FS\"\n"
        "DosType = 0x444f5380");
    return TRUE;
}

/* Defaults for a freshly added Envoy share. */
void EFS_InitShare(struct MountedShare *ms, CONST_STRPTR domain)
{
    strlcpy(ms->ms_device, EFS_DEFAULTDEV, sizeof(ms->ms_device));
    ms->ms_host[0]   = '\0';
    ms->ms_volume[0] = '\0';
    ms->ms_user[0]   = '\0';
    ms->ms_secret[0] = '\0';
    ms->ms_extra[0]  = '\0';
    ms->ms_active = TRUE;
}
