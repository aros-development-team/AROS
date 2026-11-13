
#include <exec/types.h>

#include "defs.h"

struct VMMHBase
{
    struct Library              vmmhb_LibNode;
    APTR                        vmmhb_KernelBase;
#if defined(AROS_USE_LOGRES)
    APTR                        vmmhb_LogResBase;
    APTR                        vmmhb_LogRHandle;
#endif
	/**/
    APTR                        vmmhb_AllocMem;
    APTR                        vmmhb_FreeMem;
};
