/* Create a Library, add it to the system, and try to open it.
 */

#define DUMMY_OPEN_BUG 1
#define INIT_DEBUG_BUG 0
#define REMOVE_DUMMY 1

#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

struct DummyBase
{
    struct Library		library;
    struct ExecBase		*sysbase;
};

#undef SysBase
#define SysBase     	(DummyBase->sysbase)

#define DEBUG 1
#include <aros/debug.h>

/********/

AROS_UFH3(struct DummyBase *,LIB_init,
        AROS_LHA(struct DummyBase *, DummyBase, D0),
        AROS_LHA(ULONG, seglist, A0),
	AROS_LHA(struct ExecBase *, sysbase, A6))
{
    AROS_USERFUNC_INIT
    
#if INIT_DEBUG_BUG
    D(bug("in LIB_init a\n")); /* segfaults */
#endif
    DummyBase->sysbase = sysbase;

    DummyBase->library.lib_Node.ln_Type = NT_LIBRARY;
    DummyBase->library.lib_Node.ln_Name =  "dummy.library";
    DummyBase->library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    DummyBase->library.lib_Version = 1;
    DummyBase->library.lib_Revision = 0;
    DummyBase->library.lib_IdString = "dummy.library";

    D(bug("dummy.library: Init succesfull\n"));
    return DummyBase;
    
    AROS_USERFUNC_EXIT
}

/********/

AROS_LH1(struct DummyBase *, LIB_open,
    AROS_LHA(ULONG, version, D0),
    struct DummyBase *, DummyBase, 1, Dummy)
{
    AROS_LIBFUNC_INIT

    DummyBase->library.lib_OpenCnt++;
    DummyBase->library.lib_Flags &= ~LIBF_DELEXP;
    return DummyBase;
    
    AROS_LIBFUNC_EXIT
}

/********/

AROS_UFH1(ULONG,LIB_expunge,
        AROS_LHA(struct DummyBase *, DummyBase, A6))
{
    AROS_USERFUNC_INIT

    if (DummyBase->library.lib_OpenCnt == 0)
	Remove((struct Node *)DummyBase);
    else
	DummyBase->library.lib_Flags |= LIBF_DELEXP;
    return 0;

    AROS_USERFUNC_EXIT
}

/********/

AROS_UFH1(ULONG,LIB_close,
        AROS_LHA(struct DummyBase *, DummyBase, D0))
{
    AROS_USERFUNC_INIT
    
    ULONG ret = 0;

    DummyBase->library.lib_OpenCnt--;
    if (!DummyBase->library.lib_OpenCnt)
	if (DummyBase->library.lib_Flags & LIBF_DELEXP)
	    ret = LIB_expunge(DummyBase);
    return ret;

    AROS_USERFUNC_EXIT
}

/********/

int LIB_reserved(void)
{
    return 0;
}

static struct Library *dummylib;

static void *function_array[] =
{
  Dummy_LIB_open,
  LIB_close,
  LIB_expunge,
  LIB_reserved,
  (void*)~0,
};

/* Must use the global sysbase */
#undef SysBase

static struct Library *dummy = NULL;

int AddDummy(void)
{
    D(bug("*** at %s:%d\n", __FUNCTION__, __LINE__));

    dummylib = MakeLibrary(function_array,NULL,
			   (ULONG (* const )())LIB_init,
			   sizeof(struct DummyBase),NULL);

    D(bug("*** at %s:%d\n", __FUNCTION__, __LINE__));
    if (dummylib)
    {
	AddLibrary(dummylib);
#if DUMMY_OPEN_BUG
	D(bug("%s: before OpenLibrary\n", __FUNCTION__));
	dummy = OpenLibrary("dummy.library", 0); /* segfaults */
	D(bug("%s: after OpenLibrary\n", __FUNCTION__));
	if (!dummy)
	    return 0;
#endif
	return 1;
    }
    return 0;
}

int RemoveDummy(void)
{
    if (dummy)
	CloseLibrary(dummy);
    if (dummylib)
    {
	RemLibrary(dummylib);
    }
    return 1;
}

int __nocommandline = 1;

int main (void)
{
    AddDummy();
#if REMOVE_DUMMY
    RemoveDummy();
#endif

    return 0;
}

