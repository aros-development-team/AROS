#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <sigcore.h>
#include "etask.h"
#include "exec_util.h"

#include <aros/libcall.h>
#include <asm/segments.h>

AROS_LH4(BOOL, PrepareContext,
    AROS_LHA(struct Task *, task, A0),
    AROS_LHA(APTR, entryPoint, A1),
    AROS_LHA(APTR, fallBack, A2),
    AROS_LHA(struct TagItem *, tagList, A3),
    struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    return FALSE; //PrepareContext_Common(task, entryPoint, fallBack, tagList, SysBase) ? TRUE : FALSE;

    AROS_LIBFUNC_EXIT
}
