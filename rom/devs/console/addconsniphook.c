
#include <aros/debug.h>
#include <proto/console.h>

AROS_LH1(LONG, AddConSnipHook,
	AROS_LHA(APTR, param, A0),
	struct Library *, ConsoleDevice, 11, Console)
{
    AROS_LIBFUNC_INIT

    bug("AddConSnipHook unimplemented\n");
    return 0;
    
    AROS_LIBFUNC_EXIT
}
