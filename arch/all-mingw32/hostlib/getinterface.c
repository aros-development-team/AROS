#include <proto/exec.h>

#include <stdarg.h>

#include "../kernel/hostinterface.h"
#include "hostlib_intern.h"

AROS_LH3(APTR *, HostLib_GetInterface,
    	 AROS_LHA(void *,  handle,         A0),
    	 AROS_LHA(const char **, symtable, A1),
    	 AROS_LHA(ULONG *, unresolved,     A2),
    	 struct HostLibBase *, HostLibBase, 5, HostLib)
{
    AROS_LIBFUNC_INIT

    const char **c;
    ULONG cnt = 0;
    APTR *iface = NULL;

    for (c = symtable; *c; c++)
        cnt += sizeof(APTR);

    if (cnt)
    {
    	iface = AllocVec(cnt, MEMF_CLEAR);
    	if (iface)
	{
    	    cnt = HostLibBase->HostIFace->HostLib_GetInterface(handle, symtable, iface);
    	    if (unresolved)
    	        *unresolved = cnt;
    	}
    }

    return iface;
    
    AROS_LIBFUNC_EXIT
}
