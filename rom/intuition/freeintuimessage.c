/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/utility.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(void, FreeIntuiMessage,

         /*  SYNOPSIS */
         AROS_LHA(struct IntuiMessage *, imsg, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 149, Intuition)

/*  FUNCTION
    Private to AROS: free an IntuiMessage previously allocated
    with AllocIntuiMessage.
 
    INPUTS
    imsg - The IntuiMessage. May be NULL.
 
    RESULT
    none
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_FREEINTUIMESSAGE(dprintf("FreeIntuiMessage: Msg 0x%lx\n", imsg));

    ASSERT_VALID_PTR_OR_NULL(imsg);

#define EIM(x) ((struct ExtIntuiMessage *)(x))

    if (imsg)
    {
    	if ((imsg->Class == IDCMP_IDCMPUPDATE) && (imsg->IAddress))
	{
            FreeTagItems((struct TagItem *)imsg->IAddress);
	}
	
        if (EIM(imsg)->eim_TabletData)
        {
            if (EIM(imsg)->eim_TabletData->td_TagList) FreeTagItems(EIM(imsg)->eim_TabletData->td_TagList);
            FreePooled(GetPrivIBase(IntuitionBase)->IDCMPPool,EIM(imsg)->eim_TabletData,sizeof (struct TabletData));
        }
        FreePooled(GetPrivIBase(IntuitionBase)->IDCMPPool,imsg, sizeof(struct IntIntuiMessage));
    }

    DEBUG_FREEINTUIMESSAGE(dprintf("FreeIntuiMessage: done\n"));
    AROS_LIBFUNC_EXIT
}

