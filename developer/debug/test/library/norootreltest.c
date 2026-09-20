/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test for the genmodule option norootrellibs.

    userel.library and norootrel.library are both pertaskbase libraries with
    `rellib pertask`; only norootrel is built with norootrellibs. On a
    library's first load its InitLib runs in the opener's task. By default it
    opens the rellibs for the root base there, which gives the root base a
    pertask.library base belonging to this task, kept until the library is
    expunged. With norootrellibs no rellib is opened for the root base.

    For each library: expunge it, open and close it once in this task (which
    does not use pertask.library itself), and compare pertask.library's open
    count before and after. userel is the unchanged control.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/execbase.h>
#include <exec/memory.h>

#define __NOLIBBASE__
#include <proto/norootrel.h>

struct Library *NorootrelBase;

static LONG opencount(const char *name)
{
    struct Library *lib;
    LONG count = 0;

    Forbid();
    lib = (struct Library *)FindName(&SysBase->LibList, (CONST_STRPTR)name);
    if (lib)
        count = lib->lib_OpenCnt;
    Permit();

    return count;
}

static BOOL loaded(const char *name)
{
    BOOL found;

    Forbid();
    found = FindName(&SysBase->LibList, (CONST_STRPTR)name) != NULL;
    Permit();

    return found;
}

/* A failing allocation makes exec expunge every library nobody has open. */
static void flush(void)
{
    APTR mem = AllocMem(0x7ffffff0, MEMF_PUBLIC);

    if (mem)
        FreeMem(mem, 0x7ffffff0);
}

/* Returns the change in pertask.library's open count, or -1000 when the
   library could not be tested from its first load. */
static LONG first_open(const char *name, BOOL call)
{
    struct Library *lib;
    LONG before, after;

    flush();
    if (loaded(name))
        return -1000;

    before = opencount("pertask.library");
    lib = OpenLibrary((CONST_STRPTR)name, 0);
    if (!lib)
        return -1000;
    if (call)
    {
        NorootrelBase = lib;
        Printf((CONST_STRPTR)"NOROOTRELLIBS value through the rellib: %ld\n",
               (LONG)NorootrelGetValue());
    }
    CloseLibrary(lib);
    after = opencount("pertask.library");
    flush();

    return after - before;
}

int main(void)
{
    LONG control, noroot;
    BOOL pass;

    control = first_open("userel.library", FALSE);
    noroot = first_open("norootrel.library", TRUE);

    if (control == -1000 || noroot == -1000)
    {
        Printf((CONST_STRPTR)"NOROOTRELLIBS INCONCLUSIVE: a library could not be "
               "tested from its first load (control=%ld norootrel=%ld)\n", control, noroot);
        return RETURN_WARN;
    }

    pass = (noroot == 0);
    Printf((CONST_STRPTR)"NOROOTRELLIBS %s: pertask.library references left after the "
           "first open and close: userel (default) %ld, norootrel %ld\n",
           pass ? "PASS" : "FAIL", control, noroot);

    return pass ? RETURN_OK : RETURN_FAIL;
}
