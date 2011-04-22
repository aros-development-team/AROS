
#include <aros/debug.h>
#include <proto/console.h>

#include "console_gcc.h"

AROS_LH1(LONG, SetConSnip,
	AROS_LHA(APTR, data, A0),
	struct ConsoleBase *, ConsoleDevice, 10, Console)
{
    AROS_LIBFUNC_INIT

    /* data = NUL-terminated string
     * TODO: paste to console
     */

    bug("SetConSnip unimplemented\n");
    return 0;
    
    AROS_LIBFUNC_EXIT
}
