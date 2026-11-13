#include <proto/exec.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include <devices/scsidisk.h>
#include "deviceio.h"

BOOL openIO(struct DeviceIO *dio, STRPTR device, ULONG unit) {

	dio->mp = CreateMsgPort();
	if (dio->mp)
	{
		dio->iotd = (struct IOExtTD *)CreateIORequest(dio->mp, sizeof(struct IOExtTD));
		if (dio->iotd)
		{
			if (OpenDevice(device, unit, (struct IORequest *)dio->iotd, 0)==0)
				return TRUE;
			DeleteIORequest((struct IORequest *)dio->iotd);
		}
		DeleteMsgPort(dio->mp);
	}
	return FALSE;
}

void closeIO(struct DeviceIO *dio) {
	CloseDevice((struct IORequest *)dio->iotd);
	DeleteIORequest((struct IORequest *)dio->iotd);
	DeleteMsgPort(dio->mp);
}

BOOL iscorrectType(struct IOExtTD *iotd) {
BOOL retval = TRUE;
struct DriveGeometry dg;

	iotd->iotd_Req.io_Command = TD_GETGEOMETRY;
	iotd->iotd_Req.io_Data = &dg;
	iotd->iotd_Req.io_Length = sizeof(struct DriveGeometry);
#if 0
	if (DoIO((struct IORequest *)iotd) == 0)
	{
		if (dg.dg_DeviceType == DG_CDROM)
			retval = FALSE;
//		if (dg.dg_Flags & DGF_REMOVABLE)
//			retval = FALSE;
	}
#endif
	return retval;
}

void w2strcpy(STRPTR name, UWORD *wstr, ULONG len) {

   while (len)
   {
      *((UWORD *)name) = AROS_BE2WORD(*wstr);
      name += sizeof(UWORD);
      len -= 2;
      wstr++;
   }
   name -= 2;
   while ((*name==0) || (*name==' '))
      *name-- = 0;
}

BOOL identify(struct IOExtTD *iotd, STRPTR id) {
struct SCSICmd scsicmd;
UWORD data[256];
UBYTE cmd=0xEC; /* identify */

   scsicmd.scsi_Data = data;
   scsicmd.scsi_Length = 512;
   scsicmd.scsi_Command = &cmd;
   scsicmd.scsi_CmdLength = 1;
   iotd->iotd_Req.io_Command = HD_SCSICMD;
   iotd->iotd_Req.io_Data = &scsicmd;
   iotd->iotd_Req.io_Length = sizeof(struct SCSICmd);
   if (DoIO((struct IORequest *)iotd))
      return FALSE;
   w2strcpy(id, &data[27], 40);
   return TRUE;
}

