/*

Copyright (C) 2000-2006 Neil Cafferkey

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
#include <exec/emulation.h>
#include <utility/utility.h>
#include <expansion/expansion.h>
#include <resources/card.h>
#include <libraries/pccard.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <proto/utility.h>

#include "device.h"

#include "device_protos.h"
#include "unit_protos.h"
#include "pci_protos.h"
#include "request_protos.h"


/* Private prototypes */

static struct DevBase *OS4DevInit(struct DevBase *dev_base, APTR seg_list,
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
static VOID DeleteDevice(struct DevBase *base);
static BOOL RXFunction(struct IOSana2Req *request, APTR buffer, ULONG size);
static BOOL TXFunction(APTR buffer, struct IOSana2Req *request, ULONG size);
static UBYTE *DMATXFunction(struct IOSana2Req *request);
static ULONG OS4Int(struct ExceptionContext ex_context,
   struct ExecBase *sys_base, APTR *int_data);


extern const TEXT device_name[];
extern const TEXT version_string[];
extern const TEXT utility_name[];
extern const TEXT pccard_name[];
extern const TEXT card_name[];
extern const TEXT timer_name[];
extern const struct Resident rom_tag;

static const TEXT manager_name[] = "__device";
static const TEXT expansion_name[] = EXPANSIONNAME;


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


static const ULONG rx_tags[] =
{
   S2_CopyToBuff,
   S2_CopyToBuff16,
};


static const ULONG tx_tags[] =
{
   S2_CopyFromBuff,
   S2_CopyFromBuff16,
   S2_CopyFromBuff32
};



/****i* etherlink3.device/OS4DevInit ***************************************
*
*   NAME
*	OS4DevInit
*
*   SYNOPSIS
*	dev_base = OS4DevInit(dev_base, seg_list, i_exec)
*
*	struct DevBase *OS4DevInit(struct DevBase *, APTR, ExecIFace *);
*
****************************************************************************
*
*/

static struct DevBase *OS4DevInit(struct DevBase *dev_base, APTR seg_list,
   struct ExecIFace *i_exec)
{
   struct DevBase *base;
   BOOL success = TRUE;

   dev_base->i_exec = i_exec;
   base = dev_base;
   base->seg_list = seg_list;
   base->sys_base = (APTR)i_exec->Data.LibBase;

   base->device.dd_Library.lib_Node.ln_Type = NT_DEVICE;
   base->device.dd_Library.lib_Node.ln_Name = (TEXT *)device_name;
   base->device.dd_Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
   base->device.dd_Library.lib_Version = VERSION;
   base->device.dd_Library.lib_Revision = REVISION;
   base->device.dd_Library.lib_IdString = (TEXT *)version_string;


   base->utility_base = (APTR)OpenLibrary(utility_name, UTILITY_VERSION);
   base->expansion_base = OpenLibrary(expansion_name, EXPANSION_VERSION);
   if(base->utility_base == NULL || base->expansion_base == NULL)
      success = FALSE;
   base->card_base = (APTR)OpenResource(card_name);
   base->pccard_base = (APTR)OpenLibrary(pccard_name, PCCARD_VERSION);

   if(OpenDevice(timer_name, UNIT_ECLOCK, (APTR)&base->timer_request, 0) !=
      0)
      success = FALSE;

   NewList((APTR)(&dev_base->pci_units));
   NewList((APTR)(&dev_base->pccard_units));
   base->wrapper_int_code = (APTR)OS4Int;

   if(success)
   {
      base->i_utility =
        (APTR)GetInterface((APTR)UtilityBase, "main", 1, NULL);
      base->i_pci =
        (APTR)GetInterface(ExpansionBase, "pci", 1, NULL);
      base->i_timer =
        (APTR)GetInterface((APTR)TimerBase, "main", 1, NULL);
      if(base->i_utility == NULL || base->i_pci == NULL
         || base->i_timer == NULL)
         success = FALSE;
   }

   if(!success)
   {
      DeleteDevice(base);
      base = NULL;
   }

   return base;
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
   struct Opener *opener;
   BYTE error;

   error = DevOpen(request, unit_num, flags, (APTR)self->Data.LibBase);

   /* Set up wrapper hooks to hide 68k emulation */

   if(error == 0)
   {
      opener = request->ios2_BufferManagement;
      opener->real_rx_function = opener->rx_function;
      opener->real_tx_function = opener->tx_function;
      opener->rx_function = (APTR)RXFunction;
      opener->tx_function = (APTR)TXFunction;
      if(opener->dma_tx_function != NULL)
      {
         opener->real_dma_tx_function = opener->dma_tx_function;
         opener->dma_tx_function = (APTR)DMATXFunction;
      }
   }

   return error;
}



/****i* etherlink3.device/OS4DevClose **************************************
*
*   NAME
*	OS4DevClose
*
*   SYNOPSIS
*	seg_list = OS4DevClose(request)
*
*	APTR OS4DevClose(struct IOSana2Req *);
*
****************************************************************************
*
*/

static APTR OS4DevClose(struct Interface *self, struct IOSana2Req *request)
{
   struct DevBase *base;
   APTR seg_list = NULL;

   /* Close the unit */

   base = (APTR)self->Data.LibBase;
   CloseUnit(request, base);

   /* Expunge the device if a delayed expunge is pending */

   if(base->device.dd_Library.lib_OpenCnt == 0)
   {
      if((base->device.dd_Library.lib_Flags & LIBF_DELEXP) != 0)
         seg_list = OS4DevExpunge(self);
   }

   return seg_list;
}



/****i* etherlink3.device/OS4DevExpunge ************************************
*
*   NAME
*	OS4DevExpunge
*
*   SYNOPSIS
*	seg_list = OS4DevExpunge()
*
*	APTR OS4DevExpunge(VOID);
*
****************************************************************************
*
*/

static APTR OS4DevExpunge(struct Interface *self)
{
   struct DevBase *base;
   APTR seg_list;

   base = (APTR)self->Data.LibBase;
   if(base->device.dd_Library.lib_OpenCnt == 0)
   {
      seg_list = base->seg_list;
      Remove((APTR)base);
      DeleteDevice(base);
   }
   else
   {
      base->device.dd_Library.lib_Flags |= LIBF_DELEXP;
      seg_list = NULL;
   }

   return seg_list;
}



/****i* etherlink3.device/OS4DevBeginIO ************************************
*
*   NAME
*	OS4DevBeginIO
*
*   SYNOPSIS
*	OS4DevBeginIO(request)
*
*	VOID OS4DevBeginIO(struct IORequest *);
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
*	OS4DevAbortIO(request)
*
*	VOID OS4DevAbortIO(struct IOSana2Req *);
*
****************************************************************************
*
* Disable() used instead of a semaphore because device uses interrupts.
*
*/

static VOID OS4DevAbortIO(struct Interface *self,
   struct IOSana2Req *request)
{
   DevAbortIO(request, (APTR)self->Data.LibBase);

   return;
}



/****i* etherlink3.device/DeleteDevice *************************************
*
*   NAME
*	DeleteDevice
*
*   SYNOPSIS
*	DeleteDevice()
*
*	VOID DeleteDevice(VOID);
*
****************************************************************************
*
*/

static VOID DeleteDevice(struct DevBase *base)
{
   /* Close interfaces */

   DropInterface((APTR)base->i_timer);
   DropInterface((APTR)base->i_pci);
   DropInterface((APTR)base->i_utility);

   /* Close devices */

   CloseDevice((APTR)&base->timer_request);

   /* Close libraries */

   if(base->expansion_base != NULL)
      CloseLibrary(base->expansion_base);
   if(base->utility_base != NULL)
      CloseLibrary((APTR)base->utility_base);

   /* Free device's memory */

   DeleteLibrary((APTR)base);

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
   APTR cookie;

   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   base = (struct DevBase *)request->ios2_Req.io_Device;
   request->ios2_Data = cookie;

   return EmulateTags(opener->real_rx_function,
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
   APTR cookie;

   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   base = (struct DevBase *)request->ios2_Req.io_Device;
   request->ios2_Data = cookie;
   return EmulateTags(opener->real_tx_function,
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
   APTR cookie;

   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   base = (struct DevBase *)request->ios2_Req.io_Device;
   request->ios2_Data = cookie;
   return (UBYTE *)EmulateTags(opener->real_dma_tx_function,
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
   BOOL (*int_code)(APTR, APTR);

   int_code = int_data[0];
   return int_code(int_data[1], int_code);
}



