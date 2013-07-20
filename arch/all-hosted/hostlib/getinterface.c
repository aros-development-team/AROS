#include <proto/exec.h>

#include "hostinterface.h"
#include "hostlib_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hostlib.h>

	AROS_LH3(APTR *, HostLib_GetInterface,

/*  SYNOPSIS */
    	AROS_LHA(void *, handle, A0),
    	AROS_LHA(const char **, symtable, A1),
    	AROS_LHA(ULONG *, unresolved, A2),

/*  LOCATION */
    	struct HostLibBase *, HostLibBase, 5, HostLib)

/*  FUNCTION
	Resolve array of symbols in the host operating system library.
	The result is a pointer to a dynamically allocated array of
	symbol values.

    INPUTS
	handle     - An opaque library handle provided by HostLib_Open()
	symbable   - A pointer to a NULL-terminated array of symbol names
	unresolved - An optional location where count of unresolved symbols
		     will be placed. Can be set to NULL to ignore it.

    RESULT
	A pointer to a dynamically allocated array of symbol values or NULL if
	empty symbol table was given.

    NOTES
	Note that the resulting array will always have as many entries as there
	are in symbol names array. It some symbols (or even all of them) fail
	to resolve, correspondind entries will be set to NULL. You may supply
	a valid unresolved pointer if you want to get unresolved symbols count.
	
	Even incomplete interface needs to be freed using HostLib_DropInterface().

	Resulting values are valid as long as the library is open. For portability
	sake it's advised to free interfaces before closing corresponding libraries.

	This function appeared in v2 of hostlib.resource.

    EXAMPLE

    BUGS

    SEE ALSO
	HostLib_GetPointer()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    const char **c;
    ULONG cnt = 0;
    APTR *iface = NULL;

    for (c = symtable; *c; c++)
        cnt++;

    if (cnt)
    {
    	iface = AllocVec(cnt * sizeof(APTR), MEMF_ANY);
    	if (iface)
	{
	    ULONG bad = 0;
	    ULONG i;

	    HOSTLIB_LOCK();
	    
	    for (i = 0; i < cnt; i++)
	    {
		iface[i] = HostLibBase->HostIFace->hostlib_GetPointer(handle, symtable[i], NULL);
		AROS_HOST_BARRIER

		if (!iface[i])
		    bad++;
	    }

	    HOSTLIB_UNLOCK();

    	    if (unresolved)
    	        *unresolved = bad;
    	}
    }

    return iface;

    AROS_LIBFUNC_EXIT
}
