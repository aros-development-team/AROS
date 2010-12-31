#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "hostlib_intern.h"

/* We don't want to depend on utility.library so we use own NextTagItem() */
static struct TagItem *NextTagItem(struct TagItem **tagListPtr)
{
    if (!(*tagListPtr))
	return NULL;

    while(1)
    {
        switch((*tagListPtr)->ti_Tag)
        {
            case TAG_MORE:
                if (!((*tagListPtr) = (struct TagItem *)(*tagListPtr)->ti_Data))
                    return NULL;
                continue;
            case TAG_IGNORE:
                break;

            case TAG_END:
                (*tagListPtr) = 0;
                return NULL;

            case TAG_SKIP:
                (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
                continue;

            default:
                return (struct TagItem *)(*tagListPtr)++;

        }

        (*tagListPtr)++;
    }
}

static int HostLib_Init(struct HostLibBase *HostLibBase)
{
    APTR KernelBase;
    struct TagItem *BootInfo, *tag;

    KernelBase = OpenResource("kernel.resource");
    D(bug("[hostlib] KernelBase = 0x%08lX\n", KernelBase));
    if (!KernelBase)
	return FALSE;

    BootInfo = KrnGetBootInfo();
    while ((tag = NextTagItem(&BootInfo)))
    {
	if (tag->ti_Tag == KRN_HostInterface)
	{
	    HostLibBase->HostIFace = (struct HostInterface *)tag->ti_Data;
    	    D(bug("[hostlib] HostIFace = 0x%08lX\n", HostLibBase->HostIFace));

#ifndef USE_FORBID_LOCK
	    InitSemaphore(&HostLibBase->HostSem);
#endif
	    return TRUE;
	}
    }

    return FALSE;
}

ADD2INITLIB(HostLib_Init, 0)
