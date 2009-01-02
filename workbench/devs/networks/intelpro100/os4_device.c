/*

Copyright (C) 2000-2005 Neil Cafferkey

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

#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <proto/utility.h>

#include "device.h"

#include "unit_protos.h"
#include "pci_protos.h"
#include "request_protos.h"


/* Private prototypes */

static struct DevBase *DevInit(struct DevBase *dev_base, APTR seg_list,
   struct ExecIFace *i_exec);
static ULONG IfaceObtain(struct Interface *self);
static ULONG IfaceRelease(struct Interface *self);
static LONG DevOpen(struct Interface *self, struct IOSana2Req *request,
   ULONG unit_num, ULONG flags);
static APTR DevClose(struct Interface *self, struct IOSana2Req *request);
static APTR DevExpunge(struct Interface *self);
static VOID DevBeginIO(struct Interface *self, struct IOSana2Req *request);
static VOID DevAbortIO(struct Interface *self, struct IOSana2Req *request);
static VOID DeleteDevice(struct DevBase *base);
static struct DevUnit *GetUnit(ULONG unit_num, struct DevBase *base);
static BOOL RXFunction(struct IOSana2Req *request, APTR buffer, ULONG size);
static BOOL TXFunction(APTR buffer, struct IOSana2Req *request, ULONG size);
static UBYTE *DMATXFunction(struct IOSana2Req *request);
static ULONG OS4Int(struct ExceptionContext ex_context,
   struct ExecBase *sys_base, APTR *int_data);


extern const APTR vectors[];
extern const struct TagItem init_tags[];


/* Return an error immediately if someone tries to run the device */

LONG Main()
{
   return -1;
}

const TEXT device_name[] = DEVICE_NAME;
static const TEXT version_string[] =
   DEVICE_NAME " " STR(VERSION) "." STR(REVISION) " (" DATE ")\n";
static const TEXT manager_name[] = "__device";
static const TEXT utility_name[] = UTILITYNAME;
static const TEXT expansion_name[] = EXPANSIONNAME;
static const TEXT timer_name[] = TIMERNAME;


const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AUTOINIT | RTF_NATIVE,
   VERSION,
   NT_DEVICE,
   0,
   (STRPTR)device_name,
   (STRPTR)version_string,
   (APTR)init_tags
};


static const APTR manager_vectors[] =
{
   (APTR)IfaceObtain,
   (APTR)IfaceRelease,
   (APTR)NULL,
   (APTR)NULL,
   (APTR)DevOpen,
   (APTR)DevClose,
   (APTR)DevExpunge,
   (APTR)NULL,
   (APTR)DevBeginIO,
   (APTR)DevAbortIO,
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
   {CLT_InitFunc, (UPINT)DevInit},
   {CLT_Interfaces, (UPINT)interfaces},
   {TAG_END, 0}
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



/****i* intelpro100.device/DevInit *****************************************
*
*   NAME
*	DevInit
*
*   SYNOPSIS
*	dev_base = DevInit(dev_base, seg_list, i_exec)
*
*	struct DevBase *DevInit(struct DevBase *, APTR, ExecIFace *);
*
****************************************************************************
*
*/

static struct DevBase *DevInit(struct DevBase *dev_base, APTR seg_list,
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

   if(OpenDevice(timer_name, UNIT_VBLANK, (APTR)&base->timer_request, 0) !=
      0)
      success = FALSE;

   NewList((APTR)(&dev_base->pci_units));
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



/****i* intelpro100.device/IfaceObtain *************************************
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



/****i* intelpro100.device/IfaceRelease ************************************
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



/****i* intelpro100.device/DevOpen *****************************************
*
*   NAME
*	DevOpen
*
*   SYNOPSIS
*	error = DevOpen(self, request, unit_num, flags)
*
*	LONG DevOpen(struct Interface *, struct IOSana2Req *, ULONG, ULONG);
*
****************************************************************************
*
*/

static LONG DevOpen(struct Interface *self, struct IOSana2Req *request,
   ULONG unit_num, ULONG flags)
{
   struct DevBase *base;
   struct DevUnit *unit;
   BYTE error = 0;
   struct Opener *opener;
   struct TagItem *tag_list;
   UWORD i;

   base = (APTR)self->Data.LibBase;
   base->device.dd_Library.lib_OpenCnt++;
   base->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;

   request->ios2_Req.io_Unit = NULL;
   tag_list = request->ios2_BufferManagement;
   request->ios2_BufferManagement = NULL;

   /* Check request size */

   if(request->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
      error = IOERR_OPENFAIL;

   /* Get the requested unit */

   if(error == 0)
   {
      request->ios2_Req.io_Unit = (APTR)(unit = GetUnit(unit_num, base));
      if(unit == NULL)
         error = IOERR_OPENFAIL;
   }

   /* Handle device sharing */

   if(error == 0)
   {
      if((unit->open_count != 0) && (((unit->flags & UNITF_SHARED) == 0)
         || ((flags & SANA2OPF_MINE) != 0)))
         error = IOERR_UNITBUSY;
      unit->open_count++;
   }

   if(error == 0)
   {
      if((flags & SANA2OPF_MINE) == 0)
         unit->flags |= UNITF_SHARED;
      else if((flags & SANA2OPF_PROM) != 0)
         unit->flags |= UNITF_PROM;

      /* Set up buffer-management structure and get hooks */

      request->ios2_BufferManagement = opener =
         AllocVec(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
      if(opener == NULL)
         error = IOERR_OPENFAIL;
   }

   if(error == 0)
   {
      NewList(&opener->read_port.mp_MsgList);
      opener->read_port.mp_Flags = PA_IGNORE;
      NewList((APTR)&opener->initial_stats);

      for(i = 0; i < 2; i++)
         opener->real_rx_function = (APTR)GetTagData(rx_tags[i],
            (UPINT)opener->real_rx_function, tag_list);
      for(i = 0; i < 3; i++)
         opener->real_tx_function = (APTR)GetTagData(tx_tags[i],
            (UPINT)opener->real_tx_function, tag_list);

      opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, (UPINT)NULL,
         tag_list);
      opener->real_dma_tx_function =
         (APTR)GetTagData(S2_DMACopyFromBuff32, (UPINT)NULL, tag_list);

      opener->rx_function = (APTR)RXFunction;
      opener->tx_function = (APTR)TXFunction;
      if(opener->real_dma_tx_function != NULL)
         opener->dma_tx_function = (APTR)DMATXFunction;

      Disable();
      AddTail((APTR)&unit->openers, (APTR)opener);
      Enable();
   }

   /* Back out if anything went wrong */

   if(error != 0)
      DevClose(self, request);

   /* Return */

   request->ios2_Req.io_Error = error;
   return error;
}



/****i* intelpro100.device/DevClose ****************************************
*
*   NAME
*	DevClose
*
*   SYNOPSIS
*	seg_list = DevClose(request)
*
*	APTR DevClose(struct IOSana2Req *);
*
****************************************************************************
*
*/

static APTR DevClose(struct Interface *self, struct IOSana2Req *request)
{
   struct DevBase *base;
   struct DevUnit *unit;
   APTR seg_list;
   struct Opener *opener;

   /* Free buffer-management resources */

   base = (APTR)self->Data.LibBase;
   opener = (APTR)request->ios2_BufferManagement;
   if(opener != NULL)
   {
      Disable();
      Remove((APTR)opener);
      Enable();
      FreeVec(opener);
   }

   /* Delete the unit if it's no longer in use */

   unit = (APTR)request->ios2_Req.io_Unit;
   if(unit != NULL)
   {
      if((--unit->open_count) == 0)
      {
         Remove((APTR)unit);
         switch(unit->bus)
         {
         case PCI_BUS:
            DeletePCIUnit(unit, base);
         }
      }
   }

   /* Expunge the device if a delayed expunge is pending */

   seg_list = NULL;

   if((--base->device.dd_Library.lib_OpenCnt) == 0)
   {
      if((base->device.dd_Library.lib_Flags & LIBF_DELEXP) != 0)
         seg_list = DevExpunge(self);
   }

   return seg_list;
}



/****i* intelpro100.device/DevExpunge **************************************
*
*   NAME
*	DevExpunge
*
*   SYNOPSIS
*	seg_list = DevExpunge()
*
*	APTR DevExpunge(VOID);
*
****************************************************************************
*
*/

static APTR DevExpunge(struct Interface *self)
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



/****i* intelpro100.device/DevBeginIO **************************************
*
*   NAME
*	DevBeginIO
*
*   SYNOPSIS
*	DevBeginIO(request)
*
*	VOID DevBeginIO(struct IORequest *);
*
****************************************************************************
*
*/

static VOID DevBeginIO(struct Interface *self, struct IOSana2Req *request)
{
   struct DevBase *base;
   struct DevUnit *unit;

   /* Replace caller's cookie with our own */

   base = (APTR)self->Data.LibBase;
   request->ios2_Req.io_Error = 0;
   unit = (APTR)request->ios2_Req.io_Unit;
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

   if(AttemptSemaphore(&unit->access_lock))
      ServiceRequest(request, base);
   else
   {
      PutRequest(unit->request_ports[GENERAL_QUEUE], (APTR)request, base);
   }

   return;
}



/****i* intelpro100.device/DevAbortIO **************************************
*
*   NAME
*	DevAbortIO -- Try to stop a request.
*
*   SYNOPSIS
*	DevAbortIO(request)
*
*	VOID DevAbortIO(struct IOSana2Req *);
*
****************************************************************************
*
* Disable() used instead of a semaphore because device uses interrupts.
*
*/

static VOID DevAbortIO(struct Interface *self, struct IOSana2Req *request)
{
   struct DevBase *base;
   struct DevUnit *unit;

   base = (APTR)self->Data.LibBase;
   unit = (APTR)request->ios2_Req.io_Unit;

   Disable();
   if((request->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE)
      && ((request->ios2_Req.io_Flags & IOF_QUICK) == 0))
   {
      Remove((APTR)request);
      request->ios2_Req.io_Error = IOERR_ABORTED;
      request->ios2_WireError = S2WERR_GENERIC_ERROR;
      ReplyMsg((APTR)request);
   }
   Enable();

   return;
}



/****i* intelpro100.device/DeleteDevice ************************************
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



/****i* intelpro100.device/GetUnit *****************************************
*
*   NAME
*	GetUnit -- Get a unit by number.
*
*   SYNOPSIS
*	unit = GetUnit(unit_num)
*
*	struct DevUnit *GetUnit(ULONG);
*
****************************************************************************
*
*/

static struct DevUnit *GetUnit(ULONG unit_num, struct DevBase *base)
{
   struct DevUnit *unit;
   ULONG pci_limit;

   pci_limit = GetPCICount(base);

   if(unit_num < pci_limit)
      unit = GetPCIUnit(unit_num, base);
   else
      unit = NULL;

   return unit;
}



/****i* intelpro100.device/RXFunction **************************************
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



/****i* intelpro100.device/TXFunction **************************************
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



/****i* intelpro100.device/DMATXFunction ***********************************
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



/****i* intelpro100.device/OS4Int ******************************************
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



