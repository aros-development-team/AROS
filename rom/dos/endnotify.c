/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <exec/lists.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <dos/notify.h>
#include <proto/dos.h>

	AROS_LH1(void, EndNotify,

/*  SYNOPSIS */
	AROS_LHA(struct NotifyRequest *, notify, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 149, Dos)

/*  FUNCTION

    End a notification (quit notifying for a request previously sent with
    StartNotify()).

    INPUTS

    notify  --  NotifyRequest used with StartNotify()

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    StartNotify()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_REMOVE_NOTIFY, DOSBase);

    iofs.IOFS.io_Device = (struct Device *)notify->nr_Device;

    if (iofs.IOFS.io_Device == NULL)
    {
	return;
    }

    iofs.io_Union.io_NOTIFY.io_NotificationRequest = notify;

    DoIO(&iofs.IOFS);

    if (notify->nr_Flags & NRF_SEND_MESSAGE)
    {
	struct Node          *tempNode;
	struct NotifyMessage *nm;

	Disable();

	ForeachNodeSafe(&notify->nr_stuff.nr_Msg.nr_Port->mp_MsgList,
			(struct Node *)nm, tempNode)
	{
	    if (notify->nr_MsgCount == 0)
	    {
		break;
	    }
	    
	    if (nm->nm_NReq == notify)
	    {
		notify->nr_MsgCount--;
		Remove((struct Node *)nm);
		ReplyMsg((struct Message *)nm);		
	    }
	}

	Enable();
    }
    
    SetIoErr(iofs.io_DosError);

    AROS_LIBFUNC_EXIT
} /* EndNotify */
