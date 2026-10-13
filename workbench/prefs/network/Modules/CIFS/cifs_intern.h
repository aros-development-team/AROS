/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    cifs_intern.h - Internal structures for the CIFS/SMB mounted-share module.
*/

#ifndef _NETPREFS_CIFS_INTERN_H_
#define _NETPREFS_CIFS_INTERN_H_

#include <exec/libraries.h>
#include <libraries/mui.h>

#include "netprefs_intern.h"
#include "netprefs_module.h"

/* The DOSDriver EHandler this module owns, and the default device name a
 * freshly added share starts out with (uniquified by the editor). */
#define CIFS_HANDLER     "smb-handler"
#define CIFS_DEFAULTDEV  "SMB0"

struct NetPrefsCIFSBase
{
    struct Library              npc_Lib;
    struct NetPrefsBase        *npc_NetPrefsBase;
    struct NetPrefsModule       npc_Module;
    struct MUI_CustomClass     *npc_WinClass;
};

#endif /* _NETPREFS_CIFS_INTERN_H_ */
