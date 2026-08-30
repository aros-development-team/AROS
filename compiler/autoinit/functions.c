/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: autoinit library - functions sets handling
*/

#define DEBUG 0

#include <aros/symbolsets.h>
#include <aros/debug.h>

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
