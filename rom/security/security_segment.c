
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <proto/security.h>

#include <exec/lists.h>
#include <exec/nodes.h>

#include "security_intern.h"

/*
 *      Init Segment List
 */

void InitSegList(struct SecurityBase *secBase)
{
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    ObtainSemaphore(&secBase->SegOwnerSem);
    NEWLIST((struct List *)&secBase->SegOwnerList);
    ReleaseSemaphore(&secBase->SegOwnerSem);
}
