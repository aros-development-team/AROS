#include <libcore/compiler.h>
#include <libcore/base.h>

#include "intern.h"
#include "SampleFuncs.h"

AROS_LH2(struct LibHeader *, InitLib,
    AROS_LHA(struct LibHeader *, lh, D0),
    AROS_LHA(BPTR,                 segList, A0),
    struct ExecBase *, SysBase, 0, LibHeader
);
AROS_LH1 (struct LibHeader *, OpenLib,
    AROS_LHA (ULONG, version, D0),
    struct LibHeader *, lh, 1, LibHeader
);
AROS_LH0 (BPTR, CloseLib,
    struct LibHeader *, lh, 2, LibHeader
);
AROS_LH0 (BPTR, ExpungeLib,
    struct LibHeader *, lh, 3, LibHeader
);
AROS_LH0 (struct LibHeader *, ExtFuncLib,
    struct LibHeader *, lh, 4, LibHeader
);

APTR const LIBFUNCTABLE [] =
{
    (APTR) AROS_SLIB_ENTRY(OpenLib, LibHeader),
    (APTR) AROS_SLIB_ENTRY(CloseLib, LibHeader),
    (APTR) AROS_SLIB_ENTRY(ExpungeLib, LibHeader),
    (APTR) AROS_SLIB_ENTRY(ExtFuncLib, LibHeader),

    /* add your own functions here */
    (APTR) AROS_SLIB_ENTRY(EXF_TestRequest, BASENAME),

    /* This marks the end of the table */
    (APTR) ((LONG)-1)
};

