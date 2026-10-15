/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - external modules (SYS:Libs/PAM/<name>.pam)

          The directory is resolved once, through a lock, into a path that
          names the volume, so that a later re-assignment of SYS: does not
          redirect module loading. A module name may not contain ':' or '/'.

          Modules are loaded here, not through OpenLibrary(): LDDemon would
          hand out any library already in memory under the module's name
          (anybody can load one from RAM:), so the file is LoadSeg()'d, its
          romtag initialised, and the module refused if a library of that
          name existed before. On a configured multi-user system the file
          and the directory must belong to root and not be writable by
          group or others (pam_config.c, PamTrusted()).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <resources/log.h>
#include <stddef.h>
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

/* The module's romtag in a seglist */
static struct Resident *FindRomTag(BPTR seglist)
{
    const ULONG minsize = offsetof(struct Resident, rt_Init) + sizeof(APTR);
    BPTR seg;

    for (seg = seglist; seg; seg = *(BPTR *)BADDR(seg))
    {
        UBYTE *addr = (UBYTE *)BADDR(seg) - sizeof(ULONG);
        ULONG size = *(ULONG *)addr;

        for (addr += sizeof(BPTR) + sizeof(ULONG), size -= sizeof(BPTR) + sizeof(ULONG);
             size >= minsize; size -= 2, addr += 2)
        {
            struct Resident *rt = (struct Resident *)addr;
            if (rt->rt_MatchWord == RTC_MATCHWORD && rt->rt_MatchTag == rt)
                return rt;
        }
    }
    return NULL;
}

/* Load, initialise and open a module from exactly this file */
static struct Library *LoadModule(struct PamBase *PamBase, CONST_STRPTR path)
{
    struct Library *base = NULL, *lib;
    struct Resident *rt;
    BPTR seglist, lock;

    if (!(lock = Lock(path, SHARED_LOCK)))
        return NULL;
    if (!PamTrusted(PamBase, lock, TRUE))
    {
        UnLock(lock);
        PamLogA(PamBase, NULL, LOGF_Flag_Type_Error, "module '%s' refused: not owned by root or writable by others", (IPTR []){ (IPTR)path });
        return NULL;
    }
    UnLock(lock);

    if (!(seglist = LoadSeg(path)))
        return NULL;
    if (!(rt = FindRomTag(seglist)) || rt->rt_Type != NT_LIBRARY || !rt->rt_Name)
    {
        UnLoadSeg(seglist);
        return NULL;
    }

    Forbid();
    if (FindName(&SysBase->LibList, rt->rt_Name))
    {
        Permit();
        UnLoadSeg(seglist);
        PamLogA(PamBase, NULL, LOGF_Flag_Type_Error, "module '%s' refused: a library '%s' is already loaded",
                (IPTR []){ (IPTR)path, (IPTR)rt->rt_Name });
        return NULL;
    }
    InitResident(rt, seglist);
    if ((lib = (struct Library *)FindName(&SysBase->LibList, rt->rt_Name)))
    {
        /* still under Forbid(): the one just initialised is the one opened */
        base = OpenLibrary(rt->rt_Name, 0);
        if (base != lib)
        {
            if (base)
                CloseLibrary(base);
            base = NULL;
        }
    }
    Permit();
    if (!lib)
        UnLoadSeg(seglist);   /* the romtag did not make a library; the segment is ours */
    return base;
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
        if ((m->Base = LoadModule(PamBase, path)))
        {
            m->Users = 1;
            AddTail((struct List *)&PamBase->Modules, (struct Node *)m);
        }
        else
        {
            PamLogA(PamBase, NULL, LOGF_Flag_Type_Error, "module '%s' not loaded", (IPTR []){ (IPTR)path });
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
        {
            /* expunge it too, so that a fresh pam.library may load it again */
            Forbid();
            CloseLibrary(m->Base);
            RemLibrary(m->Base);
            Permit();
        }
        FreeVec(m);
    }
}
