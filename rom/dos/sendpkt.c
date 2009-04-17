/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include "dos_intern.h"
#include <dos/dosextens.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <exec/initializers.h>
#include <string.h>
#include <proto/utility.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(void, SendPkt,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, D1),
	AROS_LHA(struct MsgPort   *, port, D2),
	AROS_LHA(struct MsgPort   *, replyport, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 41, Dos)

/*  FUNCTION

    Send a packet to a handler without waiting for the result. The packet will
    be returned to 'replyport'.

    INPUTS

    packet     --  the (initialized) packet to send
    port       --  the MsgPort to send the packet to
    replyport  --  the MsgPort to which the packet will be replied

    RESULT

    This function is callable from a task.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DoPkt(), WaitPkt(), AbortPkt()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* If port address is NULL, the called grabbed it manually from DosNode of IOFS handler.
       Forward the packet to emulator. */
    if (!port) {
        D(bug("[DOS] SendPkt(): port is NULL, using IOFS\n"));
        IOFS_SendPkt(dp, replyport, DOSBase);
        return;
    }
    /* This is a bit of magic. If the caller obtained port address from GetDeviceProc() result,
       it will actually be a struct Device *. Fortinately both MsgPort and Device
       have plain Node structure in the beginning. This means we can check node type. */
    if (port->mp_Node.ln_Type == NT_DEVICE) {
        D(bug("[DOS] SendPkt(): port is IOFS device, using IOFS\n"));
        IOFS_SendPkt(dp, replyport, DOSBase);
        return;
    }

    /* If we are here, we are working with packet-style handler and attempt to send a packet directly.
       However we have to be careful here. AROS still has long-standing misdesign problem (locks and handles
       are the same things). In order to overcome this, an internal translation is performed by IOFS emulation layer.
       The result of this is that we can't pass locks or handles to packet-style handlers directly, we still have to
       push them through the emulator instead */
    switch (dp->dp_Type) {
    case ACTION_FINDUPDATE:
    case ACTION_FINDINPUT:
    case ACTION_FINDOUTPUT:
    case ACTION_END:
    case ACTION_READ:
    case ACTION_WRITE:
    case ACTION_SEEK:
    case ACTION_CURRENT_VOLUME:
    case ACTION_SET_FILE_SIZE:
    case ACTION_LOCK_RECORD:
    case ACTION_FREE_RECORD:
    case ACTION_LOCATE_OBJECT:
    case ACTION_FREE_LOCK:
    case ACTION_COPY_DIR:
    case ACTION_PARENT:
    case ACTION_SAME_LOCK:
    case ACTION_CREATE_DIR:
    case ACTION_CHANGE_MODE:
    case ACTION_FH_FROM_LOCK:
    case ACTION_COPY_DIR_FH:
    case ACTION_PARENT_FH:
    case ACTION_EXAMINE_OBJECT:
    case ACTION_EXAMINE_NEXT:
    case ACTION_EXAMINE_FH:
    case ACTION_EXAMINE_ALL:
    case ACTION_DELETE_OBJECT:
    case ACTION_RENAME_OBJECT:
    case ACTION_MAKE_LINK:
    case ACTION_READ_LINK:
    case ACTION_SET_COMMENT:
    case ACTION_SET_DATE:
    case ACTION_SET_PROTECT:
    case ACTION_INFO:
        D(bug("[DOS] SendPkt(): Packet requires lock/handle, using IOFS\n"));
        IOFS_SendPkt(dp, replyport, DOSBase);
        break;
    /* All custom packets will be sent to packet-style handlers directly. Again, beware! You can't pass locks or
       filehandles in their arguments because of translation done by IOFS layer. Theoretically you could do this
       if you obtain these locks/handles also by direct packet I/O, not by OS functions. However this is not
       (and will not be) supported by AROS API, because the situation with file locks is going to change in some
       time. After this it should become possible to distinguish between true handles and locks and pass all packets
       directly to handlers. */
    default:
        D(bug("[DOS] SendPkt(): pkt = $%lx, port = $%lx, replyport = $%lx\n",
		      dp, port, replyport));

        dp->dp_Port=replyport;
        dp->dp_Link->mn_ReplyPort=replyport;

        PutMsg(port, dp->dp_Link);
        break;
    }
 
    AROS_LIBFUNC_EXIT
} /* SendPkt */

