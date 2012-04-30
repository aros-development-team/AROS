/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include "diskimage_device.h"

static const UWORD cmd_list[] = {
	NSCMD_DEVICEQUERY,
	CMD_UPDATE,
	CMD_CLEAR,
	TD_MOTOR,
	CMD_READ,
	CMD_WRITE,
	TD_FORMAT,
	ETD_READ,
	ETD_WRITE,
	ETD_FORMAT,
	TD_CHANGENUM,
	TD_CHANGESTATE,
	TD_PROTSTATUS,
	TD_GETGEOMETRY,
	TD_EJECT,
	TD_REMOVE,
	TD_ADDCHANGEINT,
	TD_REMCHANGEINT,
	TD_READ64,
	TD_WRITE64,
	TD_FORMAT64,
	NSCMD_TD_READ64,
	NSCMD_TD_WRITE64,
	NSCMD_TD_FORMAT64,
	NSCMD_ETD_READ64,
	NSCMD_ETD_WRITE64,
	NSCMD_ETD_FORMAT64,
	HD_SCSICMD,
	0
};

#ifdef __AROS__
AROS_LH1(void, LibBeginIO,
	AROS_LHA(struct IOExtTD *, iotd, A1),
	struct DiskImageBase *, libBase, 5, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
void LibBeginIO (REG(a1, struct IOExtTD *iotd), REG(a6, struct DiskImageBase *libBase)) {
#endif
	struct Library *SysBase = libBase->SysBase;
	struct DiskImageUnit *unit;

	unit = (struct DiskImageUnit *)iotd->iotd_Req.io_Unit;

	dbug(("BeginIO()\n"));
	dbug(("unit: %ld, command: %ld\n",
		unit->UnitNum,
		iotd->iotd_Req.io_Command));

	switch (iotd->iotd_Req.io_Command) {
		case CMD_READ:
		case CMD_WRITE:
		case TD_FORMAT:
		case ETD_READ:
		case ETD_WRITE:
		case ETD_FORMAT:
		case TD_GETGEOMETRY:
		case TD_EJECT:
		case TD_REMOVE:
		case TD_ADDCHANGEINT:
		case TD_REMCHANGEINT:
		case TD_READ64:
		case TD_WRITE64:
		case TD_FORMAT64:
		case NSCMD_TD_READ64:
		case NSCMD_TD_WRITE64:
		case NSCMD_TD_FORMAT64:
		case NSCMD_ETD_READ64:
		case NSCMD_ETD_WRITE64:
		case NSCMD_ETD_FORMAT64:
		case HD_SCSICMD:
			/* forward to unit process */
			iotd->iotd_Req.io_Flags &= ~IOF_QUICK;
			ObtainSemaphore(unit->IOSemaphore);
			PutMsg(unit->IOPort, &iotd->iotd_Req.io_Message);
			ReleaseSemaphore(unit->IOSemaphore);
			return;

		case CMD_UPDATE:
		case CMD_CLEAR:
		case TD_MOTOR:
			/* handle as no-ops */
			iotd->iotd_Req.io_Error = IOERR_SUCCESS;
			break;

		case TD_CHANGENUM:
			iotd->iotd_Req.io_Actual = unit->ChangeCnt;
			iotd->iotd_Req.io_Error = IOERR_SUCCESS;
			break;

		case TD_CHANGESTATE:
			iotd->iotd_Req.io_Actual = unit->ImageData ? 0 : 1;
			iotd->iotd_Req.io_Error = IOERR_SUCCESS;
			break;

		case TD_PROTSTATUS:
			iotd->iotd_Req.io_Actual = unit->WriteProtect;
			iotd->iotd_Req.io_Error = IOERR_SUCCESS;
			break;

		case NSCMD_DEVICEQUERY:
			if (iotd->iotd_Req.io_Length < sizeof(struct NSDeviceQueryResult)) {
				iotd->iotd_Req.io_Error = IOERR_BADLENGTH;
			} else {
				struct NSDeviceQueryResult *dq;
				dq = (struct NSDeviceQueryResult *)iotd->iotd_Req.io_Data;
				dq->DevQueryFormat = 0;
				iotd->iotd_Req.io_Actual = dq->SizeAvailable =
					sizeof(struct NSDeviceQueryResult);
				dq->DeviceType = NSDEVTYPE_TRACKDISK;
				dq->DeviceSubType = 0;
				dq->SupportedCommands = (UWORD *)cmd_list;
				iotd->iotd_Req.io_Error = IOERR_SUCCESS;
			}
			break;

		default:
			/* not supported */
			iotd->iotd_Req.io_Error = IOERR_NOCMD;
			break;
	}

	iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;

	if (!(iotd->iotd_Req.io_Flags & IOF_QUICK))
		ReplyMsg(&iotd->iotd_Req.io_Message);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH1(LONG, LibAbortIO,
	AROS_LHA(struct IOStdReq *, which_io, A1),
	struct DiskImageBase *, libBase, 6, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
LONG LibAbortIO (REG(a1, struct IOStdReq *which_io), REG(a6, struct DiskImageBase *libBase)) {
#endif
	struct Library *SysBase = libBase->SysBase;
	struct DiskImageUnit *unit;
	struct IOStdReq *io;
	LONG error = IOERR_SUCCESS;

	unit = (struct DiskImageUnit *)which_io->io_Unit;
	ObtainSemaphore(unit->IOSemaphore);

	/* Check if the request is still in the queue, waiting to be
	   processed */

	io = (struct IOStdReq *)unit->IOPort->mp_MsgList.lh_Head;
	while (io->io_Message.mn_Node.ln_Succ) {
		if (io == which_io) {
			/* remove it from the queue and tag it as aborted */
			Remove(&io->io_Message.mn_Node);

			io->io_Actual = 0;
			error = io->io_Error = IOERR_ABORTED;

			/* reply the message, as usual */
			ReplyMsg(&io->io_Message);
			break;
		}
		io = (struct IOStdReq *)io->io_Message.mn_Node.ln_Succ;
	}

	ReleaseSemaphore(unit->IOSemaphore);

	return error;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

