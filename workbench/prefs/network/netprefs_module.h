/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    netprefs_module.h - Protocol module interface for the Network prefs editor.

    Each protocol-address plugin (.netprefs library) exports ModuleInit() at
    vector 5.  ModuleInit receives a pointer to the NetPrefsBase library and
    registers a NetPrefsModule whose Startup callback will later create the
    MUI window class and call RegisterProtoHandler().
*/

#ifndef _NETPREFS_MODULE_H_
#define _NETPREFS_MODULE_H_

#include <exec/nodes.h>
#include <stdio.h>

struct NetPrefsBase;
struct ProtocolAddress;

/* Callback: called after all modules are loaded (create MUI classes, etc.) */
typedef void (*NETPREFS_STARTUP)(struct NetPrefsBase *);

/* Callback: write protocol config tokens to an interfaces file */
typedef void (*NETPREFS_WRITETOKENS)(FILE *f, struct ProtocolAddress *pa);

/*
 * NetPrefsModule - registered by each plugin during ModuleInit().
 * Startup is called after every plugin has been loaded so that modules
 * can safely use GetBase() to retrieve shared classes.
 */
struct NetPrefsModule
{
    struct Node             npm_Node;       /* ln_Name = module name, ln_Pri = sort order */
    NETPREFS_STARTUP        npm_Startup;    /* called after all modules loaded            */
    NETPREFS_STARTUP        npm_Shutdown;   /* called on app exit                         */
};

#endif /* _NETPREFS_MODULE_H_ */
