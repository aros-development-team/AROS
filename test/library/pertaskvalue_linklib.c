/*
 * This function is in the static link lib.
 * It uses the genmodule provided Pertask_GetLibbase() function so it can be
 * used both in a library that uses pertask_rel.a or a program that just uses
 * pertask.a.
 * It does not call a function in pertask.library so that a good optimizing
 * compiler with link time function inlining to optimize this very good.
 */
#include "pertaskbase.h"

#ifdef DOESNT_WORK
struct PertaskBase *Pertask_GetLibbase(void);

#else /* Does Work */

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <aros/debug.h>

static struct Library *_PertaskBase;

static int pertask_linklib_init(void)
{
	_PertaskBase = OpenLibrary("pertask.library", 0);
	bug("PertaskBase (link) = %p\n", _PertaskBase);
	return TRUE;
}
ADD2INIT(pertask_linklib_init, 10);

static int pertask_linklib_exit(void)
{
	CloseLibrary(_PertaskBase);
	return TRUE;
}
ADD2EXIT(pertask_linklib_exit, 10);

struct PertaskBase *Pertask_GetLibbase(void)
{
	return (struct PertaskBase *)_PertaskBase;
}
#endif

int *__pertask_getvalueptr(void)
{
    return &(Pertask_GetLibbase()->value);
}
