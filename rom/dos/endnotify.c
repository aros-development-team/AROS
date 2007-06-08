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
    struct DevProc *dvp;

    /* get the device pointer and dir lock. note that we don't just use
     * nr_Handler here, because we also need to supply a unit pointer so
     * packet.handler can get its mount context */
    if ((dvp = GetDeviceProc(notify->nr_FullName, NULL)) == NULL)
        return;

    /* setup the call */
    InitIOFS(&iofs, FSA_REMOVE_NOTIFY, DOSBase);
    iofs.io_Union.io_NOTIFY.io_NotificationRequest = notify;

    iofs.IOFS.io_Device = (struct Device *) dvp->dvp_Port;

    /* take the root lock from either the doslist entry or from the devproc */
    if (dvp->dvp_Lock == NULL)
        iofs.IOFS.io_Unit = dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit;
    else
        iofs.IOFS.io_Unit = ((struct FileHandle *) BADDR(dvp->dvp_Lock))->fh_Unit;

    FreeDeviceProc(dvp);

    /* go */
    do {
        DosDoIO(&iofs.IOFS);
    } while (iofs.io_DosError != 0 && ErrorReport(iofs.io_DosError, REPORT_LOCK, 0, dvp->dvp_Port));

    /* free fullname if it was built in StartNotify() */
    if (notify->nr_FullName != notify->nr_Name)
        FreeVec(notify->nr_FullName);

    /* if the filesystem has outstanding messages, they need to be replied */
    if (notify->nr_Flags & NRF_SEND_MESSAGE &&
	(notify->nr_Flags & NRF_WAIT_REPLY || notify->nr_MsgCount > 0)) {

	struct MsgPort *port = notify->nr_stuff.nr_Msg.nr_Port;
	struct NotifyMessage *nm, *tmp;

	notify->nr_Flags &= ~NRF_MAGIC;

        /* protect access to the message list */
	Disable();

        /* loop over the messages */
	ForeachNodeSafe(&port->mp_MsgList, nm, tmp) {
            /* if its one of our notify messages */
	    if (nm->nm_Class == NOTIFY_CLASS &&
		nm->nm_Code == NOTIFY_CODE &&
		nm->nm_NReq == notify) {

                /* remove and reply */
		Remove((struct Node *) nm);
		ReplyMsg((struct Message *) nm);

                /* decrement the count. bail early if we've done them all */
                notify->nr_MsgCount--;
                if (notify->nr_MsgCount == 0)
                    break;
	    }
	}

        /* unlock the list */
	Enable();
    }

    AROS_LIBFUNC_EXIT
} /* EndNotify */
