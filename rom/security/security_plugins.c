/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library plugin loading and per-task plugin contexts.
          Original code (c) 2000 Wez Furlong.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_plugins.h"
#include "security_server.h"
#include "security_memory.h"
#include "security_crypto.h"
#include "security_keys.h"
#include "security_support.h"

void UseModule(struct SecurityBase *secBase, secPluginModule *mod)
{
    ObtainSemaphore(&secBase->PluginModuleSem);
    mod->reference_count++;
    ReleaseSemaphore(&secBase->PluginModuleSem);
}

void ReleaseModule(struct SecurityBase *secBase, secPluginModule *mod)
{
    ObtainSemaphore(&secBase->PluginModuleSem);
    mod->reference_count--;
    ReleaseSemaphore(&secBase->PluginModuleSem);
}

/*
 * Locate the plugin header in a loaded seglist: scan the first 4KB of
 * every segment for the magic. The ELF loader's hunk head (size, next
 * pointer) is packed, and a section's data is then rounded up to the
 * section's alignment, so where the data starts after the next pointer
 * depends on the target: scan in ULONG steps, which lands on the header
 * (whose own alignment the loader honours) wherever it is.
 */
static struct secPluginHeader *FindPluginHeader(BPTR seglist)
{
    BPTR thisseg;

    for (thisseg = seglist; thisseg != BNULL; thisseg = *(BPTR *)BADDR(thisseg))
    {
        ULONG *addr = (ULONG *)BADDR(thisseg);
        ULONG *end = (ULONG *)((UBYTE *)addr + 4096);
        ULONG *pos;

        for (pos = addr + sizeof(BPTR) / sizeof(ULONG); pos < end; pos++)
        {
            struct secPluginHeader *hdr = (struct secPluginHeader *)pos;

            if (pos[0] == secPLUGIN_RECOGNITION && pos[1] == secPLUGIN_INTERFACE && hdr->Initialize && hdr->Terminate)
                return hdr;
        }
    }
    return NULL;
}

/* Build the plugin path: a plain name is looked for as SECURITY:<name>.secplugin,
 * then as SYS:Libs/Security/<name>.secplugin (where the distribution puts
 * its plugins); a path is used as given, plus the suffix */
static BOOL BuildPluginName(struct SecurityBase *secBase, CONST_STRPTR name, STRPTR fullname, ULONG size)
{
    BPTR lock;

    fullname[0] = '\0';
    if (strchr(name, ':') == NULL && strchr(name, '/') == NULL)
    {
        strncpy(fullname, secConfig_AssignName ":", size - 1);
        fullname[size - 1] = '\0';
        strncat(fullname, name, size - strlen(fullname) - 1);
        strncat(fullname, secPLUGIN_SUFFIX, size - strlen(fullname) - 1);
        if ((lock = Lock(fullname, SHARED_LOCK)))
        {
            UnLock(lock);
            return TRUE;
        }
        strncpy(fullname, secPLUGIN_DIRNAME "/", size - 1);
        fullname[size - 1] = '\0';
    }
    strncat(fullname, name, size - strlen(fullname) - 1);
    strncat(fullname, secPLUGIN_SUFFIX, size - strlen(fullname) - 1);
    return TRUE;
}

BOOL loadPlugin(struct SecurityBase *secBase, CONST_STRPTR name)
{
    char fullname[256];
    BPTR seglist;
    struct secPluginHeader *hdr;
    secPluginModule *mod;

    BuildPluginName(secBase, name, fullname, sizeof(fullname));
    D(bug(DEBUG_NAME_STR " %s(%s)\n", __func__, fullname);)

    if (!(seglist = LoadSeg(fullname)))
    {
        SIPTR args[2] = { (SIPTR)fullname, IoErr() };
        VLogF(secBase, LOGF_Flag_Type_Error, SECLOG_SUB_SERVER, "plugin '%s' could not be loaded (DOS error %ld)", args);
        return FALSE;
    }

    if (!(hdr = FindPluginHeader(seglist)))
    {
        SIPTR args[1] = { (SIPTR)fullname };
        VLogF(secBase, LOGF_Flag_Type_Error, SECLOG_SUB_SERVER, "'%s' is not a security.library plugin (no header)", args);
        UnLoadSeg(seglist);
        return FALSE;
    }

    if (!(mod = (secPluginModule *)MAlloc(sizeof(secPluginModule))))
    {
        UnLoadSeg(seglist);
        return FALSE;
    }
    mod->SegList = seglist;
    mod->header = hdr;
    mod->reference_count = 0;
    mod->SysBase = SysBase;
    strncpy((char *)mod->modulename, name, sizeof(mod->modulename) - 1);

    ObtainSemaphore(&secBase->PluginModuleSem);
    AddTail((struct List *)&secBase->PluginModuleList, (struct Node *)mod);
    ReleaseSemaphore(&secBase->PluginModuleSem);

    /* The init function runs in the server's context */
    if (FindTask(NULL) == (struct Task *)secBase->Server)
    {
        if (mod->header->Initialize((struct Library *)secBase, mod))
            return TRUE;
    }
    else if (SendServerPacket(secBase, secSAction_InitModule, (SIPTR)mod, 0, 0, 0))
        return TRUE;

    {
        SIPTR args[1] = { (SIPTR)fullname };
        VLogF(secBase, LOGF_Flag_Type_Error, SECLOG_SUB_SERVER, "plugin '%s' failed to initialise", args);
    }
    unloadPlugin(secBase, mod);
    return FALSE;
}

void unloadPlugin(struct SecurityBase *secBase, secPluginModule *mod)
{
    BPTR seglist = mod->SegList;

    if (FindTask(NULL) == (struct Task *)secBase->Server)
        mod->header->Terminate();
    else
        SendServerPacket(secBase, secSAction_FiniModule, (SIPTR)mod, 0, 0, 0);

    ObtainSemaphore(&secBase->PluginModuleSem);
    Remove((struct Node *)mod);
    ReleaseSemaphore(&secBase->PluginModuleSem);

    ObtainSemaphore(&secBase->TaskOwnerSem);
    FreeModuleContext(secBase, mod);
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    Free(mod, sizeof(secPluginModule));
    if (seglist)
        UnLoadSeg(seglist);
}

BOOL unloadPluginName(struct SecurityBase *secBase, CONST_STRPTR name)
{
    secPluginModule *mod, *found = NULL;

    ObtainSemaphore(&secBase->PluginModuleSem);
    ForeachNode(&secBase->PluginModuleList, mod)
    {
        if (!Stricmp((CONST_STRPTR)mod->modulename, name))
        {
            found = mod;
            break;
        }
    }
    ReleaseSemaphore(&secBase->PluginModuleSem);

    if (found && found->reference_count == 0)
    {
        unloadPlugin(secBase, found);
        return TRUE;
    }
    return FALSE;
}

/* (Un)register a handler op-table with the subsystem it belongs to */
ULONG regHandler(struct SecurityBase *secBase, BOOL reg, struct plugin_ops *ops)
{
    if (!ops || !ops->module)
        return secpiFALSE;

    switch (ops->HandlerType)
    {
    case ID_PLUGIN_ENCRYPTION:
        return reg ? RegisterEncryptionHandler(secBase, ops) : UnRegisterEncryptionHandler(secBase, ops);
    case ID_PLUGIN_HWAUTH:
        return reg ? RegisterHwAuthHandler(secBase, ops) : UnRegisterHwAuthHandler(secBase, ops);
    default:
        return secpiNOTSUPP;
    }
}

/*
 * Contexts
 *
 * A context is a block of memory a plugin associates with a caller (e.g.
 * getpwent's position). Contexts are attached to the task that opened the
 * library, child tasks inherit them, and every OpenLibrary()/CloseLibrary()
 * pushes/pops a level so nested LoadSeg/RunCommand combinations work.
 */
void FreeAllContext(struct secTaskNode *node)
{
    struct secContextNode *con;
    struct secContextList *clist;

    while ((clist = (struct secContextList *)RemHead((struct List *)&node->Context)) != NULL)
    {
        while ((con = (struct secContextNode *)RemHead((struct List *)&clist->Context)) != NULL)
            FreeV(con);
        Free(clist, sizeof(struct secContextList));
    }
}

/* Free all context allocated by (module). Expensive. */
void FreeModuleContext(struct SecurityBase *secBase, secPluginModule *module)
{
    struct MinNode *tnode, *cnode, *cnext, *clnode;
    int i;

    for (i = 0; i < TASKHASHVALUE; i++)
    {
        ForeachNode(&secBase->TaskOwnerList[i], tnode)
        {
            struct secTaskNode *tasknode = TASKNODE_FROM_LISTNODE(tnode);

            ForeachNode(&tasknode->Context, clnode)
            {
                struct secContextList *clist = (struct secContextList *)clnode;

                for (cnode = clist->Context.mlh_Head; (cnext = cnode->mln_Succ); cnode = cnext)
                {
                    struct secContextNode *con = (struct secContextNode *)cnode;
                    if (con->mod == module)
                    {
                        Remove((struct Node *)con);
                        FreeV(con);
                    }
                }
            }
        }
    }
}

APTR AllocateContext(struct secTaskNode *node, secPluginModule *module, ULONG id, ULONG size)
{
    struct secContextNode *con;
    struct secContextList *clist;

    if (IsMinListEmpty(&node->Context))
        return NULL;
    if ((con = (struct secContextNode *)MAllocV(sizeof(struct secContextNode) + size)))
    {
        con->mod = module;
        con->id = id;
        clist = (struct secContextList *)node->Context.mlh_Head;
        AddHead((struct List *)&clist->Context, (struct Node *)con);
        return (APTR)(con + 1);
    }
    return NULL;
}

struct secTaskNode *FindContextOwner(struct SecurityBase *secBase, struct Task *caller)
{
    struct secTaskNode *ret = FindTaskNode(secBase, caller);

    while (ret)
    {
        if (!IsMinListEmpty(&ret->Context))
            return ret;
        ret = ret->Parent;
    }
    return NULL;
}

APTR FindContext(struct secTaskNode *node, secPluginModule *module, ULONG id)
{
    struct secContextList *clist;
    struct MinNode *n;

    if (!node || IsMinListEmpty(&node->Context))
        return NULL;
    clist = (struct secContextList *)node->Context.mlh_Head;
    ForeachNode(&clist->Context, n)
    {
        struct secContextNode *con = (struct secContextNode *)n;
        if ((con->mod == module) && (con->id == id))
            return (APTR)(con + 1);
    }
    return NULL;
}

/* Push a new level of context onto (caller): called when it opens the library */
void PushContext(struct SecurityBase *secBase, struct Task *caller)
{
    struct secContextList *clist;
    struct secTaskNode *tnode;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((tnode = FindOrCreateTaskNode(secBase, caller)) &&
        (clist = (struct secContextList *)MAlloc(sizeof(struct secContextList))))
    {
        NEWLIST((struct List *)&clist->Context);
        AddHead((struct List *)&tnode->Context, (struct Node *)clist);
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
}

/* Pop the head context level of (caller): called when it closes the library */
void PopContext(struct SecurityBase *secBase, struct Task *caller)
{
    struct secContextList *clist;
    struct secTaskNode *tnode;
    struct secContextNode *con;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((tnode = FindTaskNode(secBase, caller)) &&
        (clist = (struct secContextList *)RemHead((struct List *)&tnode->Context)))
    {
        while ((con = (struct secContextNode *)RemHead((struct List *)&clist->Context)) != NULL)
            FreeV(con);
        Free(clist, sizeof(struct secContextList));
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
}
