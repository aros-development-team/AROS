/*

Copyright (C) 2000-2012 Neil Cafferkey

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
#include <utility/utility.h>
#include "initializers.h"

#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <proto/utility.h>

#include "device.h"

#include "device_protos.h"
#include "usb_protos.h"
#include "request_protos.h"


/* Private prototypes */

static VOID DeleteDevice(struct DevBase *base);
static struct DevUnit *GetUnit(ULONG unit_num, struct DevBase *base);


/* Return an error immediately if someone tries to run the device */

LONG Main()
{
   return -1;
}


const TEXT device_name[] = DEVICE_NAME;
const TEXT version_string[] =
   DEVICE_NAME " " STR(VERSION) "." STR(REVISION) " (" DATE ")\n";
static const TEXT utility_name[] = UTILITYNAME;
static const TEXT poseidon_name[] = "poseidon.library";
static const TEXT timer_name[] = TIMERNAME;


static const APTR vectors[] =
{
   (APTR)DevOpen,
   (APTR)DevClose,
   (APTR)DevExpunge,
   (APTR)DevReserved,
   (APTR)DevBeginIO,
   (APTR)DevAbortIO,
   (APTR)-1
};


#ifdef __MORPHOS__
#pragma pack(2)
#endif
const struct
{
   SMALLINITBYTEDEF(type);
   SMALLINITPINTDEF(name);
   SMALLINITBYTEDEF(flags);
   SMALLINITWORDDEF(version);
   SMALLINITWORDDEF(revision);
   SMALLINITPINTDEF(id_string);
   INITENDDEF;
}
init_data =
{
   SMALLINITBYTE(OFFSET(Node, ln_Type), NT_DEVICE),
   SMALLINITPINT(OFFSET(Node, ln_Name), device_name),
   SMALLINITBYTE(OFFSET(Library, lib_Flags), LIBF_SUMUSED | LIBF_CHANGED),
   SMALLINITWORD(OFFSET(Library, lib_Version), VERSION),
   SMALLINITWORD(OFFSET(Library, lib_Revision), REVISION),
   SMALLINITPINT(OFFSET(Library, lib_IdString), version_string),
   INITEND
};
#ifdef __MORPHOS__
#pragma pack()
#endif


static const APTR init_table[] =
{
   (APTR)sizeof(struct DevBase),
   (APTR)vectors,
   (APTR)&init_data,
   (APTR)DevInit
};


const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AUTOINIT,
   VERSION,
   NT_DEVICE,
   0,
   (STRPTR)device_name,
   (STRPTR)version_string,
   (APTR)init_table
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



/****i* realtek8180.device/DevInit *****************************************
*
*   NAME
*	DevInit
*
*   SYNOPSIS
*	dev_base = DevInit(dev_base, seg_list)
*
*	struct DevBase *DevInit(struct DevBase *, APTR);
*
****************************************************************************
*
*/

struct DevBase *DevInit(REG(d0, struct DevBase *dev_base),
   REG(a0, APTR seg_list), REG(BASE_REG, struct DevBase *base))
{
   BOOL success = TRUE;

   /* Initialise base structure */

   dev_base->sys_base = (APTR)base;
   base = dev_base;
   base->seg_list = seg_list;
   NewList((APTR)(&base->usb_units));

   /* Open libraries, resources and devices */

   base->utility_base = (APTR)OpenLibrary(utility_name, UTILITY_VERSION);
   base->poseidon_base = OpenLibrary(poseidon_name, POSEIDON_VERSION);

   if(base->utility_base == NULL)
      success = FALSE;

   if(OpenDevice(timer_name, UNIT_ECLOCK, (APTR)&base->timer_request, 0)
      != 0)
      success = FALSE;

   if(!success)
   {
      DeleteDevice(base);
      base = NULL;
   }

   return base;
}



/****i* realtek8180.device/DevOpen *****************************************
*
*   NAME
*	DevOpen
*
*   SYNOPSIS
*	error = DevOpen(request, unit_num, flags)
*
*	BYTE DevOpen(struct IOSana2Req *, ULONG, ULONG);
*
****************************************************************************
*
*/

BYTE DevOpen(REG(a1, struct IOSana2Req *request),
   REG(d0, ULONG unit_num), REG(d1, ULONG flags),
   REG(BASE_REG, struct DevBase *base))
{
   struct DevUnit *unit;
   BYTE error = 0;
   struct Opener *opener;
   struct TagItem *tag_list;
   UWORD i;

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
      if(unit->open_count != 0 && ((unit->flags & UNITF_SHARED) == 0
         || (flags & SANA2OPF_MINE) != 0))
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
      NewList(&opener->mgmt_port.mp_MsgList);
      opener->mgmt_port.mp_Flags = PA_IGNORE;
      NewList((APTR)&opener->initial_stats);

      for(i = 0; i < 2; i++)
         opener->rx_function = (APTR)GetTagData(rx_tags[i],
            (UPINT)opener->rx_function, tag_list);
      for(i = 0; i < 3; i++)
         opener->tx_function = (APTR)GetTagData(tx_tags[i],
            (UPINT)opener->tx_function, tag_list);

      opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, (UPINT)NULL,
         tag_list);
      opener->dma_tx_function =
         (APTR)GetTagData(S2_DMACopyFromBuff32, (UPINT)NULL, tag_list);

      Disable();
      AddTail((APTR)&unit->openers, (APTR)opener);
      Enable();
   }

   /* Back out if anything went wrong */

   if(error != 0)
      CloseUnit(request, base);

   /* Return */

   request->ios2_Req.io_Error = error;
   return error;
}



/****i* realtek8180.device/DevClose ****************************************
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

APTR DevClose(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base))
{
   APTR seg_list = NULL;

   /* Close the unit */

   CloseUnit(request, base);

   /* Expunge the device if a delayed expunge is pending */

   if(base->device.dd_Library.lib_OpenCnt == 0)
   {
      if((base->device.dd_Library.lib_Flags & LIBF_DELEXP) != 0)
         seg_list = DevExpunge(base);
   }

   return seg_list;
}



/****i* realtek8180.device/DevExpunge **************************************
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

APTR DevExpunge(REG(BASE_REG, struct DevBase *base))
{
   APTR seg_list;

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



/****i* realtek8180.device/DevReserved *************************************
*
*   NAME
*	DevReserved
*
*   SYNOPSIS
*	result = DevReserved()
*
*	APTR DevReserved(VOID);
*
****************************************************************************
*
*/

APTR DevReserved()
{
   return NULL;
}



/****i* realtek8180.device/DevBeginIO **************************************
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

VOID DevBeginIO(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base))
{
   struct DevUnit *unit;

   request->ios2_Req.io_Error = 0;
   unit = (APTR)request->ios2_Req.io_Unit;

   if(AttemptSemaphore(&unit->access_lock))
      ServiceRequest(request, base);
   else
      PutRequest(unit->request_ports[GENERAL_QUEUE], (APTR)request, base);

   return;
}



/****i* realtek8180.device/DevAbortIO **************************************
*
*   NAME
*	DevAbortIO -- Try to stop a request.
*
*   SYNOPSIS
*	DevAbortIO(request)
*
*	VOID DevAbortIO(struct IOSana2Req *);
*
*   FUNCTION
*	Do our best to halt the progress of a request.
*
****************************************************************************
*
*/

VOID DevAbortIO(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base))
{
   Disable();
   if(request->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE &&
      (request->ios2_Req.io_Flags & IOF_QUICK) == 0)
   {
      Remove((APTR)request);
      request->ios2_Req.io_Error = IOERR_ABORTED;
      request->ios2_WireError = S2WERR_GENERIC_ERROR;
      ReplyMsg((APTR)request);
   }
   Enable();

   return;
}



/****i* realtek8180.device/DeleteDevice ************************************
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

VOID DeleteDevice(struct DevBase *base)
{
   UWORD neg_size, pos_size;

   /* Close devices */

   CloseDevice((APTR)&base->timer_request);

   /* Close libraries */

   if(base->poseidon_base != NULL)
      CloseLibrary(base->poseidon_base);
   if(base->utility_base != NULL)
      CloseLibrary((APTR)base->utility_base);

   /* Free device's memory */

   neg_size = base->device.dd_Library.lib_NegSize;
   pos_size = base->device.dd_Library.lib_PosSize;
   FreeMem((UBYTE *)base - neg_size, pos_size + neg_size);

   return;
}



/****i* realtek8180.device/CloseUnit ***************************************
*
*   NAME
*	CloseUnit
*
*   SYNOPSIS
*	CloseUnit(request)
*
*	VOID CloseUnit(struct IOSana2Req *);
*
****************************************************************************
*
*/

VOID CloseUnit(struct IOSana2Req *request, struct DevBase *base)
{
   struct DevUnit *unit;
   struct Opener *opener;

   /* Decrement device usage count and free buffer-management resources */

   base->device.dd_Library.lib_OpenCnt--;
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
         case USB_BUS:
            DeleteUSBUnit(unit, base);
            break;
         }
      }
   }

   return;
}



/****i* realtek8180.device/GetUnit *****************************************
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

struct DevUnit *GetUnit(ULONG unit_num, struct DevBase *base)
{
   struct DevUnit *unit;
   ULONG usb_limit;

   usb_limit = GetUSBCount(base);

   if(unit_num < usb_limit)
      unit = GetUSBUnit(unit_num, base);
   else
      unit = NULL;

   return unit;
}



/****i* realtek8180.device/WrapInt *****************************************
*
*   NAME
*	WrapInt
*
****************************************************************************
*
*/

BOOL WrapInt(struct Interrupt *interrupt, struct DevBase *base)
{
   BOOL success = TRUE;
   APTR *int_data;

   if(base->wrapper_int_code != NULL)
   {
      int_data = AllocMem(2 * sizeof(APTR), MEMF_PUBLIC | MEMF_CLEAR);
      if(int_data != NULL)
      {
         int_data[0] = interrupt->is_Code;
         int_data[1] = interrupt->is_Data;
         interrupt->is_Code = base->wrapper_int_code;
         interrupt->is_Data = int_data;
      }
      else
         success = FALSE;
   }

   return success;
}



/****i* realtek8180.device/UnwrapInt ***************************************
*
*   NAME
*	UnwrapInt
*
****************************************************************************
*
*/

VOID UnwrapInt(struct Interrupt *interrupt, struct DevBase *base)
{
   if(interrupt->is_Code == base->wrapper_int_code)
      FreeMem(interrupt->is_Data, 2 * sizeof(APTR));

   return;
}



