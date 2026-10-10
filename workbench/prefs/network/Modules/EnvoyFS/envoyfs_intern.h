/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    envoyfs_intern.h - Internal structures for the Envoy Filesystem
    mounted-share module.
*/

#ifndef _NETPREFS_ENVOYFS_INTERN_H_
#define _NETPREFS_ENVOYFS_INTERN_H_

#include <exec/libraries.h>
#include <libraries/mui.h>

#include "netprefs_intern.h"
#include "netprefs_module.h"

/* The DOSDriver Filesystem this module owns (matched on the file part of
 * the Filesystem= path), and the default device name for a new share
 * (replaced by "<host>-<export>" when the device field is left empty). */
#define EFS_HANDLER      "EnvoyFileSystem"
#define EFS_DEFAULTDEV   "EFS0"

/* Unit string: "host¦export¦user¦password¦[flags¦]" */
#define EFS_SEPARATOR    0xA6

struct NetPrefsEFSBase
{
    struct Library              npe_Lib;
    struct NetPrefsBase        *npe_NetPrefsBase;
    struct NetPrefsModule       npe_Module;
    struct MUI_CustomClass     *npe_WinClass;
};

/* Envoy libraries - all OPTIONAL ("show but degrade"): parsing and writing
 * mountfiles must work with none of them installed.  Opened at Startup,
 * closed at Shutdown (envoyfs_init.c); NULL when absent. */
extern struct Library *NIPCBase;        /* discovery transactions      */
extern struct Library *ServicesBase;    /* FindService                 */
extern struct Library *AccountsBase;    /* ECrypt password hashing     */
extern struct Library *EnvoyBase;       /* HostRequest                 */

#endif /* _NETPREFS_ENVOYFS_INTERN_H_ */
