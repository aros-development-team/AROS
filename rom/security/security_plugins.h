#ifndef _SECURITY_PLUGINS_H
#define _SECURITY_PLUGINS_H

#include "security_task.h"

extern void UseModule(struct SecurityBase*secBase, secPluginModule * mod);
extern void ReleaseModule(struct SecurityBase*secBase, secPluginModule * mod);

extern BOOL loadPlugin(struct SecurityBase *secBase, STRPTR name);
extern void unloadPlugin(struct SecurityBase *secBase, secPluginModule * mod);

/* Internal functions for plugin contexts */
extern APTR AllocateContext(struct secTaskNode * node, secPluginModule * module, ULONG id, ULONG size);
extern APTR FindContext(struct secTaskNode * node, secPluginModule * module, ULONG id);
extern struct secTaskNode * FindContextOwner(struct SecurityBase*secBase, struct Task * caller);

extern void FreeModuleContext(struct SecurityBase *secBase, secPluginModule * module);

/* Free All Context associated with a task */
extern void FreeAllContext(struct secTaskNode * node);
extern void PopContext(struct SecurityBase*secBase, struct Task * caller);
extern void PushContext(struct SecurityBase*secBase, struct Task * caller);

#endif /* _SECURITY_PLUGINS_H */