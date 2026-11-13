/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    netprefs_intern.h - Internal structures for the NetPrefs registration
    library created at runtime by the Network prefs editor.
*/

#ifndef _NETPREFS_INTERN_H_
#define _NETPREFS_INTERN_H_

#include <exec/libraries.h>
#include <libraries/mui.h>
#include <stdio.h>

#include "netprefs_module.h"
#include "protocols.h"

/*
 * ProtoHandlerNode - registered by a module's Startup callback via
 * RegisterProtoHandler().  The main app iterates this list to discover
 * protocol address window classes and WriteTokens functions.
 */
struct ProtoHandlerNode
{
    struct Node             ph_Node;        /* ln_Name = display name ("IPv4","IPv6") */
    enum ProtocolFamily     ph_Family;      /* PROTO_FAMILY_IPV4 or _IPV6             */
    UBYTE                   ph_ID;          /* ID assigned at registration; used as   */
                                            /* ln_Type of this plugin's address nodes */
    struct MUI_CustomClass *ph_WinClass;    /* PAWinClass subclass for config window   */
    NETPREFS_WRITETOKENS    ph_WriteTokens; /* writes config tokens to FILE            */
    NETPREFS_READTOKENS     ph_ReadTokens;  /* claims/parses a token -> address node   */
    NETPREFS_DISPLAY        ph_Display;     /* formats a list-column string            */
};

/*
 * FSHandlerNode - registered by a module's Startup callback via
 * RegisterFSHandler().  The main app iterates this list to discover
 * mounted-share (filesystem) types for the "Mounted Shares" page.
 */
struct FSHandlerNode
{
    struct Node             fsh_Node;       /* ln_Name = display name, ln_Pri = pri   */
    UBYTE                   fsh_ID;         /* assigned at registration; used as      */
                                            /* ln_Type of this handler's share nodes  */
    struct MUI_CustomClass *fsh_WinClass;   /* PAWinClass subclass for the edit window */
    NETPREFS_READMOUNT      fsh_ReadMount;  /* claims + parses one mountfile           */
    NETPREFS_WRITEMOUNT     fsh_WriteMount; /* writes one share's mountfile            */
    NETPREFS_INITSHARE      fsh_InitShare;  /* defaults for a newly added share        */
    NETPREFS_FSDISPLAY      fsh_Display;    /* optional column text override           */
};

/*
 * NetPrefsIntBase - wraps a named base registration (like SysexpIntBase).
 */
struct NetPrefsIntBase
{
    struct Node             npib_Node;      /* ln_Name = base ID string */
    APTR                    npib_Base;      /* arbitrary pointer        */
};

/*
 * NetPrefsIntModule - wraps a NetPrefsModule with the module library base.
 */
struct NetPrefsIntModule
{
    struct NetPrefsModule   npim_Module;
    APTR                    npim_ModuleBase; /* module's Library base */
};

/*
 * NetPrefsBase - the in-process library that modules call into.
 * Created by the main app via MakeLibrary().
 */
struct NetPrefsBase
{
    struct Library          npb_Lib;
    struct List             npb_GenericBases;    /* NetPrefsIntBase nodes       */
    struct List             npb_Modules;         /* NetPrefsIntModule nodes     */
    struct List             npb_ProtoHandlers;   /* ProtoHandlerNode nodes      */
    struct List             npb_FSHandlers;      /* FSHandlerNode nodes         */
    UBYTE                   npb_NextFSID;        /* fsh_ID allocator (session)  */
};

#endif /* _NETPREFS_INTERN_H_ */
