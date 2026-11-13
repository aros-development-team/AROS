/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library plugin handling
*/
#ifndef _SECURITY_PLUGINS_H
#define _SECURITY_PLUGINS_H

#include "security_task.h"

struct SecurityBase;

extern void UseModule(struct SecurityBase *secBase, secPluginModule *mod);
extern void ReleaseModule(struct SecurityBase *secBase, secPluginModule *mod);

/* These run in the server's context */
extern BOOL loadPlugin(struct SecurityBase *secBase, CONST_STRPTR name);
extern void unloadPlugin(struct SecurityBase *secBase, secPluginModule *mod);
extern BOOL unloadPluginName(struct SecurityBase *secBase, CONST_STRPTR name);

/* Handler registration */
extern ULONG regHandler(struct SecurityBase *secBase, BOOL reg, struct plugin_ops *ops);

/* Plugin contexts (TaskOwnerSem held by caller unless noted) */
extern APTR AllocateContext(struct secTaskNode *node, secPluginModule *module, ULONG id, ULONG size);
extern APTR FindContext(struct secTaskNode *node, secPluginModule *module, ULONG id);
extern struct secTaskNode *FindContextOwner(struct SecurityBase *secBase, struct Task *caller);
extern void FreeModuleContext(struct SecurityBase *secBase, secPluginModule *module);
extern void FreeAllContext(struct secTaskNode *node);
/* These obtain the semaphore themselves */
extern void PopContext(struct SecurityBase *secBase, struct Task *caller);
extern void PushContext(struct SecurityBase *secBase, struct Task *caller);

#endif /* _SECURITY_PLUGINS_H */
