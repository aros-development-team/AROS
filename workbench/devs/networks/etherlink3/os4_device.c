/*

Copyright (C) 2000-2025 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


#include <exec/types.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/exectags.h>
#include <exec/interfaces.h>
#include <interfaces/exec.h>
#include <exec/emulation.h>

#include <proto/exec.h>

#include "device.h"

#include "device_protos.h"


/* Private prototypes */

static struct DevBase *OS4DevInit(struct DevBase *base, APTR seg_list,
   struct ExecIFace *i_exec);
static ULONG IfaceObtain(struct Interface *self);
static ULONG IfaceRelease(struct Interface *self);
static LONG OS4DevOpen(struct Interface *self, struct IOSana2Req *request,
   ULONG unit_num, ULONG flags);
static APTR OS4DevClose(struct Interface *self, struct IOSana2Req *request);
static APTR OS4DevExpunge(struct Interface *self);
static VOID OS4DevBeginIO(struct Interface *self,
   struct IOSana2Req *request);
static VOID OS4DevAbortIO(struct Interface *self,
   struct IOSana2Req *request);
static BOOL RXFunction(struct IOSana2Req *request, APTR buffer, ULONG size);
static BOOL TXFunction(APTR buffer, struct IOSana2Req *request, ULONG size);
static UBYTE *DMATXFunction(struct IOSana2Req *request);
static ULONG OS4Int(struct ExceptionContext ex_context,
   struct ExecBase *sys_base, APTR *int_data);


/* Constant data */

extern const TEXT device_name[];
extern const TEXT version_string[];
extern const struct Resident rom_tag;

static const TEXT manager_name[] = "__device";


static const APTR manager_vectors[] =
{
   (APTR)IfaceObtain,
   (APTR)IfaceRelease,
   (APTR)NULL,
   (APTR)NULL,
   (APTR)OS4DevOpen,
   (APTR)OS4DevClose,
   (APTR)OS4DevExpunge,
   (APTR)NULL,
   (APTR)OS4DevBeginIO,
   (APTR)OS4DevAbortIO,
   (APTR)-1
};


static const struct TagItem manager_tags[] =
{
   {MIT_Name, (UPINT)manager_name},
   {MIT_VectorTable, (UPINT)manager_vectors},
   {MIT_Version, 1},
   {TAG_END, 0}
};


static const struct TagItem *interfaces[] =
{
   manager_tags,
   NULL
};


static const struct TagItem init_tags[] =
{
   {CLT_DataSize, sizeof(struct DevBase)},
   {CLT_InitFunc, (UPINT)OS4DevInit},
   {CLT_Interfaces, (UPINT)interfaces},
   {TAG_END, 0}
};


const struct Resident os4_rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&os4_rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AUTOINIT | RTF_NATIVE,
   VERSION,
   NT_DEVICE,
   0,
   (STRPTR)device_name,
   (STRPTR)version_string,
   (APTR)init_tags
};


/* Private structures */

struct OpenerExtra
{
   struct DevBase *dev_base;
   const VOID *real_rx_function;
   const VOID *real_tx_function;
   const VOID *real_dma_tx_function;
};



/****i* etherlink3.device/OS4DevInit ***************************************
*
*   NAME
*	OS4DevInit
*
*   SYNOPSIS
*	base = OS4DevInit(base, seg_list, i_exec)
*
*	struct DevBase *OS4DevInit(struct DevBase *, APTR, ExecIFace *);
*
****************************************************************************
*
*/

static struct DevBase *OS4DevInit(struct DevBase *base, APTR seg_list,
   struct ExecIFace *i_exec)
{
   base->device.dd_Library.lib_Node.ln_Type = NT_DEVICE;
   base->device.dd_Library.lib_Node.ln_Name = (TEXT *)device_name;
   base->device.dd_Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
   base->device.dd_Library.lib_Version = VERSION;
   base->device.dd_Library.lib_Revision = REVISION;
   base->device.dd_Library.lib_IdString = (TEXT *)version_string;

   base->wrapper_int_code = (APTR)OS4Int;

   /* Do generic device initialisation */

   return DevInit(base, seg_list, (APTR)i_exec);
}



/****i* etherlink3.device/IfaceObtain **************************************
*
*   NAME
*	IfaceObtain
*
*   SYNOPSIS
*	ref_count = IfaceObtain(self)
*
*	ULONG IfaceObtain(struct Interface *);
*
****************************************************************************
*
*/

static ULONG IfaceObtain(struct Interface *self)
{
   return self->Data.RefCount++;
}



/****i* etherlink3.device/IfaceRelease *************************************
*
*   NAME
*	IfaceRelease
*
*   SYNOPSIS
*	ref_count = IfaceRelease(self)
*
*	ULONG IfaceRelease(struct Interface *);
*
****************************************************************************
*
*/

static ULONG IfaceRelease(struct Interface *self)
{
   return --self->Data.RefCount;
}



/****i* etherlink3.device/OS4DevOpen ***************************************
*
*   NAME
*	OS4DevOpen
*
*   SYNOPSIS
*	error = OS4DevOpen(self, request, unit_num,
*	    flags)
*
*	LONG OS4DevOpen(struct Interface *, struct IOSana2Req *, ULONG,
*	    ULONG);
*
****************************************************************************
*
*/

static LONG OS4DevOpen(struct Interface *self, struct IOSana2Req *request,
   ULONG unit_num, ULONG flags)
{
   struct DevBase *base;
   struct Opener *opener;
   struct OpenerExtra *opener_extra;
   BYTE error = 0;

   base = (APTR)self->Data.LibBase;
   opener_extra = AllocMem(sizeof(struct OpenerExtra),
      MEMF_PUBLIC | MEMF_CLEAR);
   if(opener_extra == NULL)
      request->ios2_Req.io_Error = error = IOERR_OPENFAIL;

   if(error == 0)
      error = DevOpen(request, unit_num, flags, (APTR)self->Data.LibBase);

   /* Set up wrapper hooks to hide 68k emulation */

   if(error == 0)
   {
      opener = request->ios2_BufferManagement;
      opener_extra->dev_base = base;
      opener_extra->real_rx_function = opener->rx_function;
      opener_extra->real_tx_function = opener->tx_function;
      opener->rx_function = (APTR)RXFunction;
      opener->tx_function = (APTR)TXFunction;
      if(opener->dma_tx_function != NULL)
      {
         opener_extra->real_dma_tx_function = opener->dma_tx_function;
         opener->dma_tx_function = (APTR)DMATXFunction;
      }
      opener->read_port.mp_Node.ln_Name = (APTR)opener_extra;
   }

   /* Back out if anything went wrong */

   if(error != 0)
   {
      if(opener_extra != NULL)
         FreeMem(opener_extra, sizeof(struct OpenerExtra));
   }

   return error;
}



/****i* etherlink3.device/OS4DevClose **************************************
*
*   NAME
*	OS4DevClose
*
*   SYNOPSIS
*	seg_list = OS4DevClose(self, request)
*
*	APTR OS4DevClose(struct Interface *, struct IOSana2Req *);
*
****************************************************************************
*
*/

static APTR OS4DevClose(struct Interface *self, struct IOSana2Req *request)
{
   struct DevBase *base;
   struct Opener *opener;

   /* Free extra data structure for wrapper hooks hiding 68k emulation */

   base = (APTR)self->Data.LibBase;
   opener = request->ios2_BufferManagement;
   FreeMem(opener->read_port.mp_Node.ln_Name, sizeof(struct OpenerExtra));

   /* Close unit */

   return DevClose(request, (struct DevBase *)self->Data.LibBase);
}



/****i* etherlink3.device/OS4DevExpunge ************************************
*
*   NAME
*	OS4DevExpunge
*
*   SYNOPSIS
*	seg_list = OS4DevExpunge(self)
*
*	APTR OS4DevExpunge(struct Interface *);
*
****************************************************************************
*
*/

static APTR OS4DevExpunge(struct Interface *self)
{
   return DevExpunge((struct DevBase *)self->Data.LibBase);
}



/****i* etherlink3.device/OS4DevBeginIO ************************************
*
*   NAME
*	OS4DevBeginIO
*
*   SYNOPSIS
*	OS4DevBeginIO(self, request)
*
*	VOID OS4DevBeginIO(struct Interface *, struct IORequest *);
*
****************************************************************************
*
*/

static VOID OS4DevBeginIO(struct Interface *self,
   struct IOSana2Req *request)
{
   /* Replace caller's cookie with our own */

   request->ios2_Req.io_Error = 0;
   switch(request->ios2_Req.io_Command)
   {
   case CMD_READ:
   case CMD_WRITE:
   case S2_MULTICAST:
   case S2_BROADCAST:
   case S2_READORPHAN:
      request->ios2_StatData = request->ios2_Data;
      request->ios2_Data = request;
   }

   /* Send request for processing */

   DevBeginIO(request, (APTR)self->Data.LibBase);

   return;
}



/****i* etherlink3.device/OS4DevAbortIO ************************************
*
*   NAME
*	OS4DevAbortIO -- Try to stop a request.
*
*   SYNOPSIS
*	OS4DevAbortIO(self, request)
*
*	VOID OS4DevAbortIO(struct Interface *, struct IOSana2Req *);
*
****************************************************************************
*
*/

static VOID OS4DevAbortIO(struct Interface *self,
   struct IOSana2Req *request)
{
   DevAbortIO(request, (APTR)self->Data.LibBase);

   return;
}



/****i* etherlink3.device/RXFunction ***************************************
*
*   NAME
*	RXFunction
*
****************************************************************************
*
*/

static BOOL RXFunction(struct IOSana2Req *request, APTR buffer, ULONG size)
{
   struct DevBase *base;
   struct Opener *opener;
   struct OpenerExtra *opener_extra;
   APTR cookie;

   opener = request->ios2_BufferManagement;
   opener_extra = (APTR)opener->read_port.mp_Node.ln_Name;
   base = opener_extra->dev_base;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;

   return EmulateTags(opener_extra->real_rx_function,
      ET_RegisterA0, cookie, ET_RegisterA1, buffer,
      ET_RegisterD0, size, TAG_END);
}



/****i* etherlink3.device/TXFunction ***************************************
*
*   NAME
*	TXFunction
*
****************************************************************************
*
*/

static BOOL TXFunction(APTR buffer, struct IOSana2Req *request, ULONG size)
{
   struct DevBase *base;
   struct Opener *opener;
   struct OpenerExtra *opener_extra;
   APTR cookie;

   opener = request->ios2_BufferManagement;
   opener_extra = (APTR)opener->read_port.mp_Node.ln_Name;
   base = opener_extra->dev_base;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;
   return EmulateTags(opener_extra->real_tx_function,
      ET_RegisterA0, buffer, ET_RegisterA1, cookie,
      ET_RegisterD0, size, TAG_END);
}



/****i* etherlink3.device/DMATXFunction ************************************
*
*   NAME
*	DMATXFunction
*
****************************************************************************
*
*/

static UBYTE *DMATXFunction(struct IOSana2Req *request)
{
   struct DevBase *base;
   struct Opener *opener;
   struct OpenerExtra *opener_extra;
   APTR cookie;

   opener = request->ios2_BufferManagement;
   opener_extra = (APTR)opener->read_port.mp_Node.ln_Name;
   base = opener_extra->dev_base;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;
   return (UBYTE *)EmulateTags(opener_extra->real_dma_tx_function,
      ET_RegisterA0, cookie, TAG_END);
}



/****i* etherlink3.device/OS4Int *******************************************
*
*   NAME
*	OS4Int
*
****************************************************************************
*
*/

static ULONG OS4Int(struct ExceptionContext ex_context,
   struct ExecBase *sys_base, APTR *int_data)
{
   BOOL (*int_code)(APTR, APTR, UBYTE);

   int_code = int_data[0];
   return int_code(int_data[1], int_code, 0x4);
}



