
#include <aros/debug.h>
#include <proto/console.h>

AROS_LH1(LONG, RemConSnipHook,
	AROS_LHA(APTR, param, A0),
	struct Library *, ConsoleDevice, 12, Console)
{
    AROS_LIBFUNC_INIT

    bug("RemConSnipHook unimplemented\n");
    return 0;
    
    AROS_LIBFUNC_EXIT
}
