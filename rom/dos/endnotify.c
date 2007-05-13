/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

#include <string.h>

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IOFileSys iofs;

    InitIOFS(&iofs, FSA_REMOVE_NOTIFY, DOSBase);
    iofs.io_Union.io_NOTIFY.io_NotificationRequest = notify;
    iofs.IOFS.io_Device = notify->nr_Device;

    DoIO(&iofs.IOFS);

    if (notify->nr_FullName != notify->nr_Name)
        FreeVec(notify->nr_FullName);

    if (notify->nr_Flags & NRF_SEND_MESSAGE &&
	(notify->nr_Flags & NRF_WAIT_REPLY || notify->nr_MsgCount)) {

	struct MsgPort *port = notify->nr_stuff.nr_Msg.nr_Port;
	struct NotifyMessage *nm, *tmp;

	notify->nr_Flags &= ~NRF_MAGIC;

	Disable();

	ForeachNodeSafe(&port->mp_MsgList, nm, tmp) {
	    if (nm->nm_Class == NOTIFY_CLASS &&
		nm->nm_Code == NOTIFY_CODE &&
		nm->nm_NReq == notify) {

		Remove((struct Node *) nm);
		ReplyMsg((struct Message *) nm);

                notify->nr_MsgCount--;
                if (notify->nr_MsgCount == 0)
                    break;
	    }
	}

	Enable();
    }

    AROS_LIBFUNC_EXIT
} /* EndNotify */
