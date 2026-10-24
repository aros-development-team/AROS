/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: autoinit library - functions sets handling
*/

#define DEBUG 0

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <proto/exec.h>

int _set_call_funcs(const void * const set[], int direction, int test_fail, struct ExecBase *SysBase)
{
    int pos, (*func)(struct ExecBase *SysBase);

    D(bug("entering set_call_funcs() - %p\n", set));
    
    ForeachElementInSet(set, direction, pos, func)
    {
        D(bug("  %p[%d] %p()", set, pos, func));

        if (test_fail)
        {
            int ret = (*func)(SysBase);
            D(bug(" => %d", ret));
            if (!ret)
                return 0;
        }
        else
        {
            (void)(*func)(SysBase);
        }
        D(bug("\n"));
    }
    
    return 1;
}

/* The open/init sequence genmodule used to inline into every module's InitLib.
   The order here - and the rollback order on failure - must stay exactly as
   the generated code had it. */
int _set_libinit(const struct __aros_libinit_sets *s, void *libbase, struct ExecBase *SysBase)
{
    int ok = 1, initcalled = 0;

    if (s->libs && !_set_open_libraries_list(s->libs, SysBase))
        ok = 0;
    if (ok && s->rellibs && !_set_open_rellibraries_list(libbase, s->rellibs, SysBase))
        ok = 0;
    if (ok && !_set_call_funcs(s->init, 1, 1, SysBase))
        ok = 0;
    if (ok && s->classesinit && !set_call_libfuncs(s->classesinit, 1, 1, libbase))
        ok = 0;

    if (ok)
    {
        _set_call_funcs(s->ctors, -1, 0, SysBase);
        _set_call_funcs(s->init_array, 1, 0, SysBase);

        initcalled = 1;
        ok = set_call_libfuncs(s->initlib, 1, 1, libbase);
    }

    if (!ok)
    {
        if (initcalled)
            set_call_libfuncs(s->expungelib, -1, 0, libbase);
        _set_libexpunge(s, libbase, SysBase);
        return 0;
    }

    return 1;
}

void _set_libexpunge(const struct __aros_libinit_sets *s, void *libbase, struct ExecBase *SysBase)
{
    _set_call_funcs(s->fini_array, -1, 0, SysBase);
    _set_call_funcs(s->dtors, 1, 0, SysBase);
    _set_call_funcs(s->exit, -1, 0, SysBase);
    if (s->classesexpunge)
        set_call_libfuncs(s->classesexpunge, -1, 0, libbase);
    if (s->rellibs)
        _set_close_rellibraries_list(libbase, s->rellibs, SysBase);
    if (s->libs)
        _set_close_libraries_list(s->libs, SysBase);
}

/* The InitLib, OpenLib, CloseLib and ExpungeLib bodies genmodule used to
   emit inline into every module with an auto-initialised base. Each one
   must keep the order of the generated code it replaces. */
void *_set_libinitbase(const struct __aros_libinit_sets *s, struct Library *lib,
                       LONG sysver, struct Library **oopbaseptr, struct ExecBase *SysBase)
{
    if (!SysBase || SysBase->LibNode.lib_Version < sysver)
        return NULL;

    if (oopbaseptr)
    {
        *oopbaseptr = OpenLibrary("oop.library", 0);
        if (*oopbaseptr == NULL)
            return NULL;
    }

    if (!_set_libinit(s, lib, SysBase))
    {
        FreeMem((UBYTE *)lib - lib->lib_NegSize, lib->lib_NegSize + lib->lib_PosSize);
        return NULL;
    }

    return lib;
}

/* A resource that builds its own base: allocation and vectors here, the
   set sequence and AddResource() in _set_resadd(). */
static ULONG resvecsize(ULONG vecsize)
{
    if (vecsize > 0)
        vecsize = ((vecsize-1)/sizeof(IPTR) + 1)*sizeof(IPTR);
    return vecsize;
}

void *_set_resalloc(ULONG vecsize, ULONG basesize, CONST_STRPTR name, BYTE pri,
                    CONST_APTR functable, LONG sysver, struct ExecBase *SysBase)
{
    struct Node *n;
    char *mem;

    if (!SysBase || SysBase->LibNode.lib_Version < sysver)
        return NULL;

    vecsize = resvecsize(vecsize);
    mem = AllocMem(vecsize+basesize, MEMF_PUBLIC|MEMF_CLEAR);
    if (mem == NULL)
         return NULL;
    n = (struct Node *)(mem + vecsize);
    n->ln_Type = NT_RESOURCE;
    n->ln_Pri = pri;
    n->ln_Name = (char *)name;
    MakeFunctions(n, (APTR)functable, NULL);

    return n;
}

void *_set_resadd(const struct __aros_libinit_sets *s, APTR base, ULONG vecsize,
                  ULONG basesize, struct ExecBase *SysBase)
{
    if (!_set_libinit(s, base, SysBase))
    {
        vecsize = resvecsize(vecsize);
        FreeMem((char *)base - vecsize, vecsize+basesize);
        return NULL;
    }

    AddResource(base);
    return base;
}

void *_set_libopen(const struct __aros_libfuncs_sets *s, struct Library *lib)
{
    if (set_call_libfuncs(s->openlib, 1, 1, lib))
    {
        lib->lib_OpenCnt++;
        lib->lib_Flags &= ~LIBF_DELEXP;
        return lib;
    }

    return NULL;
}

void _set_devopen(const struct __aros_libfuncs_sets *s, struct Library *lib,
                  struct IORequest *ioreq, IPTR unitnum, ULONG flags)
{
    if (set_call_libfuncs(s->openlib, 1, 1, lib)
        && set_call_devfuncs(s->opendev, 1, 1, lib, ioreq, unitnum, flags)
    )
    {
        lib->lib_OpenCnt++;
        lib->lib_Flags &= ~LIBF_DELEXP;

        ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    }
    else
    {
        if (ioreq->io_Error >= 0)
            ioreq->io_Error = IOERR_OPENFAIL;
    }
}

/* Returns TRUE when the caller has to expunge the module */
int _set_libclose(const struct __aros_libfuncs_sets *s, struct Library *lib,
                  struct IORequest *ioreq)
{
    if (s->closedev && !set_call_devfuncs(s->closedev, -1, 1, lib, ioreq, 0, 0))
        return FALSE;

    lib->lib_OpenCnt--;
    set_call_libfuncs(s->closelib, -1, 0, lib);

    return (lib->lib_OpenCnt == 0) && (lib->lib_Flags & LIBF_DELEXP);
}

BPTR _set_libexpungebase(const struct __aros_libfuncs_sets *s, struct Library *lib,
                         BPTR seglist, struct Library **oopbaseptr, struct ExecBase *SysBase)
{
    if (lib->lib_OpenCnt == 0)
    {
        if (!set_call_libfuncs(s->init.expungelib, -1, 1, lib))
        {
            lib->lib_Flags |= LIBF_DELEXP;
            return BNULL;
        }

        Remove((struct Node *)lib);

        _set_libexpunge(&s->init, lib, SysBase);
        if (oopbaseptr)
            CloseLibrary(*oopbaseptr);

        FreeMem((UBYTE *)lib - lib->lib_NegSize, lib->lib_NegSize + lib->lib_PosSize);

        return seglist;
    }

    lib->lib_Flags |= LIBF_DELEXP;

    return BNULL;
}
