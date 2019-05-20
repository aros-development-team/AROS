
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_plugins.h"
#include "security_server.h"
#include "security_memory.h"


void UseModule(struct SecurityBase *secBase, secPluginModule * mod)
{
    D(bug( DEBUG_NAME_STR " %s: using module %p\n", __func__, mod));
    ObtainSemaphore(&secBase->PluginModuleSem);
    mod->reference_count++;
    ReleaseSemaphore(&secBase->PluginModuleSem);
}

void ReleaseModule(struct SecurityBase *secBase, secPluginModule * mod)
{
    D(bug( DEBUG_NAME_STR " %s: releasing module %p\n", __func__, mod));
    ObtainSemaphore(&secBase->PluginModuleSem);
    mod->reference_count--;
    ReleaseSemaphore(&secBase->PluginModuleSem);
}

BOOL loadPlugin(struct SecurityBase *secBase, STRPTR name)
{
    char fullname[256];
    BPTR seglist;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    strncpy(fullname, name, sizeof(fullname));
    strncat(fullname, secPLUGIN_SUFFIX, sizeof(fullname));
    D(bug( DEBUG_NAME_STR " %s(%s)\n", __func__, fullname));
    seglist = LoadSeg(fullname);
    if (seglist)	{
        BPTR thisseg = seglist;
        struct secPluginHeader * hdr = NULL;
        D(bug( DEBUG_NAME_STR " %s: seglist loaded\n", __func__));	
        /* Scan the seglist for magic identifier */
        while((hdr == NULL) && (thisseg != BNULL))	{
            IPTR size, pos, *addr;
            addr = BADDR(thisseg);
            size = addr[-1];
            D(bug( DEBUG_NAME_STR " %s: scanning hunk of size %ld for plugin header\n", __func__, size));
            /* start at 1 to skip pointer to next segment */
            for (pos = 1; pos < size; pos++)	{
                if (addr[pos] == secPLUGIN_RECOGNITION)	{
                    /* Found the plugin */
                    hdr = (struct secPluginHeader*)&addr[pos];
                    break;
                }
            }
            thisseg = (BPTR)addr[0];	/* get next segment in the list */
        }
        if (hdr != NULL)	{
            D(bug( DEBUG_NAME_STR " %s: plugin header found at %lx\n", __func__, hdr));
            if (hdr->Version == MUFS_PLUGIN_INTERFACE)	{
                secPluginModule * mod = (secPluginModule *)MAlloc(sizeof(secPluginModule));
                D(bug( DEBUG_NAME_STR " %s: Version matches\n", __func__));
                if (mod)	{
                    D(bug( DEBUG_NAME_STR " %s: Allocated module\n", __func__));
                    mod->SegList = seglist;
                    mod->header = hdr;
                    mod->reference_count = 0;
                    strncpy(mod->modulename, name, sizeof(mod->modulename));
                    ObtainSemaphore(&secBase->PluginModuleSem);
                    AddTail((struct List*)&secBase->PluginModuleList, (struct Node*)mod);
                    ReleaseSemaphore(&secBase->PluginModuleSem);
                    
                    /* Loaded and added to the list - get the server to call the
                     * init function (unless we are the server: do it now) */

                    if ((struct Process*)FindTask(NULL) == secBase->Server)	{
                        if (mod->header->Initialize((struct Library *)secBase, mod))
                            return TRUE;
                    }
                    else	{
                        D(bug( DEBUG_NAME_STR " %s: Asking server to init module\n", __func__));
                        if ((BOOL)SendServerPacket(secBase, secSAction_InitModule, (SIPTR)mod, (SIPTR)NULL, (SIPTR)NULL, (SIPTR)NULL))
                            return TRUE;
                    }
                    D(bug( DEBUG_NAME_STR " %s: Failed :-(\n", __func__));
                    
                    /* Failed to init for some reason - get the server to call the
                     * fini function.  unloadPlugin also frees the memory for mod
                     * and unloadsegs the seglist.
                     * */
                    unloadPlugin(secBase, mod);
                    return FALSE;
                }
            }
        }
        UnLoadSeg(seglist);
    }
    return FALSE;
}


void unloadPlugin(struct SecurityBase *secBase, secPluginModule * mod)
{
    BPTR seglist = mod->SegList;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if ((struct Process*)FindTask(NULL) == secBase->Server)
        mod->header->Terminate();
    else
        SendServerPacket(secBase, secSAction_FiniModule, (SIPTR)mod, (SIPTR)NULL, (SIPTR)NULL, (SIPTR)NULL);

    /* Remove the module from the list */
    ObtainSemaphore(&secBase->PluginModuleSem);
    Remove((struct Node*)mod);
    ReleaseSemaphore(&secBase->PluginModuleSem);
    Free(mod, sizeof(secPluginModule));
    if (seglist)
        UnLoadSeg(seglist);
    /* We can do this even if mod no longer exists in memory: the reference is
     * good enough for us */
    ObtainSemaphore(&secBase->TaskOwnerSem);
    FreeModuleContext(secBase, mod);
    ReleaseSemaphore(&secBase->TaskOwnerSem);
}

/* Explanation of Contexts
 *
 * `Context' is some block of memory that some part of the multiuser system
 * needs to associate with a particular caller.  For example: getpwent needs
 * to remember the position in the password file between calls - this
 * information belongs in the context of the caller.
 *
 * Simple context management can be acheived by associating the context with
 * the task that makes the call into the security library - however, the
 * AmigaOS architecture is such that this will break if one task opens the
 * library and CreateNewProcs a child process that shares the library base.
 *
 * So, the amazing security.library handles this case too: context is
 * associated with a task that explicitly opens the security.library.
 * It's child tasks inherit context.
 *
 * OK, so what if a child task LoadSegs some code and then does a RunCommand
 * on that seglist?  No problem.  OK, but what if the loaded code opens the
 * security.library itself - it will run fine (and get it's own context), but
 * when it closes the library, won't it's context dissapear too?
 *
 * Ahh, we can handle this case. Because we keep context in a stack; each time
 * the library is opened, we push a new context list onto this stack.  Each
 * time the library is closed, the context stack is popped back.
 * This means that even if the LoadSeg/RunCommand-ed child also LoadSegs and
 * RunCommands, things will still work as expected.
 *
 * What a really cool thing this is.
 *
 * OK, so how is it used?
 *
 * Modules within the system can request a certain amount of context space to
 * be allocated by using the secContextLocate API.  The returned memory can be
 * used for anything that the module desires; the only restriction is that the
 * size the module requests should be consistent between calls - the first
 * call allocates the memory - subsequent calls return the memory that was
 * allocated before-hand.
 *
 * There is no need to explicitly free the context - the library handles this:
 * context is freed when the current context is popped, or when the task node
 * is freed.  Additionally, context allocated under a certain module is freed
 * when that module is unloaded.
 * */

/* Free all context associated with the tasknode */
void FreeAllContext(struct secTaskNode * node)
{
    struct secContextNode * con;
    struct secContextList * clist;
    while ( (clist = (struct secContextList*)RemHead((struct List*)&node->Context)) != NULL)	{
        while( (con = (struct secContextNode*)RemHead((struct List*)&clist->Context)) != NULL)
                FreeV(con);
        Free(clist, sizeof(struct secContextList));
    }
}

/* Free all context allocated by the (module).
 * This is expensive. */
void FreeModuleContext(struct SecurityBase *secBase, secPluginModule * module)
{
    struct secContextNode * con;
    struct MinNode * tnode, * cnode, *clnode;
    struct secTaskNode * tasknode;
    struct secContextList * clist;
    int i;

    /* For each task,
     * 	For each context list
     * 		For each context node
     * 			if the module matches, remove and free the node
     * */
    
    for (i=0; i<TASKHASHVALUE; i++)	{
        for (tnode = secBase->TaskOwnerList[i].mlh_Head; tnode->mln_Succ; tnode = tnode->mln_Succ)	{
            tasknode = (struct secTaskNode*)tnode;
            for (clnode = tasknode->Context.mlh_Head; clnode->mln_Succ; clnode = clnode->mln_Succ)	{
                clist = (struct secContextList*)clnode;
                for (cnode = clist->Context.mlh_Head; cnode->mln_Succ; cnode = cnode->mln_Succ)	{
                    con = (struct secContextNode*)cnode;
                    if (con->mod == module)	{
                        Remove((struct Node*)con);
                        FreeV(con);
                    }
                }
            }
        }
    }
}

/* Allocate memory for the context of a module/id of a given size.
 * memory is attached to the context list of the supplied tasknode */
APTR AllocateContext(struct secTaskNode * node, secPluginModule * module, ULONG id, ULONG size)
{
    struct secContextNode * con;
    struct secContextList * clist;

    con = (struct secContextNode*)MAllocV(sizeof(struct secContextNode) + size);
    if (con)	{
        con->mod = module;
        con->id = id;
        clist = (struct secContextList*)node->Context.mlh_Head;
        AddHead((struct List*)&clist->Context, (struct Node*)con);
        return (APTR)((IPTR)con+sizeof(struct secContextNode));
    }
    return NULL;
}

struct secTaskNode * FindContextOwner(struct SecurityBase*secBase, struct Task * caller)
{
    struct secTaskNode * ret = FindTaskNode(secBase, caller);
    while (ret)	{
        if (!IsListEmpty((struct List*)&(ret->Context)))
            return ret;
        ret = ret->Parent;
    }
    return NULL;
}

APTR FindContext(struct secTaskNode * node, secPluginModule * module, ULONG id)
{
    struct secContextNode * con;
    struct secContextList * clist;
    struct MinNode * n;
    clist = (struct secContextList*)node->Context.mlh_Head;
    if (clist)	{
        for (n = clist->Context.mlh_Head; n->mln_Succ; n = n->mln_Succ)	{
            con = (struct secContextNode*)n;
            if ((con->mod == module) && (con->id == id))
                return (APTR)((IPTR)con+sizeof(struct secContextNode));
        }
    }
    return NULL;
}

/* Push a new level of context onto (caller).
 * Call this when (caller) opens the security.library
 * */
void PushContext(struct SecurityBase*secBase, struct Task * caller)
{
    struct secContextList * clist;
    struct secTaskNode * tnode;
    tnode = FindTaskNode(secBase, caller);
    if (tnode == NULL)
        return;	/* TODO: Create an orphan task here? This should never happen */
    clist = (struct secContextList*)MAlloc(sizeof(struct secContextList));
    if (clist == NULL)
        return;
    NEWLIST((struct List*)&clist->Context);
    AddHead((struct List*)&tnode->Context, (struct Node*)clist);
}

/* Remove the head from the (caller) context list.
 * Call this when (caller) closes the security.library */
void PopContext(struct SecurityBase*secBase, struct Task * caller)
{
    struct secContextList * clist;
    struct secTaskNode * tnode;
    struct secContextNode * con;
    
    tnode = FindTaskNode(secBase, caller);
    if (tnode == NULL)
        return;	/* TODO: Create an orphan task here? This should never happen */
    clist = (struct secContextList*)RemHead((struct List*)&tnode->Context);
    if (clist == NULL)
        return;
    /* Now free the context memory */
    while( (con = (struct secContextNode*)RemHead((struct List*)&clist->Context)) != NULL)
            FreeV(con);
    Free(clist, sizeof(struct secContextList));
}
