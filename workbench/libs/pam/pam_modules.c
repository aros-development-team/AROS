/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - external modules (SYS:Libs/PAM/<name>.pam)

          The directory is resolved once, through a lock, into a path that
          names the volume, so that a later re-assignment of SYS: does not
          redirect module loading. A module name may not contain ':' or '/'.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "pam_intern.h"

static BOOL ResolveModuleDir(struct PamBase *PamBase)
{
    BPTR lock;
    char path[256];

    if (PamBase->ModuleDir)
        return TRUE;
    if (!(lock = Lock(PAM_MODULE_DIR, SHARED_LOCK)))
        return FALSE;
    if (NameFromLock(lock, path, sizeof(path) - 2))
    {
        ULONG len = strlen(path);
        if (len && path[len - 1] != ':' && path[len - 1] != '/')
            strcat(path, "/");
        PamBase->ModuleDir = PamStrDup(PamBase, path);
    }
    UnLock(lock);
    return PamBase->ModuleDir != NULL;
}

struct PamModule *PamLoadModule(struct PamBase *PamBase, CONST_STRPTR name)
{
    struct PamModule *m;
    char path[300];

    if (!name || !name[0] || strlen(name) >= sizeof(m->Name) || strchr(name, ':') || strchr(name, '/'))
        return NULL;

    ObtainSemaphore(&PamBase->ModuleSem);
    ForeachNode(&PamBase->Modules, m)
    {
        if (!Stricmp(m->Name, name))
        {
            m->Users++;
            ReleaseSemaphore(&PamBase->ModuleSem);
            return m;
        }
    }
    if (ResolveModuleDir(PamBase) && (m = AllocVec(sizeof(struct PamModule), MEMF_CLEAR)))
    {
        strcpy(m->Name, name);
        strcpy(path, PamBase->ModuleDir);
        strcat(path, name);
        strcat(path, PAM_MODULE_SUFFIX);
        if ((m->Base = OpenLibrary(path, 0)))
        {
            m->Users = 1;
            AddTail((struct List *)&PamBase->Modules, (struct Node *)m);
        }
        else
        {
            D(bug(DEBUG_NAME_STR " %s: cannot open '%s'\n", __func__, path);)
            FreeVec(m);
            m = NULL;
        }
    }
    else
        m = NULL;
    ReleaseSemaphore(&PamBase->ModuleSem);
    return m;
}

void PamReleaseModule(struct PamBase *PamBase, struct PamModule *mod)
{
    ObtainSemaphore(&PamBase->ModuleSem);
    if (mod->Users)
        mod->Users--;
    /* Kept open until the library is expunged: re-opening is cheap and the
     * module may hold state */
    ReleaseSemaphore(&PamBase->ModuleSem);
}

void PamExpungeModules(struct PamBase *PamBase)
{
    struct PamModule *m;

    while ((m = (struct PamModule *)RemHead((struct List *)&PamBase->Modules)))
    {
        if (m->Base)
            CloseLibrary(m->Base);
        FreeVec(m);
    }
}
