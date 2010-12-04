
#include <aros/debug.h>
#include <proto/console.h>

AROS_LH1(LONG, SetConSnip,
	AROS_LHA(APTR, param, A0),
	struct Library *, ConsoleDevice, 10, Console)
{
    AROS_LIBFUNC_INIT

    bug("SetConSnip unimplemented\n");
    return 0;
    
    AROS_LIBFUNC_EXIT
}
