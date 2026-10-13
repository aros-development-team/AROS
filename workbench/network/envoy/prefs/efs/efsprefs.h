#ifndef EFSPREFS_H
#define EFSPREFS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Exports - the exports of ENV:Envoy/EFS.prefs
          (re/spec/efs-protocol.md §6): FORM PREF { PRHD, VOLM ... }.
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>

#define EFS_PREFS_PATH      "Envoy/EFS.prefs"
#define EFS_PREFS_ENV       "ENV:Envoy/EFS.prefs"
#define EFS_PREFS_ENVARC    "ENVARC:Envoy/EFS.prefs"

/* VOLM flag bits (§6.3) */
#define EXPF_SNAPSHOT       0x01    /* Disk.info not protected            */
#define EXPF_LEFTOUT        0x02    /* .backdrop not hidden               */
#define EXPF_FULLSECURITY   0x04    /* Security "Mounts & Files"          */
#define EXPF_NOSECURITY     0x08    /* Security "None"                    */
#define EXPF_EMULATEEXALL   0x10
#define EXPF_READONLY       0x20
#define EXPF_REMOVABLE      0x40

#define EXP_MAXACCESS       64
#define EXP_NAMELEN         64      /* both string fields of a VOLM chunk */

struct Export
{
    struct MinNode  Node;
    char            Path[EXP_NAMELEN];
    char            Name[EXP_NAMELEN];
    ULONG           Flags;
    ULONG           NumAccess;
    UWORD           AccessID[EXP_MAXACCESS];
    UBYTE           AccessGroup[EXP_MAXACCESS];        /* 0 = user ID, 1 = group ID */
    char            AccessName[EXP_MAXACCESS][40];     /* display text, filled by the editor */
};

extern struct MinList Exports;

void EfsPrefs_Init(void);
void EfsPrefs_Free(void);
struct Export *EfsPrefs_New(void);
BOOL EfsPrefs_ImportFH(BPTR fh);
BOOL EfsPrefs_ExportFH(BPTR fh);
BOOL EfsPrefs_HandleArgs(CONST_STRPTR from, BOOL use, BOOL save);
void EfsPrefs_Normalise(struct Export *e);

#endif
