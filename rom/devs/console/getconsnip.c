
#include <aros/debug.h>
#include <proto/console.h>

AROS_LH0(LONG, GetConSnip,
	struct Library *, ConsoleDevice, 9, Console)

{
    AROS_LIBFUNC_INIT

    bug("GetConSnip unimplemented\n");
    return 0;
    
    AROS_LIBFUNC_EXIT
}
