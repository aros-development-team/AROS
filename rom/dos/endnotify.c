/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include "dos_intern.h"

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
        notify - NotifyRequest used with StartNotify()

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

    /*
     * Packet handlers love to replace nr_Handler of active requests with a pointer
     * to own real message port. It's not possible to prevent this by (simple) external
     * means.
     * This is why we use packet I/O here on all architectures. If nr_Handler points
     * to packet message port, the packet will be sent directly, bypassing IOFS layer.
     * This is 100% safe because we don't pass any locks and/or filehandles here.
     * If nr_Handler still points to IOFS device, packet I/O emulator will take care about
     * this.
     */
    dopacket1(DOSBase, NULL, notify->nr_Handler, ACTION_REMOVE_NOTIFY, (SIPTR)notify);

    /* free fullname if it was built in StartNotify() */
    if (notify->nr_FullName != notify->nr_Name)
        FreeVec(notify->nr_FullName);

    /* if the filesystem has outstanding messages, they need to be replied */
    if ((notify->nr_Flags & NRF_SEND_MESSAGE) &&
        ((notify->nr_Flags & NRF_WAIT_REPLY) || notify->nr_MsgCount > 0))
    {
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
